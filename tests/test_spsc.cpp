#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <gtest/gtest.h>
#include <optional>
#include <thread>

#include "qlog/spsc.hpp"

namespace {

template <size_t Capacity, size_t MaxMsgSize, typename T>
  requires std::is_trivially_copyable_v<T>
bool write_helper(qlog::detail::SPSCBuffer<Capacity, MaxMsgSize> &buffer,
                  const T &val) {
  auto buffer_span = buffer.prepare_write(sizeof(T));
  if (buffer_span.empty()) {
    return false;
  }
  std::memcpy(buffer_span.data(), &val, sizeof(T));
  buffer.commit_write(sizeof(T));
  return true;
}

template <typename T, size_t Capacity, size_t MaxMsgSize>
  requires std::is_trivially_copyable_v<T>
std::optional<T>
read_helper(qlog::detail::SPSCBuffer<Capacity, MaxMsgSize> &buffer) {
  auto buffer_span = buffer.prepare_read(sizeof(T));
  if (buffer_span.size() < sizeof(T)) {
    return std::nullopt;
  }
  T res{};
  std::memcpy(&res, buffer_span.data(), sizeof(T));
  buffer.commit_read(sizeof(T));
  return res;
}

constexpr size_t kIterations = 500'000;

template <size_t Capacity, size_t MaxMsgSize>
void producer(qlog::detail::SPSCBuffer<Capacity, MaxMsgSize> &buffer) {
  for (size_t i{}; i < kIterations; ++i) {
    const size_t len = 2 + (i % (MaxMsgSize - 1));
    auto span = buffer.prepare_write(len);
    while (span.empty()) {
      std::this_thread::yield();
      span = buffer.prepare_write(len);
    }
    span[0] = static_cast<std::byte>(len);
    for (size_t j{1}; j != len; ++j) {
      span[j] = static_cast<std::byte>((i + j) & 0xFF);
    }
    buffer.commit_write(len);
  }
}

template <size_t Capacity, size_t MaxMsgSize>
void consumer(qlog::detail::SPSCBuffer<Capacity, MaxMsgSize> &buffer) {
  for (size_t i{}; i < kIterations; ++i) {
    auto span = buffer.prepare_read(1);
    while (span.empty()) {
      std::this_thread::yield();
      span = buffer.prepare_read(1);
    }
    auto expected_len = static_cast<size_t>(span[0]);
    ASSERT_EQ(expected_len, 2 + (i % (MaxMsgSize - 1)));
    while (span.size() < expected_len) {
      span = buffer.prepare_read(expected_len);
    }
    ASSERT_TRUE(span.size() >= expected_len);
    bool all_ok = true;
    for (size_t j{1}; j != expected_len; ++j) {
      all_ok &= span[j] == static_cast<std::byte>((i + j) % 0xFF);
    }
    ASSERT_TRUE(all_ok);
    buffer.commit_read(expected_len);
  }
}

} // namespace

TEST(SPSCBufferUnit, SingleWriteRead) {
  qlog::detail::SPSCBuffer<64, 16> buffer;
  EXPECT_TRUE(buffer.prepare_read().empty());
  const uint64_t val = 0xDEADBEEF;
  EXPECT_TRUE(write_helper(buffer, val));
  const auto read_res = read_helper<uint64_t>(buffer);
  ASSERT_TRUE(read_res.has_value());
  // NOLINTNEXTLINE
  EXPECT_EQ(val, read_res.value());
  EXPECT_TRUE(buffer.prepare_read().empty());
}

TEST(SPSCBufferUnit, BufferFullAndReject) {
  qlog::detail::SPSCBuffer<64, 16> buffer;
  EXPECT_TRUE(buffer.prepare_write(20).empty());
  bool all_write = true;
  for (int i{}; i < 8; ++i) {
    all_write &= write_helper(buffer, 0xDEADBEEFULL);
  }
  EXPECT_TRUE(all_write);
  EXPECT_TRUE(buffer.prepare_write(8).empty());
  auto read_res = read_helper<uint64_t>(buffer);
  ASSERT_TRUE(read_res.has_value());
  // NOLINTNEXTLINE
  EXPECT_EQ(read_res, 0xDEADBEEF);
  EXPECT_FALSE(buffer.prepare_write(8).empty());
}

TEST(SPSCBufferUnit, WrapAroundPocket) {
  qlog::detail::SPSCBuffer<64, 16> buffer;
  for (int i{}; i < 7; ++i) {
    write_helper(buffer, 0xDEADBEEFULL);
    read_helper<uint64_t>(buffer);
  }
  struct MyTest {
    uint64_t first{};
    uint64_t second{};
  };
  ASSERT_TRUE(
      write_helper(buffer, MyTest{.first = 0x01020304, .second = 0x40302010}));
  auto read_res = read_helper<MyTest>(buffer);
  ASSERT_TRUE(read_res.has_value());
  // NOLINTNEXTLINE
  EXPECT_EQ(read_res->first, 0x01020304);
  // NOLINTNEXTLINE
  EXPECT_EQ(read_res->second, 0x40302010);
}

TEST(SPSCBufferUnit, MultithreadedVariableLength) {
  qlog::detail::SPSCBuffer<64, 16> buffer;
  const std::jthread prod(producer<64, 16>, std::ref(buffer));
  const std::jthread cons(consumer<64, 16>, std::ref(buffer));
  EXPECT_TRUE(buffer.prepare_read().empty());
}

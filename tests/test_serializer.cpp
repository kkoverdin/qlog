#include <array>
#include <cstddef>
#include <cstdint>
#include <gtest/gtest.h>
#include <limits>
#include <numbers>

#include "qlog/serializer.hpp"

namespace {

struct Point {
  int x;
  int y;
};

[[maybe_unused]] bool operator==(const Point &lhs, const Point &rhs) {
  return lhs.x == rhs.x && lhs.y == rhs.y;
}

} // namespace
TEST(detailUnit, TriviallySerializable) {
  const int32_t zero{0};
  const uint64_t big_number{std::numeric_limits<uint64_t>::max()};
  const double euler{std::numbers::e};
  const bool correct{true};
  const Point cool_point{.x = 10, .y = 20};
  const int &clvalue = 42;
  EXPECT_EQ(qlog::detail::serialized_size(zero, big_number, euler, correct,
                                          cool_point, clvalue, 42, 3.14),
            45);
  std::array<std::byte, 60> buffer{};
  qlog::detail::serialize_args(buffer, zero, big_number, euler, correct,
                               cool_point, clvalue, 42, 3.14);
  const auto res =
      qlog::detail::deserialize_args<int32_t, uint64_t, double, bool, Point,
                                     int, int, double>(buffer);
  EXPECT_EQ(std::get<0>(res), zero);
  EXPECT_EQ(std::get<1>(res), big_number);
  EXPECT_NEAR(std::get<2>(res), euler, 1e-9);
  EXPECT_EQ(std::get<3>(res), correct);
  EXPECT_EQ(std::get<4>(res), cool_point);
  EXPECT_EQ(std::get<5>(res), clvalue);
  EXPECT_EQ(std::get<6>(res), 42);
  EXPECT_NEAR(std::get<7>(res), 3.14, 1e-9);
}

TEST(detailUnit, StringLike) {
  const char *raw_ptr = "raw c-string";

  EXPECT_EQ(qlog::detail::serialized_size(raw_ptr, "static literal",
                                          std::string{"heap or sso string"},
                                          std::string_view{"view string"}),
            (4 * sizeof(uint16_t)) + 55);
  std::array<std::byte, (4 * sizeof(uint16_t)) + 55> buffer{};
  qlog::detail::serialize_args(buffer, raw_ptr, "static literal",
                               std::string{"heap or sso string"},
                               std::string_view{"view string"});
  const auto res =
      qlog::detail::deserialize_args<std::string_view, std::string_view,
                                     std::string_view, std::string_view>(
          buffer);
  EXPECT_EQ(std::get<0>(res), raw_ptr);
  EXPECT_EQ(std::get<1>(res), "static literal");
  EXPECT_EQ(std::get<2>(res), "heap or sso string");
  EXPECT_EQ(std::get<3>(res), "view string");
}

TEST(detailUnit, EmptyString) {
  EXPECT_EQ(
      qlog::detail::serialized_size("", std::string{}, std::string_view{}), 6);
  std::array<std::byte, 6> buffer{};
  qlog::detail::serialize_args(buffer, "", std::string{}, std::string_view{});
  const auto res =
      qlog::detail::deserialize_args<std::string_view, std::string_view,
                                     std::string_view>(buffer);
  EXPECT_TRUE(std::get<0>(res).empty());
  EXPECT_TRUE(std::get<1>(res).empty());
  EXPECT_TRUE(std::get<2>(res).empty());
}

TEST(detailUnit, NoCopyString) {
  std::string temp = "ephemeral string";
  std::array<std::byte, 20> buffer{};
  qlog::detail::serialize_args(buffer, temp);
  temp = "destroyed";
  const auto res = qlog::detail::deserialize_args<std::string_view>(buffer);
  EXPECT_EQ(std::get<0>(res), "ephemeral string");
  EXPECT_TRUE(buffer.data() <= reinterpret_cast<const std::byte *>(
                                   std::get<0>(res).data()) &&
              reinterpret_cast<const std::byte *>(std::get<0>(res).data()) <=
                  buffer.data() + buffer.size());
}

TEST(detailUnit, UnalignedAccess) {
  std::array<std::byte, 60> buffer{};
  qlog::detail::serialize_args(buffer, uint8_t{1}, "abc",
                               uint64_t{0x1122334455667788ULL},
                               std::numbers::e);
  const auto res = qlog::detail::deserialize_args<uint8_t, std::string_view,
                                                  uint64_t, double>(buffer);
  EXPECT_EQ(std::get<0>(res), 1);
  EXPECT_EQ(std::get<1>(res), "abc");
  EXPECT_EQ(std::get<2>(res), 0x1122334455667788ULL);
  EXPECT_NEAR(std::get<3>(res), std::numbers::e, 1e-9);
}

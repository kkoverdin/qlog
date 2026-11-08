#pragma once

#include <array>
#include <atomic>
#include <bit>
#include <cstddef>
#include <span>

namespace qlog::detail {

template <size_t Capacity, size_t MaxMsgSize>
  requires(std::has_single_bit(Capacity))
class SPSCBuffer {
  static_assert(MaxMsgSize > 0 && MaxMsgSize <= Capacity);

public:
  SPSCBuffer() = default;
  ~SPSCBuffer() = default;

  SPSCBuffer(const SPSCBuffer &other) = delete;
  SPSCBuffer &operator=(const SPSCBuffer &other) = delete;
  SPSCBuffer(SPSCBuffer &&other) = delete;
  SPSCBuffer &operator=(SPSCBuffer &&other) = delete;

  std::span<std::byte> prepare_write(size_t n) noexcept;
  void commit_write(size_t n) noexcept;

  std::span<const std::byte> prepare_read(size_t min_bytes = 1) noexcept;
  void commit_read(size_t n) noexcept;

private:
  static constexpr size_t kMask = Capacity - 1;
  static constexpr size_t kCacheLineSize = 64;

  alignas(kCacheLineSize) std::atomic<size_t> write_pos_{0};
  size_t cached_read_pos_{0};
  alignas(kCacheLineSize) std::atomic<size_t> read_pos_{0};
  size_t cached_write_pos_{0};
  alignas(
      kCacheLineSize) std::array<std::byte, Capacity + MaxMsgSize> buffer_{};
};

template <size_t Capacity, size_t MaxMsgSize>
  requires(std::has_single_bit(Capacity))
std::span<std::byte>
SPSCBuffer<Capacity, MaxMsgSize>::prepare_write(size_t n) noexcept {
  if (n > MaxMsgSize) [[unlikely]] {
    return std::span<std::byte>{};
  }
  const size_t write_pos = write_pos_.load(std::memory_order_relaxed);
  if (Capacity - (write_pos - cached_read_pos_) < n) {
    cached_read_pos_ = read_pos_.load(std::memory_order_acquire);
    if (Capacity - (write_pos - cached_read_pos_) < n) {
      return std::span<std::byte>{};
    }
  }
  return std::span<std::byte>(buffer_.data() + (write_pos & kMask), n);
}

template <size_t Capacity, size_t MaxMsgSize>
  requires(std::has_single_bit(Capacity))
void SPSCBuffer<Capacity, MaxMsgSize>::commit_write(size_t n) noexcept {
  const size_t write_pos = write_pos_.load(std::memory_order_relaxed);
  write_pos_.store(write_pos + n, std::memory_order_release);
}

template <size_t Capacity, size_t MaxMsgSize>
  requires(std::has_single_bit(Capacity))
std::span<const std::byte>
SPSCBuffer<Capacity, MaxMsgSize>::prepare_read(size_t min_bytes) noexcept {
  if (min_bytes == 0 || min_bytes > MaxMsgSize) {
    return std::span<const std::byte>{};
  }
  const size_t current_read = read_pos_.load(std::memory_order_relaxed);
  size_t available = cached_write_pos_ - current_read;
  if (available < min_bytes) {
    cached_write_pos_ = write_pos_.load(std::memory_order_acquire);
    available = cached_write_pos_ - current_read;
    if (available < min_bytes) {
      return std::span<const std::byte>{};
    }
  }
  const size_t phys_idx = current_read & kMask;
  const size_t to_ring_end = Capacity - phys_idx;
  const size_t read_len =
      to_ring_end >= min_bytes ? std::min(available, to_ring_end) : min_bytes;
  return std::span<const std::byte>{buffer_.data() + phys_idx, read_len};
}

template <size_t Capacity, size_t MaxMsgSize>
  requires(std::has_single_bit(Capacity))
void SPSCBuffer<Capacity, MaxMsgSize>::commit_read(size_t n) noexcept {
  read_pos_.fetch_add(n, std::memory_order_release);
}

} // namespace qlog::detail

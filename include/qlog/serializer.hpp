#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>
#include <type_traits>

namespace qlog::detail {

template <typename T>
concept StringLike =
    !std::is_same_v<std::remove_cvref_t<T>, std::nullptr_t> &&
    std::is_convertible_v<std::remove_cvref_t<T>, std::string_view>;

template <typename T>
concept TriviallySerializable =
    !StringLike<T> && std::is_trivially_copyable_v<std::remove_cvref_t<T>>;

template <typename T> size_t serialized_size(const T &arg) {
  if constexpr (TriviallySerializable<T>) {
    return sizeof(std::remove_cvref_t<T>);
  }
  if constexpr (StringLike<T>) {
    return sizeof(uint16_t) + std::string_view{arg}.size();
  }
}

template <typename... Args> size_t serialized_size(const Args &...args) {
  return (size_t{0} + ... + serialized_size(args));
}

template <typename T>
void serialize_one(std::span<std::byte> &buffer, const T &val) {
  if constexpr (TriviallySerializable<T>) {
    std::memcpy(buffer.data(), &val, sizeof(T));
    buffer = buffer.subspan(sizeof(T));
  }
  if constexpr (StringLike<T>) {
    const auto view = std::string_view{val};
    const auto len = static_cast<uint16_t>(view.size());
    std::memcpy(buffer.data(), &len, sizeof(uint16_t));
    buffer = buffer.subspan(sizeof(uint16_t));
    std::memcpy(buffer.data(), view.data(), len);
    buffer = buffer.subspan(len);
  }
}

template <typename... Args>
void serialize_args(std::span<std::byte> buffer, const Args &...args) {
  (serialize_one(buffer, args), ...);
}

template <typename T>
using decoded_type_t = std::conditional_t<
    StringLike<T>, std::string_view,
    std::conditional_t<TriviallySerializable<T>, std::remove_cvref_t<T>,
                       std::nullptr_t>>;

template <typename T>
decoded_type_t<T> deserialize_one(std::span<const std::byte> &buffer) {
  if constexpr (TriviallySerializable<T>) {
    std::remove_cvref_t<T> val{};
    std::memcpy(&val, buffer.data(), sizeof(val));
    buffer = buffer.subspan(sizeof(val));
    return val;
  }
  if constexpr (StringLike<T>) {
    uint16_t len{};
    std::memcpy(&len, buffer.data(), sizeof(uint16_t));
    buffer = buffer.subspan(sizeof(uint16_t));
    auto view =
        std::string_view{reinterpret_cast<const char *>(buffer.data()), len};
    buffer = buffer.subspan(len);
    return view;
  }
}

template <typename... Args>
std::tuple<decoded_type_t<Args>...>
deserialize_args(std::span<const std::byte> buffer) {
  return std::tuple<decoded_type_t<Args>...>{deserialize_one<Args>(buffer)...};
}

} // namespace qlog::detail

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

TEST(SerializerUnit, TriviallySerializable) {
  const int32_t zero{0};
  const uint64_t big_number{std::numeric_limits<uint64_t>::max()};
  const double euler{std::numbers::e};
  const bool correct{true};
  const Point cool_point{.x = 10, .y = 20};
  const int &clvalue = 42;
  EXPECT_EQ(qlog::serializer::serialized_size(zero, big_number, euler, correct,
                                              cool_point, clvalue, 42, 3.14),
            45);
  std::array<std::byte, 60> buffer{};
  qlog::serializer::serialize_args(buffer, zero, big_number, euler, correct,
                                   cool_point, clvalue, 42, 3.14);
  const auto res =
      qlog::serializer::deserialize_args<int32_t, uint64_t, double, bool, Point,
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

TEST(SerializerUnit, StringLike) {
  const char *raw_ptr = "raw c-string";

  EXPECT_EQ(qlog::serializer::serialized_size(raw_ptr, "static literal",
                                              std::string{"heap or sso string"},
                                              std::string_view{"view string"}),
            (4 * sizeof(uint16_t)) + 55);
}

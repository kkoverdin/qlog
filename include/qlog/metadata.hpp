#pragma once

#include <cstddef>
#include <cstdint>
#include <source_location>
#include <span>
#include <string_view>

namespace qlog::meta {

struct LogMetadata {
  enum class LogLevel : uint8_t { Trace, Debug, Info, Warn, Error, Critical };
  LogLevel level;
  static constexpr std::string_view log_level_string(LogLevel lvl) {
    switch (lvl) {
    case LogLevel::Trace:
      return "TRACE";
    case LogLevel::Debug:
      return "DEBUG";
    case LogLevel::Info:
      return "INFO";
    case LogLevel::Warn:
      return "WARN";
    case LogLevel::Error:
      return "ERROR";
    case LogLevel::Critical:
      return "CRITICAL";
    }
  }
  std::string_view format;
  std::source_location location;
};

using FormatFn = void (*)(const LogMetadata *meta, uint64_t timestamp,
                          std::span<const std::byte> payload, std::string &out);

struct LogHeader {
  size_t total_size;
  uint64_t timestamp;
  const LogMetadata *meta;
  FormatFn format_fn;
};

template <typename... Args>
void format_payload(const LogMetadata *meta, uint64_t timestamp,
                    std::span<const std::byte> payload, std::string &out) {}

} // namespace qlog::meta

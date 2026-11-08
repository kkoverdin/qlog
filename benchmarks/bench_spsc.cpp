#include <benchmark/benchmark.h>
#include <cstring>
#include <immintrin.h>
#include <thread>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__linux__)
#include <pthread.h>
#include <sched.h>
#endif

#include "qlog/spsc.hpp"

namespace {

template <size_t Capacity, size_t MaxMsgSize, size_t MsgSize>
void bm_spsc_ping_pong(benchmark::State &state) {
  qlog::detail::SPSCBuffer<Capacity, MaxMsgSize> buffer;
  alignas(64) std::array<std::byte, MaxMsgSize> payload{};
  for (auto cur_state : state) {
    auto span_write = buffer.prepare_write(MsgSize);
    std::memcpy(span_write.data(), payload.data(), MsgSize);
    buffer.commit_write(MsgSize);
    benchmark::ClobberMemory();

    auto span_read = buffer.prepare_read();
    benchmark::DoNotOptimize(payload.data());
    std::memcpy(payload.data(), span_read.data(), MsgSize);
    benchmark::DoNotOptimize(payload.data());
    buffer.commit_read(MsgSize);
  }

  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(MsgSize));
  state.SetItemsProcessed(state.iterations());
}

template <size_t Capacity, size_t MaxMsgSize, size_t MsgSize>
void bm_spsc_multithreaded(benchmark::State &state) {

  qlog::detail::SPSCBuffer<Capacity, MaxMsgSize> buffer;
  alignas(64) std::array<std::byte, MsgSize> payload{};
  std::atomic<bool> consumer_ready{false};

  std::jthread consumer(
      [&buffer, &consumer_ready](const std::stop_token &stoken) {
        alignas(64) std::array<std::byte, MaxMsgSize> read_buf{};
        consumer_ready.store(true, std::memory_order_release);

        while (!stoken.stop_requested()) {
          auto span = buffer.prepare_read(MsgSize);
          if (!span.empty()) {
            std::memcpy(read_buf.data(), span.data(), MsgSize);
            benchmark::DoNotOptimize(read_buf.data());
            buffer.commit_read(MsgSize);
          }
        }
      });

  for (auto cur_state : state) {
    auto span = buffer.prepare_write(MsgSize);
    while (span.empty()) {
      std::this_thread::yield();
      span = buffer.prepare_write(MsgSize);
    }
    benchmark::DoNotOptimize(payload.data());
    std::memcpy(span.data(), payload.data(), MsgSize);
    benchmark::ClobberMemory();
    buffer.commit_write(MsgSize);
  }

  consumer.request_stop();
  consumer.join();

  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(MsgSize));
  state.SetItemsProcessed(state.iterations());
}

} // namespace

BENCHMARK_TEMPLATE(bm_spsc_ping_pong, 1024, 64, 8);
BENCHMARK_TEMPLATE(bm_spsc_ping_pong, 1024, 64, 32);
BENCHMARK_TEMPLATE(bm_spsc_ping_pong, 1024, 64, 64);

BENCHMARK_TEMPLATE(bm_spsc_ping_pong, 65536, 64, 8);
BENCHMARK_TEMPLATE(bm_spsc_ping_pong, 65536, 64, 32);
BENCHMARK_TEMPLATE(bm_spsc_ping_pong, 65536, 64, 64);

BENCHMARK_TEMPLATE(bm_spsc_multithreaded, 1024, 64, 8);
BENCHMARK_TEMPLATE(bm_spsc_multithreaded, 1024, 64, 32);
BENCHMARK_TEMPLATE(bm_spsc_multithreaded, 1024, 64, 64);

BENCHMARK_TEMPLATE(bm_spsc_multithreaded, 65536, 64, 8);
BENCHMARK_TEMPLATE(bm_spsc_multithreaded, 65536, 64, 32);
BENCHMARK_TEMPLATE(bm_spsc_multithreaded, 65536, 64, 64);

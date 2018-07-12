/*
 * bench_main.cpp
 */

#include <vector>

#include "benchmark/benchmark.h"

#include "random.h"
#include "methods.h"

#include <time.h>

constexpr size_t N = 10 * 1000 * 1000;
constexpr bool do_flush = true;

uint32_t* source;

class BenchWithFixture : public ::benchmark::Fixture {
public:

    BenchWithFixture() {
        // stuff here gets called once per defined benchmark (but not every combination of state parameters,
        // and not multiple times for the multiple benchmark runs)
    }

    void SetUp(const ::benchmark::State& state) {
        // stuff here gets called before *every* benchmark run, including any state changes, but also repeated
        // many times even for the same state since the benchmarks run several times with varying iteration counts
        // to get stable timings
        if (!source) {
            printf("Making source... ");
            fflush(stdout);
            source = create_sorted_array(N);
            printf("DONE\n");
            printf("Current uptime (CPU time): %ld\n", clock() * 1000 / CLOCKS_PER_SEC);
        }
    }

    virtual ~BenchWithFixture() {}
};

//pcg32_random_t rstate = PCG_DEFAULT_SEED;

#define BENCH(method) \
    BENCHMARK_DEFINE_F(BenchWithFixture, method)(benchmark::State& state) { \
        pcg32_random_t rstate = PCG_DEFAULT_SEED;                           \
        for (auto _ : state) {                                              \
            if (do_flush) {          \
                state.PauseTiming();                                         \
                array_cache_flush(source, N);                                   \
                state.ResumeTiming();                                           \
            }                                                               \
            method(source, N, pcg32_random_r(&rstate));                     \
        }                                                                   \
    }                                                                       \
    BENCHMARK_REGISTER_F(BenchWithFixture, method);


BENCH(linear_search)
BENCH(binary_search)
BENCH(pf_binary_search)
BENCH(shotgun_binary_search)
BENCH(branchless_binary_search)

BENCHMARK_MAIN();

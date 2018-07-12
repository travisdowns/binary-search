#ifndef RANDOM_H
#define RANDOM_H

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

struct pcg_state_setseq_64 { // Internals are *Private*.
  uint64_t state;            // RNG state.  All values are possible.
  uint64_t inc;              // Controls which RNG sequence (stream) is
                             // selected. Must *always* be odd.
};
typedef struct pcg_state_setseq_64 pcg32_random_t;

#define PCG_DEFAULT_SEED {0x853c49e6748fea9bULL, 0xda3e39cb94b95bdbULL}

static pcg32_random_t pcg32_global = PCG_DEFAULT_SEED;

static inline uint32_t pcg32_random_r(pcg32_random_t *rng) {
  uint64_t oldstate = rng->state;
  rng->state = oldstate * 6364136223846793005ULL + rng->inc;
  uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
  uint32_t rot = oldstate >> 59u;
  return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

static inline uint32_t pcg32_random() { return pcg32_random_r(&pcg32_global); }

static inline int qsort_compare_uint32_t(const void *a, const void *b) {
  if (*(uint32_t *)a > *(uint32_t *)b) return 1;
  if (*(uint32_t *)a < *(uint32_t *)b) return -1;
  return 0;
}

uint32_t *create_sorted_array(size_t length);

uint32_t *create_random_array(size_t count);

uint32_t *randomize(uint32_t *targets, size_t count);

#ifdef __cplusplus
}
#endif

#endif

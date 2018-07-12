// gcc  -std=c99 -O3 -o shotguntest shotguntest.c -Wall -Wextra -lm

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "benchutil.h"
#include "random.h"

// linear search
int32_t __attribute__((noinline))
linear_search(uint32_t *array, int32_t lenarray, uint32_t ikey) {
  for (int32_t i = 0; i < lenarray; i++)
    if (array[i] == ikey)
      return i;
    else if (array[i] > ikey)
      return -(i + 1);
  return -(lenarray + 1);
}

// good old bin. search
int32_t __attribute__((noinline))
binary_search(uint32_t *array, int32_t lenarray, uint32_t ikey) {
  int32_t low = 0;
  int32_t high = lenarray - 1;
  while (low <= high) {
    int32_t middleIndex = (low + high) >> 1;
    uint32_t middleValue = array[middleIndex];
    if (middleValue < ikey) {
      low = middleIndex + 1;
    } else if (middleValue > ikey) {
      high = middleIndex - 1;
    } else {
      return middleIndex;
    }
  }

  return -(low + 1);
}

// experimental "shotgun" bin. search
int32_t __attribute__((noinline))
shotgun_binary_search(uint32_t *array, int32_t lenarray, uint32_t ikey) {
  int32_t low = 0;
  int32_t high = lenarray - 1;
  while (low <= high) {

    int32_t middleIndex = (low + high) >> 1;
    uint32_t middleValue = array[middleIndex];

    int32_t quarterIndex = (low + middleIndex) >> 1;
    uint32_t quarterValue = array[quarterIndex];

    int32_t threequarterIndex = (quarterIndex + high) >> 1;
    uint32_t threequarterValue = array[threequarterIndex];

    if (middleValue < ikey) {
      if (threequarterValue < ikey) {
        low = threequarterIndex + 1;
      } else if (threequarterValue > ikey) {
        high = threequarterIndex - 1;
        low = middleIndex + 1;
      } else
        return threequarterIndex;
    } else if (middleValue > ikey) {
      if (quarterValue < ikey) {
        low = quarterIndex + 1;
        high = middleIndex - 1;
      } else if (quarterValue > ikey) {
        high = quarterIndex - 1;
      } else
        return quarterIndex;
    } else {
      return middleIndex;
    }
  }
  return -(low + 1);
}

//  Paul-Virak Khuong and Pat Morin, http://arxiv.org/pdf/1509.05053.pdf
int32_t __attribute__((noinline))
branchless_binary_search(uint32_t *source, int32_t n, uint32_t target) {
  uint32_t *base = source;
  if (n == 0)
    return -1;
  if (target > source[n - 1])
    return -n - 1; // without this we have a buffer overrun
  while (n > 1) {
    int32_t half = n >> 1;
    base = (base[half] < target) ? &base[half] : base;
    n -= half;
  }
  base += *base < target;
  return *base == target ? base - source : source - base - 1;
}

// number of levels to prefetch ahead, PF_LEVEL 1 means just the next probe point (which is pointless, since a demand
// load is about to be issued for exactly that probe point), PF_LEVEL 2 means the 2 possible probe points that come
// after that, and so on
#define PF_LEVEL 2
// for a PF level of N, the "width" (number of PFs issued) at the last level is 2^N
#define PF_WIDTH (1u << PF_LEVEL)
// hint to __builtin_prefetch
// 0 - no locality (prefetchnta)
// 1 - L3
// 2 - L2
// 3 - L1
#define PF_LOCALITY 3

#define PF_DEBUG_ON 0

// below this threshold we do a linear search which is more efficient than binary search for small sizes
#define BIN_THRESHOLD 1

#if PF_DEBUG_ON
#define PF_DEBUG(...) __VA_ARGS__
#else
#define PF_DEBUG(...)
#endif



static inline void issue_one_pf(uint32_t *p, uint32_t *original_base) {
    PF_DEBUG(printf("PF    issued for %16p (%u)\n", p, (unsigned)(p - original_base)));
    __builtin_prefetch(p, 0, PF_LOCALITY);
}


//__attribute__((noinline))
static inline
void issue_next_pfs(uint32_t *base, uint64_t length, uint32_t *original_base) {
    // level 0
    // base + span * 1/2

    // level 1
    // base + span * 1/4
    // base + span * 3/4

    // level 2
    // base + span * 1/8
    // base + span * 3/8
    // base + span * 5/8
    // base + span * 7/8

#if PF_LEVEL > 0
    if (length > 32) {
        for (uint32_t i = 0; i < PF_WIDTH; i++) {
            uint32_t *p = base + ((length + (i * length << 1) + 1) >> (PF_LEVEL + 1));
    #if PF_DEBUG_ON
            assert(p > base);
            assert(p < base + length);
    #endif
            issue_one_pf(p, original_base);
        }
    }
#endif
}

/** issue the original PF_LEVELS prefetch requests, after which are steady state (issue only the deepest
 *  level per probe */
void __attribute__((noinline))
issue_initial_pfs(uint32_t *base, uint64_t length) {
    PF_DEBUG(printf("\n"));
    for (uint32_t level = 0; level < PF_LEVEL; level++) {
        for (uint32_t i = 0; i < (1u << level); i++) {
            uint32_t *p = base + (length >> (level + 1)) + ((i * length) >> level);
#if PF_DEBUG_ON
            assert(p > base);
            assert(p < base + length);
#endif
            issue_one_pf(p, base);
        }
    }
}

int32_t pf_binary_search(uint32_t *source, int32_t n, uint32_t target) {
    uint32_t *base = source;
    if (n == 0) {
      return -(n + 1);
    }

    issue_initial_pfs(source, n);

    while (n > BIN_THRESHOLD) {
      int32_t half = n >> 1;
      uint32_t probe = base[half];

      // issue prefetch requests for the last level (PF_LEVELS ahead)
      issue_next_pfs(base, n, source);

      PF_DEBUG(printf("PROBE issued for %16p (%u) <<<<\n", base + half, (unsigned)(base + half - source)));
      base = (probe < target) ? base + half : base;
      n -= half;
    }

    uint32_t *end = base + n;
    for (; base < end; base++) {
        if (*base >= target) {
            break;
        }
    }

//    base += *base < target;
    return *base == target ? base - source: source - base - 1;
}

// flushes the array from cache
void array_cache_flush(uint32_t *B, int32_t length) {
  const int32_t CACHELINESIZE = 64; // 64 bytes per cache line
  for (int32_t k = 0; k < length; k += CACHELINESIZE / sizeof(uint32_t)) {
      void *ptr = B + k;
      __asm__ __volatile__("clflushopt (%0)\t\n": : "r"(ptr) :"memory");
//    __builtin_ia32_clflush(B + k);
  }
  __asm__ __volatile__("lfence\n mfence\n": : :"memory");
}

// tries to put the array in cache
void array_cache_prefetch(uint32_t *B, int32_t length) {
  const int32_t CACHELINESIZE = 64; // 64 bytes per cache line
  for (int32_t k = 0; k < length; k += CACHELINESIZE / sizeof(uint32_t)) {
    __builtin_prefetch(B + k);
  }
}


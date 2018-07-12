// gcc  -std=c99 -O3 -o shotguntest shotguntest.c -Wall -Wextra -lm

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "benchutil.h"
#include "random.h"
#include "methods.h"

#undef NDEBUG
#include <assert.h>

void check_result_fn(const uint32_t *source, uint32_t needle, int32_t idx, const char *str) {
    if (idx < 0) {
        fprintf(stderr, "FAILED: for %s expected to find the test value but got %d", str, idx);
        abort();
    }
    if (source[idx] != needle) {
        fprintf(stderr, "FAILED for %s found %u at index but got %u", str, source[idx], needle);
        abort();
    }
}

#define check_result(value) check_result_fn(source, needle, value, #value);


void demo() {
  size_t nbrtestvalues = 100;
  uint32_t *testvalues = create_random_array(nbrtestvalues);
  int32_t bogus = 0;
  printf("# Objective: fast search in large arrays.\n");
  printf("# We report the average number of cycles per query.\n");
  size_t N = 10000000;
  printf("# Array size = %zu (elements), %f (MB).\n", N,
         N * sizeof(uint32_t) / (1024 * 1024.0));
  printf("# We do  %zu consecutive queries, but we try to flush the cache "
         "between queries.\n",
         nbrtestvalues);
  printf("# creating sorted random array (takes some time) ");
  fflush(NULL);
  uint32_t *source = create_sorted_array(N);
  for (size_t tv = 0; tv < nbrtestvalues; tv++)
    testvalues[tv] = source[testvalues[tv] % N];
  printf(" Done! \n");
  printf("# Running sanity tests: ");
  fflush(NULL);
  ASSERT_PRE_ARRAY(source, N, binary_search, testvalues, nbrtestvalues);
  ASSERT_PRE_ARRAY(source, N, branchless_binary_search, testvalues,
                   nbrtestvalues);
  ASSERT_PRE_ARRAY(source, N, shotgun_binary_search, testvalues, nbrtestvalues);
  for (size_t k = 0; k < nbrtestvalues; k++) {
      uint32_t needle = testvalues[k];

      int32_t v1 = binary_search           (source, N, needle);
      int32_t v2 = branchless_binary_search(source, N, needle);
      int32_t v3 = shotgun_binary_search   (source, N, needle);
      int32_t v4 = linear_search           (source, N, needle);
      int32_t v5 = pf_binary_search    (source, N, needle);

      check_result(v1);
      check_result(v2);
      check_result(v3);
      check_result(v4);
      check_result(v5);
  }
  printf(" Done! \n");

  int repeats = 3;
  printf("Running out-of-cache benchmarks (%d times each)\n", repeats);
  for (int k = 0; k < repeats; k++)
    BEST_TIME_PRE_ARRAY(source, N, binary_search, array_cache_flush, testvalues,
                        nbrtestvalues, bogus);
  for (int k = 0; k < repeats; k++)
    BEST_TIME_PRE_ARRAY(source, N, branchless_binary_search, array_cache_flush,
                        testvalues, nbrtestvalues, bogus);
  for (int k = 0; k < repeats; k++)
    BEST_TIME_PRE_ARRAY(source, N, shotgun_binary_search, array_cache_flush,
                        testvalues, nbrtestvalues, bogus);
  for (int k = 0; k < repeats; k++)
      BEST_TIME_PRE_ARRAY(source, N, pf_binary_search, array_cache_flush,
                          testvalues, nbrtestvalues, bogus);
  /*printf("\n With data in cache as much as possible :\n");
  BEST_TIME_PRE_ARRAY(source, N, binary_search, array_cache_prefetch,
                      testvalues, nbrtestvalues, bogus);
  BEST_TIME_PRE_ARRAY(source, N, branchless_binary_search, array_cache_prefetch,
                      testvalues, nbrtestvalues, bogus);
  BEST_TIME_PRE_ARRAY(source, N, shotgun_binary_search, array_cache_prefetch,
                      testvalues, nbrtestvalues, bogus);
  */
  free(source);
  printf("\n");
  printf("bogus = %d \n", bogus);
  free(testvalues);
}

int main() {
  for (int t = 0; t < 3; ++t) {
    demo();
    printf("=====\n\n");
  }
  return 0;
}

/*
 * methods.h
 */

#ifndef METHODS_H_
#define METHODS_H_

#ifdef __cplusplus
extern "C" {
#endif

int32_t linear_search(uint32_t *source, int32_t n, uint32_t target);
int32_t binary_search(uint32_t *source, int32_t n, uint32_t target);
int32_t shotgun_binary_search(uint32_t *source, int32_t n, uint32_t target);
int32_t branchless_binary_search(uint32_t *source, int32_t n, uint32_t target);
int32_t pf_binary_search(uint32_t *source, int32_t n, uint32_t target);

/* tries to prefetch N levels ahead and uses cmov to resolve the results */
int32_t travis1_binary_search(uint32_t *source, int32_t n, uint32_t target);

// utility
// flushes the array from cache
void array_cache_flush(uint32_t *B, int32_t length);

// tries to put the array in cache
void array_cache_prefetch(uint32_t *B, int32_t length);

#ifdef __cplusplus
}
#endif

#endif /* METHODS_H_ */

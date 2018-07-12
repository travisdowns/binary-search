#include "random.h"



uint32_t *create_sorted_array(size_t length) {
  uint32_t *array = (uint32_t *)malloc(length * sizeof(uint32_t));
  for (size_t i = 0; i < length; i++) {
    uint32_t v = (uint32_t)pcg32_random();
//    printf("V: %u\n", v);
    array[i] = v;
  }
  qsort(array, length, sizeof(*array), qsort_compare_uint32_t);
  return array;
}

uint32_t *create_random_array(size_t count) {
  uint32_t *targets = (uint32_t *)malloc(count * sizeof(uint32_t));
  for (size_t i = 0; i < count; i++)
    targets[i] = (uint32_t)pcg32_random();
  return targets;
}

uint32_t *randomize(uint32_t *targets, size_t count) {
  for (size_t i = 0; i < count; i++)
    targets[i] = (uint32_t)pcg32_random();
  return targets;
}


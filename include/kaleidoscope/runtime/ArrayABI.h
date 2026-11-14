#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Ranked memref descriptor for the _mlir_ciface_* wrappers (64-bit host).
   Only borrowed contiguous f64 arrays: offset=0, stride=1, 0<=size<=INT32_MAX.
   Caller owns storage and must provide a valid descriptor for the whole call. */
typedef struct KaleArray {
    double *allocated;
    double *aligned;
    int64_t offset;
    int64_t sizes[1];
    int64_t strides[1];
} KaleArray;
#ifdef __cplusplus
}
#endif

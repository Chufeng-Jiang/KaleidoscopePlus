#pragma once
#include <stddef.h>
#include <stdint.h>
#include "kaleidoscope/runtime/ArrayABI.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Teaching ABI, synchronous, single-device, thread-confined. Not OpenACC ABI. */
typedef struct KrtSession KrtSession;
typedef double (*KrtElement)(double, double, double);
enum KrtStatus {
    KRT_OK = 0,
    KRT_INVALID = 1,
    KRT_NOT_FOUND = 2,
    KRT_STATE = 3,
    KRT_DEVICE_ERROR = 4,
    KRT_NO_DEVICE = 5,
    KRT_INTERNAL = 6
};
typedef struct KrtStats {
    double h2d_ms, kernel_ms, d2h_ms;
    uint64_t allocations, live_allocations, launches;
} KrtStats;
int krt_create(const char *backend, KrtSession **out);
void krt_destroy(KrtSession *session);
const char *krt_error(KrtSession *session);
const char *krt_device(KrtSession *session);
/* Re-entering exactly the same range is idempotent (no reference counting).
   Overlapping/changed ranges are rejected. The caller retains host ownership. */
int krt_enter(KrtSession *, void *host, size_t bytes);
int krt_copyin(KrtSession *, void *host);
int krt_copyout(KrtSession *, void *host);
int krt_exit(KrtSession *, void *host);
int krt_set_cpu_kernel(KrtSession *, KrtElement);
int krt_load_ptx(KrtSession *, const char *path);
/* out[i] = element(x[i], y[i], a). All three ranges must be mapped. */
int krt_launch(KrtSession *, void *x, void *y, void *out, double a, size_t count);
int krt_wait(KrtSession *);
int krt_stats(KrtSession *, KrtStats *out);
/* Synchronous scoped teaching directive. Equal-length, contiguous, disjoint
   descriptors; ranges must not already be mapped. No nested data regions. */
int krt_acc_map(KrtSession *, const KaleArray *x, const KaleArray *y, KaleArray *out, double a,
                KrtElement element);
#ifdef __cplusplus
}
#endif

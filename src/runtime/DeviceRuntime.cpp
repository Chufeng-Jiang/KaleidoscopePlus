#include "kaleidoscope/runtime/DeviceRuntime.h"
#include <chrono>
#include <cstring>
#include <limits>
#include <map>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#ifdef KALE_HAS_CUDA
#include <cuda.h>
#endif

using Clock = std::chrono::steady_clock;
static double elapsed(Clock::time_point t) {
    return std::chrono::duration<double, std::milli>(Clock::now() - t).count();
}
struct Buffer {
    size_t bytes = 0;
    bool initialized = false;
    std::unique_ptr<unsigned char[]> cpu;
#ifdef KALE_HAS_CUDA
    CUdeviceptr device = 0;
#endif
};
struct KrtSession {
    bool cuda = false;
    std::string error, deviceName = "CPU simulation";
    std::map<void *, Buffer> buffers;
    KrtElement element = nullptr;
    KrtStats stats{};
#ifdef KALE_HAS_CUDA
    CUdevice device = 0;
    CUcontext context = nullptr;
    CUmodule module = nullptr;
    CUfunction kernel = nullptr;
    CUevent start = nullptr, stop = nullptr;
#endif
};
static int fail(KrtSession *s, int status, const char *message) {
    if (s)
        s->error = message;
    return status;
}
#ifdef KALE_HAS_CUDA
static void check(CUresult code) {
    if (code == CUDA_SUCCESS)
        return;
    const char *name = nullptr, *message = nullptr;
    cuGetErrorName(code, &name);
    cuGetErrorString(code, &message);
    throw std::runtime_error(std::string(name ? name : "CUDA error") + ": " +
                             (message ? message : "unknown"));
}
struct ContextGuard {
    bool active;
    explicit ContextGuard(KrtSession *s) : active(s->cuda) {
        if (active)
            check(cuCtxPushCurrent(s->context));
    }
    ~ContextGuard() {
        if (active) {
            CUcontext old;
            cuCtxPopCurrent(&old);
        }
    }
};
#else
struct ContextGuard {
    explicit ContextGuard(KrtSession *) {}
};
#endif
template <class F> static int invoke(KrtSession *s, F fn) {
    if (!s)
        return KRT_INVALID;
    try {
        s->error.clear();
        ContextGuard guard(s);
        return fn();
    } catch (const std::exception &e) {
        s->error = e.what();
        return s->cuda ? KRT_DEVICE_ERROR : KRT_INTERNAL;
    } catch (...) {
        return fail(s, KRT_INTERNAL, "Unknown runtime failure");
    }
}
extern "C" {
int krt_create(const char *backend, KrtSession **out) {
    if (!out)
        return KRT_INVALID;
    *out = nullptr;
    if (!backend || (std::strcmp(backend, "cpu") && std::strcmp(backend, "cuda")))
        return KRT_INVALID;
    auto *s = new (std::nothrow) KrtSession;
    if (!s)
        return KRT_INTERNAL;
    s->cuda = !std::strcmp(backend, "cuda");
    if (s->cuda) {
#ifdef KALE_HAS_CUDA
        try {
            check(cuInit(0));
            check(cuDeviceGet(&s->device, 0));
            check(cuDevicePrimaryCtxRetain(&s->context, s->device));
            ContextGuard guard(s);
            char name[256];
            check(cuDeviceGetName(name, sizeof(name), s->device));
            s->deviceName = name;
            check(cuEventCreate(&s->start, CU_EVENT_DEFAULT));
            check(cuEventCreate(&s->stop, CU_EVENT_DEFAULT));
        } catch (...) {
            krt_destroy(s);
            return KRT_NO_DEVICE;
        }
#else
        delete s;
        return KRT_NO_DEVICE;
#endif
    }
    *out = s;
    return KRT_OK;
}
void krt_destroy(KrtSession *s) {
    if (!s)
        return;
#ifdef KALE_HAS_CUDA
    if (s->cuda && s->context) {
        if (cuCtxPushCurrent(s->context) == CUDA_SUCCESS) {
            cuCtxSynchronize();
            for (auto &entry : s->buffers)
                if (entry.second.device)
                    cuMemFree(entry.second.device);
            if (s->module)
                cuModuleUnload(s->module);
            if (s->start)
                cuEventDestroy(s->start);
            if (s->stop)
                cuEventDestroy(s->stop);
            CUcontext old;
            cuCtxPopCurrent(&old);
        }
        cuDevicePrimaryCtxRelease(s->device);
    }
#endif
    delete s;
}
const char *krt_error(KrtSession *s) { return s ? s->error.c_str() : "Invalid session"; }
const char *krt_device(KrtSession *s) { return s ? s->deviceName.c_str() : "Invalid session"; }
int krt_enter(KrtSession *s, void *host, size_t bytes) {
    return invoke(s, [&]() -> int {
        if (!host)
            return fail(s, KRT_INVALID,
                        "Mapping requires a non-null host identity, including empty ranges");
        auto address = reinterpret_cast<uintptr_t>(host);
        if (bytes > std::numeric_limits<uintptr_t>::max() - address)
            return fail(s, KRT_INVALID, "Range overflow");
        auto existing = s->buffers.find(host);
        if (existing != s->buffers.end())
            return existing->second.bytes == bytes ? KRT_OK
                                                   : fail(s, KRT_INVALID, "Mapping size changed");
        for (auto &entry : s->buffers) {
            auto start = reinterpret_cast<uintptr_t>(entry.first);
            if (bytes && entry.second.bytes && address < start + entry.second.bytes &&
                start < address + bytes)
                return fail(s, KRT_INVALID, "Overlapping host mappings are unsupported");
        }
        // Insert before allocation so allocation failure cannot leak device memory.
        auto [it, inserted] = s->buffers.try_emplace(host);
        try {
            auto &buffer = it->second;
            buffer.bytes = bytes;
            if (bytes) {
#ifdef KALE_HAS_CUDA
                if (s->cuda)
                    check(cuMemAlloc(&buffer.device, bytes));
                else
#endif
                    buffer.cpu = std::make_unique<unsigned char[]>(bytes);
                ++s->stats.allocations;
                ++s->stats.live_allocations;
            }
        } catch (...) {
            s->buffers.erase(it);
            throw;
        }
        return KRT_OK;
    });
}
int krt_copyin(KrtSession *s, void *host) {
    return invoke(s, [&]() -> int {
        auto it = s->buffers.find(host);
        if (it == s->buffers.end())
            return fail(s, KRT_NOT_FOUND, "Host range is not mapped");
        auto &buf = it->second;
        buf.initialized = false; // A failed replacement copy must not expose old/partial contents.
        auto t = Clock::now();
        if (buf.bytes) {
#ifdef KALE_HAS_CUDA
            if (s->cuda)
                check(cuMemcpyHtoD(buf.device, host, buf.bytes));
            else
#endif
                std::memcpy(buf.cpu.get(), host, buf.bytes);
        }
        buf.initialized = true;
        s->stats.h2d_ms += elapsed(t);
        return KRT_OK;
    });
}
int krt_copyout(KrtSession *s, void *host) {
    return invoke(s, [&]() -> int {
        auto it = s->buffers.find(host);
        if (it == s->buffers.end())
            return fail(s, KRT_NOT_FOUND, "Host range is not mapped");
        auto &buf = it->second;
        if (!buf.initialized)
            return fail(s, KRT_STATE, "Device range is uninitialized");
        auto t = Clock::now();
        if (buf.bytes) {
#ifdef KALE_HAS_CUDA
            if (s->cuda)
                check(cuMemcpyDtoH(host, buf.device, buf.bytes));
            else
#endif
                std::memcpy(host, buf.cpu.get(), buf.bytes);
        }
        s->stats.d2h_ms += elapsed(t);
        return KRT_OK;
    });
}
int krt_exit(KrtSession *s, void *host) {
    return invoke(s, [&]() -> int {
        auto it = s->buffers.find(host);
        if (it == s->buffers.end())
            return fail(s, KRT_NOT_FOUND, "Host range is not mapped");
#ifdef KALE_HAS_CUDA
        if (s->cuda && it->second.device)
            check(cuMemFree(it->second.device));
#endif
        if (it->second.bytes)
            --s->stats.live_allocations;
        s->buffers.erase(it);
        return KRT_OK;
    });
}
int krt_set_cpu_kernel(KrtSession *s, KrtElement fn) {
    return invoke(s, [&]() -> int {
        if (s->cuda || !fn)
            return fail(s, KRT_INVALID, "Expected CPU backend and non-null kernel");
        s->element = fn;
        return KRT_OK;
    });
}
int krt_load_ptx(KrtSession *s, const char *path) {
    return invoke(s, [&]() -> int {
        if (!s->cuda || !path)
            return fail(s, KRT_INVALID, "Expected CUDA backend and PTX path");
#ifdef KALE_HAS_CUDA
        CUmodule next = nullptr;
        check(cuModuleLoad(&next, path));
        CUfunction fn = nullptr;
        auto status = cuModuleGetFunction(&fn, next, "kale_map");
        if (status != CUDA_SUCCESS) {
            cuModuleUnload(next);
            check(status);
        }
        if (s->module) {
            status = cuModuleUnload(s->module);
            if (status != CUDA_SUCCESS) {
                cuModuleUnload(next);
                check(status);
            }
        }
        s->module = next;
        s->kernel = fn;
        return KRT_OK;
#else
        return KRT_NO_DEVICE;
#endif
    });
}
int krt_launch(KrtSession *s, void *x, void *y, void *out, double a, size_t n) {
    return invoke(s, [&]() -> int {
        if (n > static_cast<size_t>(2147483647) ||
            n > std::numeric_limits<size_t>::max() / sizeof(double))
            return fail(s, KRT_INVALID, "Element count exceeds the teaching ABI limit");
        auto ix = s->buffers.find(x), iy = s->buffers.find(y), io = s->buffers.find(out);
        if (ix == s->buffers.end() || iy == s->buffers.end() || io == s->buffers.end())
            return fail(s, KRT_NOT_FOUND, "Kernel range is not mapped");
        auto &bx = ix->second;
        auto &by = iy->second;
        auto &bo = io->second;
        // Require complete ranges: marking a partially written output initialized is unsafe.
        if (bx.bytes != n * sizeof(double) || by.bytes != n * sizeof(double) ||
            bo.bytes != n * sizeof(double))
            return fail(s, KRT_INVALID, "Kernel count must match all mapped ranges");
        if (n && (!bx.initialized || !by.initialized))
            return fail(s, KRT_STATE, "Kernel inputs require copyin");
        bo.initialized = false; // Failed launches cannot make partially written data readable.
#ifdef KALE_HAS_CUDA
        if (s->cuda) {
            if (!s->kernel)
                return fail(s, KRT_STATE, "PTX kernel is not loaded");
            if (n) {
                uint64_t count = n;
                void *args[] = {&bx.device, &by.device, &bo.device, &a, &count};
                check(cuEventRecord(s->start, nullptr));
                check(cuLaunchKernel(s->kernel, static_cast<unsigned>((n + 255) / 256), 1, 1, 256,
                                     1, 1, 0, nullptr, args, nullptr));
                check(cuEventRecord(s->stop, nullptr));
                check(cuEventSynchronize(s->stop));
                float ms = 0;
                check(cuEventElapsedTime(&ms, s->start, s->stop));
                s->stats.kernel_ms += ms;
            }
        } else
#endif
        {
            if (!s->element)
                return fail(s, KRT_STATE, "CPU kernel is not set");
            auto begin = Clock::now();
            // memcpy keeps the raw byte buffers independent of alignment/lifetime assumptions.
            for (size_t i = 0; i < n; ++i) {
                double vx, vy;
                std::memcpy(&vx, bx.cpu.get() + i * sizeof(double), sizeof(double));
                std::memcpy(&vy, by.cpu.get() + i * sizeof(double), sizeof(double));
                auto v = s->element(vx, vy, a);
                std::memcpy(bo.cpu.get() + i * sizeof(double), &v, sizeof(double));
            }
            s->stats.kernel_ms += elapsed(begin);
        }
        bo.initialized = true;
        if (n)
            ++s->stats.launches;
        return KRT_OK;
    });
}
int krt_wait(KrtSession *s) {
    return invoke(s, [&]() -> int {
#ifdef KALE_HAS_CUDA
        if (s->cuda)
            check(cuCtxSynchronize());
#endif
        return KRT_OK;
    });
}
int krt_stats(KrtSession *s, KrtStats *out) {
    if (!s || !out)
        return KRT_INVALID;
    *out = s->stats;
    return KRT_OK;
}
int krt_acc_map(KrtSession *s, const KaleArray *x, const KaleArray *y, KaleArray *out, double a,
                KrtElement element) {
    return invoke(s, [&]() -> int {
        if (!x || !y || !out || !element)
            return fail(s, KRT_INVALID, "Invalid #acc descriptor or element");
        const KaleArray *arrays[] = {x, y, out};
        for (auto *array : arrays) {
            if (!array->aligned || array->allocated != array->aligned || array->offset != 0 ||
                array->strides[0] != 1 || array->sizes[0] < 0 || array->sizes[0] > 2147483647 ||
                array->sizes[0] != x->sizes[0])
                return fail(s, KRT_INVALID,
                            "#acc requires equal-length contiguous array descriptors");
            if (s->buffers.count(array->aligned))
                return fail(s, KRT_STATE, "#acc scoped arrays must not already be mapped");
        }
        size_t n = static_cast<size_t>(x->sizes[0]), bytes = n * sizeof(double);
        for (unsigned i = 0; i < 3; ++i) {
            auto begin = reinterpret_cast<uintptr_t>(arrays[i]->aligned);
            if (bytes > std::numeric_limits<uintptr_t>::max() - begin)
                return fail(s, KRT_INVALID, "#acc array range overflow");
            for (unsigned j = 0; j < i; ++j) {
                auto other = reinterpret_cast<uintptr_t>(arrays[j]->aligned);
                if (begin == other || (bytes && begin < other + bytes && other < begin + bytes))
                    return fail(s, KRT_INVALID, "#acc requires disjoint array ranges");
            }
        }
#ifdef KALE_HAS_CUDA
        if (s->cuda && !s->kernel)
            return fail(s, KRT_STATE, "#acc CUDA session requires its compiled PTX module");
#endif
        if (!s->cuda)
            s->element = element;
        void *hosts[] = {x->aligned, y->aligned, out->aligned};
        unsigned owned = 0;
        int status = KRT_OK;
        for (; owned < 3; ++owned) {
            status = krt_enter(s, hosts[owned], bytes);
            if (status != KRT_OK)
                break;
        }
        if (status == KRT_OK)
            status = krt_copyin(s, hosts[0]);
        if (status == KRT_OK)
            status = krt_copyin(s, hosts[1]);
        if (status == KRT_OK)
            status = krt_launch(s, hosts[0], hosts[1], hosts[2], a, n);
        if (status == KRT_OK)
            status = krt_wait(s);
        if (status == KRT_OK)
            status = krt_copyout(s, hosts[2]);
        std::string message = s->error;
        while (owned) {
            int cleanup = krt_exit(s, hosts[--owned]);
            if (status == KRT_OK && cleanup != KRT_OK) {
                status = cleanup;
                message = s->error;
            }
        }
        s->error = std::move(message);
        return status;
    });
}
}

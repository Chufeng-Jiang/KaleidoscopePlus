#include "kaleidoscope/runtime/ArrayABI.h"
#include "kaleidoscope/runtime/DeviceRuntime.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
extern "C" double element(double, double, double);
extern "C" double _mlir_ciface_vector(KaleArray *, KaleArray *, KaleArray *, double);

static KaleArray descriptor(std::vector<double> &v, size_t n) {
    return {v.data(), v.data(), 0, {static_cast<int64_t>(n)}, {1}};
}
int main(int argc, char **argv) {
    try {
        std::string backend = argc > 1 ? argv[1] : "cpu";
        std::string ptx = argc > 2 ? argv[2] : "";
        KrtSession *raw = nullptr;
        int status = krt_create(backend.c_str(), &raw);
        if (status != KRT_OK) {
            std::cerr << "Runtime initialization failed, status=" << status << '\n';
            return status == KRT_NO_DEVICE ? 77 : 1;
        }
        std::unique_ptr<KrtSession, decltype(&krt_destroy)> session(raw, krt_destroy);
        auto check = [&](int code) {
            if (code != KRT_OK)
                throw std::runtime_error(krt_error(raw));
        };
        if (backend == "cuda")
            check(krt_load_ptx(raw, ptx.c_str()));
        else
            check(krt_set_cpu_kernel(raw, element));
        std::cout << "backend=" << backend << " device=" << krt_device(raw) << '\n';
        for (size_t n : {size_t(0), size_t(1), size_t(17), size_t(257), size_t(100003)}) {
            std::vector<double> x(n + 1), y(n + 1), out(n + 1, -9876), reference(n + 1, -9876);
            for (size_t i = 0; i < n; ++i) {
                x[i] = (static_cast<int>(i % 97) - 48) * 0.125;
                y[i] = (i % 31) * 0.25;
            }
            double a = 2.25;
            auto dx = descriptor(x, n), dy = descriptor(y, n), dr = descriptor(reference, n);
            _mlir_ciface_vector(&dx, &dy, &dr, a);
            auto begin = std::chrono::steady_clock::now();
            KrtStats before{};
            check(krt_stats(raw, &before));
            for (void *p : {static_cast<void *>(x.data()), static_cast<void *>(y.data()),
                            static_cast<void *>(out.data())})
                check(krt_enter(raw, p, n * sizeof(double)));
            KrtStats entered{};
            check(krt_stats(raw, &entered));
            check(krt_enter(raw, x.data(), n * sizeof(double)));
            KrtStats repeated{};
            check(krt_stats(raw, &repeated));
            if (entered.allocations != repeated.allocations)
                throw std::runtime_error("Repeated enter allocated twice");
            check(krt_copyin(raw, x.data()));
            check(krt_copyin(raw, y.data()));
            check(krt_launch(raw, x.data(), y.data(), out.data(), a, n));
            check(krt_wait(raw));
            check(krt_copyout(raw, out.data()));
            for (void *p : {static_cast<void *>(x.data()), static_cast<void *>(y.data()),
                            static_cast<void *>(out.data())})
                check(krt_exit(raw, p));
            auto total =
                std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin)
                    .count();
            KrtStats after{};
            check(krt_stats(raw, &after));
            if (after.live_allocations)
                throw std::runtime_error("Leaked mapping");
            for (size_t i = 0; i < n; ++i) {
                auto expected = a * x[i] + y[i];
                if (std::abs(out[i] - reference[i]) > 1e-12 || std::abs(out[i] - expected) > 1e-12)
                    throw std::runtime_error("CPU/GPU/reference mismatch at " + std::to_string(i));
            }
            if (out[n] != -9876 || reference[n] != -9876)
                throw std::runtime_error("Tail guard overwritten");

            std::cout << std::fixed << std::setprecision(6) << "n=" << n
                      << " h2d_ms=" << after.h2d_ms - before.h2d_ms
                      << " kernel_ms=" << after.kernel_ms - before.kernel_ms
                      << " d2h_ms=" << after.d2h_ms - before.d2h_ms << " total_ms=" << total
                      << " result=PASS\n";
        }
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

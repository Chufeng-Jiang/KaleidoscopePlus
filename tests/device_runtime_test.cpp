#include "kaleidoscope/runtime/DeviceRuntime.h"
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
static double element(double x, double y, double a) { return a * x + y; }
int main(int argc, char **argv) {
    KrtSession *raw = nullptr;
    std::string backend = argc > 1 ? argv[1] : "cpu";
    int created = krt_create(backend.c_str(), &raw);
    if (created == KRT_NO_DEVICE)
        return 77;
    if (created != KRT_OK)
        return 1;
    std::unique_ptr<KrtSession, decltype(&krt_destroy)> s(raw, krt_destroy);
    auto expect = [&](int actual, int expected) {
        if (actual != expected)
            throw std::runtime_error("Expected " + std::to_string(expected) + ", got " +
                                     std::to_string(actual) + ": " + krt_error(raw));
    };
    try {
        double x[4] = {1, 2, 3, 4}, y[4] = {2, 3, 4, 5}, out[4] = {};
        expect(krt_copyin(raw, x), KRT_NOT_FOUND);
        expect(krt_enter(raw, nullptr, 0), KRT_INVALID);
        expect(krt_enter(raw, x, sizeof x), KRT_OK);
        expect(krt_enter(raw, x, sizeof x), KRT_OK);
        KrtStats stats{};
        expect(krt_stats(raw, &stats), KRT_OK);
        if (stats.allocations != 1 || stats.live_allocations != 1)
            return 1;
        expect(krt_enter(raw, x, sizeof x - 8), KRT_INVALID);
        expect(krt_enter(raw, x + 1, 8), KRT_INVALID);
        expect(krt_enter(raw, y, sizeof y), KRT_OK);
        expect(krt_enter(raw, out, sizeof out), KRT_OK);
        expect(krt_copyout(raw, out), KRT_STATE);
        expect(krt_launch(raw, x, y, out, 2, 4), KRT_STATE);
        expect(krt_copyin(raw, x), KRT_OK);
        expect(krt_copyin(raw, y), KRT_OK);
        expect(krt_launch(raw, x, y, out, 2, 4), KRT_STATE); // No kernel.
        if (backend == "cuda") {
            expect(krt_load_ptx(raw, "/does-not-exist/kale.ptx"), KRT_DEVICE_ERROR);
            if (argc < 3)
                return 1;
            expect(krt_load_ptx(raw, argv[2]), KRT_OK);
            expect(krt_set_cpu_kernel(raw, element), KRT_INVALID);
        } else
            expect(krt_set_cpu_kernel(raw, element), KRT_OK);
        expect(krt_launch(raw, x, y, out, 2, 3), KRT_INVALID);
        expect(krt_launch(raw, x, y, out, 2, 4), KRT_OK);
        expect(krt_wait(raw), KRT_OK);
        expect(krt_copyout(raw, out), KRT_OK);
        for (int i = 0; i < 4; ++i)
            if (out[i] != 2 * x[i] + y[i])
                return 1;
        // In-place output is supported because each element reads before writing.
        expect(krt_launch(raw, x, y, y, 2, 4), KRT_OK);
        expect(krt_copyout(raw, y), KRT_OK);
        for (int i = 0; i < 4; ++i)
            if (y[i] != out[i])
                return 1;
        expect(krt_exit(raw, x), KRT_OK);
        expect(krt_exit(raw, x), KRT_NOT_FOUND);
        expect(krt_copyout(raw, x), KRT_NOT_FOUND);
        expect(krt_exit(raw, y), KRT_OK);
        expect(krt_exit(raw, out), KRT_OK);
        expect(krt_stats(raw, &stats), KRT_OK);
        if (stats.live_allocations != 0)
            return 1;
        // Destructor also owns cleanup when explicit exit is omitted.
        expect(krt_enter(raw, x, sizeof x), KRT_OK);
        std::cout << "Runtime lifecycle and negative tests passed: " << backend << '\n';
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

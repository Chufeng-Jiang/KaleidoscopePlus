#include "kaleidoscope/runtime/DeviceRuntime.h"
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
extern "C" double _mlir_ciface_vector(KaleArray *, KaleArray *, KaleArray *, double);
extern "C" int kacc_vector(KrtSession *, const KaleArray *, const KaleArray *, KaleArray *, double);
static KaleArray descriptor(std::vector<double> &v, size_t n) {
    return {v.data(), v.data(), 0, {static_cast<int64_t>(n)}, {1}};
}
int main(int argc, char **argv) {
    std::string backend = argc > 1 ? argv[1] : "cpu";
    KrtSession *raw = nullptr;
    int status = krt_create(backend.c_str(), &raw);
    if (status != KRT_OK)
        return status == KRT_NO_DEVICE ? 77 : 1;
    std::unique_ptr<KrtSession, decltype(&krt_destroy)> session(raw, krt_destroy);
    auto expect = [&](int result, int wanted = KRT_OK) {
        if (result != wanted)
            throw std::runtime_error(std::to_string(result) + ": " + krt_error(raw));
    };
    try {
        if (backend == "cuda") {
            if (argc < 3)
                throw std::runtime_error("Expected matching PTX artifact");
            expect(krt_load_ptx(raw, argv[2]));
        }
        std::cout << "teaching #acc backend=" << backend << " device=" << krt_device(raw) << '\n';
        for (size_t n : {size_t(0), size_t(1), size_t(17), size_t(257), size_t(100003)}) {
            std::vector<double> x(n + 1), y(n + 1), out(n + 1, -999), reference(n + 1, -999);
            for (size_t i = 0; i < n; ++i) {
                x[i] = (i % 97) * .125;
                y[i] = (i % 23) * .25;
            }
            auto dx = descriptor(x, n), dy = descriptor(y, n), dout = descriptor(out, n),
                 dr = descriptor(reference, n);
            _mlir_ciface_vector(&dx, &dy, &dr, 2.25);
            // No enter/copy/launch/wait/exit calls in user code.
            expect(kacc_vector(raw, &dx, &dy, &dout, 2.25));
            for (size_t i = 0; i < n; ++i)
                if (std::abs(out[i] - reference[i]) > 1e-12 || out[i] != 2.25 * x[i] + y[i])
                    throw std::runtime_error("Directive result mismatch");
            if (out[n] != -999)
                throw std::runtime_error("Tail guard overwritten");
            // Repeat the same scope: cleanup must have released all prior mappings.
            expect(kacc_vector(raw, &dx, &dy, &dout, 2.25));
            KrtStats stats{};
            expect(krt_stats(raw, &stats));
            if (stats.live_allocations)
                throw std::runtime_error("Scoped mapping leaked");
            std::cout << "n=" << n << " automatic-data-management=PASS\n";
        }
        std::vector<double> x(18, 1), y(18, 2), out(18, -999);
        auto dx = descriptor(x, 17), dy = descriptor(y, 17), dout = descriptor(out, 17);
        dy.sizes[0] = 16;
        expect(kacc_vector(raw, &dx, &dy, &dout, 2), KRT_INVALID);
        dy.sizes[0] = 17;
        dout.strides[0] = 2;
        expect(kacc_vector(raw, &dx, &dy, &dout, 2), KRT_INVALID);
        dout.strides[0] = 1;
        expect(kacc_vector(raw, &dx, &dy, &dx, 2), KRT_INVALID);
        expect(kacc_vector(raw, nullptr, &dy, &dout, 2), KRT_INVALID);
        expect(krt_enter(raw, x.data(), 17 * sizeof(double)));
        expect(kacc_vector(raw, &dx, &dy, &dout, 2), KRT_STATE);
        expect(krt_exit(raw, x.data()));
        // Failure after acquiring x and y must release both, retaining the
        // unrelated mapping that caused the out-range overlap.
        expect(krt_enter(raw, out.data() + 1, sizeof(double)));
        expect(kacc_vector(raw, &dx, &dy, &dout, 2), KRT_INVALID);
        if (std::string(krt_error(raw)).find("Overlapping") == std::string::npos)
            throw std::runtime_error("Cleanup lost original error");
        KrtStats stats{};
        expect(krt_stats(raw, &stats));
        if (stats.live_allocations != 1)
            throw std::runtime_error("Partial-enter cleanup failed");
        expect(krt_exit(raw, out.data() + 1));
        for (auto v : out)
            if (v != -999)
                throw std::runtime_error("Invalid scope modified output");
        expect(kacc_vector(raw, &dx, &dy, &dout, 2));
        std::cout << "descriptor validation and failure cleanup=PASS\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

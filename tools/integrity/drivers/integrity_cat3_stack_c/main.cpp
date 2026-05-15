// Stack C driver for cat3.cubic-kernel.
//
// Evaluates the cubic SPH kernel at command-line-specified (q, h) points
// and emits JSON to stdout. Built only when GPU_SIMS_BUILD_INTEGRITY_CAT3=ON.
//
// The kernel implementation is a literal transcription of the cubic
// spline formula from
// particle-fluids/sph-water/shaders/density_alpha.comp.glsl:72-82,
// which itself transcribes SPlisHSPlasH 2.16.1 SPHKernels.h:43-85.
// Any drift in this C++ transcription will surface as a tolerance
// failure when the Python check compares against the spec-formula
// expected values.
//
// Usage: integrity_cat3_stack_c [q h]...
// Output (stdout):
//   {"evaluations": [{"q":..., "h":..., "W":..., "gradW_magnitude":...}, ...]}

#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace {

constexpr double PI = 3.14159265358979323846;

double cubicW(double q, double h) {
    if (q > 1.0) return 0.0;
    const double norm = 8.0 / (PI * h * h * h);
    if (q <= 0.5) {
        return norm * (6.0 * q * q * q - 6.0 * q * q + 1.0);
    }
    return norm * 2.0 * (1.0 - q) * (1.0 - q) * (1.0 - q);
}

double cubicGradWMagnitude(double q, double h) {
    if (q > 1.0 || q == 0.0) return 0.0;
    const double norm = 8.0 / (PI * h * h * h * h);
    if (q <= 0.5) {
        return norm * std::fabs(18.0 * q * q - 12.0 * q);
    }
    return norm * 6.0 * (1.0 - q) * (1.0 - q);
}

}  // namespace

int main(int argc, char** argv) {
    if ((argc - 1) % 2 != 0) {
        std::fprintf(stderr, "usage: %s [q h]...\n", argv[0]);
        return 1;
    }

    std::printf("{\"evaluations\":[");
    bool first = true;
    for (int i = 1; i + 1 < argc; i += 2) {
        double q = std::atof(argv[i]);
        double h = std::atof(argv[i + 1]);
        double W = cubicW(q, h);
        double gradW = cubicGradWMagnitude(q, h);
        if (!first) std::printf(",");
        std::printf(
            "{\"q\":%.17g,\"h\":%.17g,\"W\":%.17g,\"gradW_magnitude\":%.17g}",
            q, h, W, gradW
        );
        first = false;
    }
    std::printf("]}\n");
    return 0;
}

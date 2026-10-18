#include "RobotPal/Util/bench.h"

#include <cmath>
#include <iostream>

namespace {

bool nearly_equal(double a, double b, double eps = 1e-9) {
    return std::fabs(a - b) < eps;
}

int run_all_tests() {
    {
        PerfStats stats;
        if (!nearly_equal(stats.last_ms, 0.0)) {
            std::cerr << "Expected initial last_ms == 0\n";
            return 1;
        }
        if (!nearly_equal(stats.avg_ms, 0.0)) {
            std::cerr << "Expected initial avg_ms == 0\n";
            return 1;
        }
        if (!nearly_equal(stats.max_ms, 0.0)) {
            std::cerr << "Expected initial max_ms == 0\n";
            return 1;
        }
        if (stats.count != 0) {
            std::cerr << "Expected initial count == 0\n";
            return 1;
        }
        if (!(stats.min_ms > 1e8)) {
            std::cerr << "Expected initial min_ms sentinel value\n";
            return 1;
        }
    }

    {
        PerfStats stats;
        stats.add(10.0);
        stats.add(20.0);
        stats.add(30.0);

        if (stats.count != 3) {
            std::cerr << "Expected count == 3\n";
            return 1;
        }
        if (!nearly_equal(stats.last_ms, 30.0)) {
            std::cerr << "Expected last_ms == 30\n";
            return 1;
        }
        if (!nearly_equal(stats.avg_ms, 20.0)) {
            std::cerr << "Expected avg_ms == 20\n";
            return 1;
        }
        if (!nearly_equal(stats.min_ms, 10.0)) {
            std::cerr << "Expected min_ms == 10\n";
            return 1;
        }
        if (!nearly_equal(stats.max_ms, 30.0)) {
            std::cerr << "Expected max_ms == 30\n";
            return 1;
        }
    }

    {
        PerfStats stats;
        stats.add(12.5);
        stats.add(7.0);
        stats.add(19.25);
        stats.add(7.0);

        const double expected_avg = (12.5 + 7.0 + 19.25 + 7.0) / 4.0;
        if (!nearly_equal(stats.avg_ms, expected_avg)) {
            std::cerr << "Expected stable online average calculation\n";
            return 1;
        }
        if (!nearly_equal(stats.min_ms, 7.0)) {
            std::cerr << "Expected duplicated minimum value to be preserved\n";
            return 1;
        }
        if (!nearly_equal(stats.max_ms, 19.25)) {
            std::cerr << "Expected max_ms == 19.25\n";
            return 1;
        }
    }

    std::cout << "perf_stats_test: all checks passed\n";
    return 0;
}

} // namespace

int main() {
    return run_all_tests();
}

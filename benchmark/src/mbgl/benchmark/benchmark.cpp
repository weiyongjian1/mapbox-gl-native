#include <mbgl/benchmark.hpp>
#include <mbgl/memory.benchmark.hpp>

#include <benchmark/benchmark.h>

#include <iostream>

namespace mbgl {

int runBenchmark(int argc, char* argv[]) {
    std::cout << std::endl << "CPU Benchmarks" << std::endl;
    ::benchmark::Initialize(&argc, argv);
    ::benchmark::RunSpecifiedBenchmarks();
    
    std::cout << "Memory Benchmarks" << std::endl;
    bool memoryPassed = benchmark::memoryFootprintBenchmark();

    return memoryPassed ? 0 : 1;
}

} // namespace mbgl

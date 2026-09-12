#include <gtest/gtest.h>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <memory>
#include <string>
#include "SharedPtr.hpp"
#include "SmartArraySequence.hpp"

// Инструмент учета динамической памяти
struct MemoryTracker {
    static inline size_t total_allocated_bytes = 0;
    static inline size_t allocation_count = 0;
    static inline bool active = false;

    static void start() {
        total_allocated_bytes = 0;
        allocation_count = 0;
        active = true;
    }

    static void stop() {
        active = false;
    }
};

void* operator new(std::size_t size) {
    if (MemoryTracker::active) {
        MemoryTracker::total_allocated_bytes += size;
        MemoryTracker::allocation_count++;
    }
    void* p = std::malloc(size);
    if (!p) throw std::bad_alloc();
    return p;
}

void operator delete(void* p) noexcept {
    std::free(p);
}

void operator delete(void* p, std::size_t) noexcept {
    std::free(p);
}

void* operator new[](std::size_t size) {
    if (MemoryTracker::active) {
        MemoryTracker::total_allocated_bytes += size;
        MemoryTracker::allocation_count++;
    }
    void* p = std::malloc(size);
    if (!p) throw std::bad_alloc();
    return p;
}

void operator delete[](void* p) noexcept {
    std::free(p);
}

void operator delete[](void* p, std::size_t) noexcept {
    std::free(p);
}


struct BenchmarkResult {
    std::string name;
    size_t stack_size;
    size_t alloc_count;
    size_t heap_bytes;
    double duration_us; // время в микросекундах
};

static void PrintBenchmarkTable(const std::string& title, size_t N, const SmartArraySequence<BenchmarkResult>& results) {
    std::cout << "\n======================================================================================================\n";
    std::cout << " BENCHMARK: " << title << " (N = " << N << " objects)\n";
    std::cout << "======================================================================================================\n";
    std::cout << std::left << std::setw(23) << "Approach"
              << std::right << std::setw(12) << "Stack Size"
              << std::setw(15) << "Alloc Count"
              << std::setw(18) << "Heap Memory"
              << std::setw(18) << "Time (us)"
              << std::setw(14) << "Time (ms)" << "\n";
    std::cout << "------------------------------------------------------------------------------------------------------\n";

    for (int i = 0; i < results.GetLength(); ++i) {
        const auto& r = results[i];
        std::cout << std::left << std::setw(23) << r.name
                  << std::right << std::setw(10) << r.stack_size << " B"
                  << std::setw(15) << r.alloc_count
                  << std::setw(16) << r.heap_bytes << " B"
                  << std::setw(18) << std::fixed << std::setprecision(1) << r.duration_us
                  << std::setw(14) << std::fixed << std::setprecision(3) << (r.duration_us / 1000.0) << "\n";
    }
    std::cout << "======================================================================================================\n\n";
}

// Защита от оптимизации мертвого кода компилятором (-O3)
volatile int g_sink = 0;

// Малое число аллокаций (N = 1000) с усреднением по 100 прогонам
TEST(BenchmarkTest, SmallAllocations) {
    const size_t N = 1000;
    const size_t ITERATIONS = 100;
    SmartArraySequence<BenchmarkResult> results;

    // 1. Raw Pointer
    {
        double total_time = 0;
        size_t recorded_allocs = 0, recorded_heap = 0;

        for (size_t iter = 0; iter < ITERATIONS; ++iter) {
            int** ptrs = new int*[N]; // буфер выделяется вне замера

            MemoryTracker::start();
            auto t0 = std::chrono::high_resolution_clock::now();

            for (size_t i = 0; i < N; ++i) {
                ptrs[i] = new int(static_cast<int>(i));
                g_sink += *ptrs[i];
            }
            for (size_t i = 0; i < N; ++i) {
                delete ptrs[i];
            }

            auto t1 = std::chrono::high_resolution_clock::now();
            MemoryTracker::stop();

            delete[] ptrs;
            total_time += std::chrono::duration<double, std::micro>(t1 - t0).count();
            if (iter == 0) {
                recorded_allocs = MemoryTracker::allocation_count;
                recorded_heap = MemoryTracker::total_allocated_bytes;
            }
        }
        results.Append(BenchmarkResult{"Raw Pointer", sizeof(int*), recorded_allocs, recorded_heap, total_time / ITERATIONS});
    }

    // 2. UniquePtr
    {
        double total_time = 0;
        size_t recorded_allocs = 0, recorded_heap = 0;

        for (size_t iter = 0; iter < ITERATIONS; ++iter) {
            SmartArraySequence<UniquePtr<int>> seq(N); // буфер выделяется ДО замера

            MemoryTracker::start();
            auto t0 = std::chrono::high_resolution_clock::now();

            for (size_t i = 0; i < N; ++i) {
                seq.Append(UniquePtr<int>(new int(static_cast<int>(i))));
                g_sink += *seq[static_cast<int>(i)];
            }
            seq.Clear();

            auto t1 = std::chrono::high_resolution_clock::now();
            MemoryTracker::stop();

            total_time += std::chrono::duration<double, std::micro>(t1 - t0).count();
            if (iter == 0) {
                recorded_allocs = MemoryTracker::allocation_count;
                recorded_heap = MemoryTracker::total_allocated_bytes;
            }
        }
        results.Append(BenchmarkResult{"Custom UniquePtr", sizeof(UniquePtr<int>), recorded_allocs, recorded_heap, total_time / ITERATIONS});
    }

    // 3. std::unique_ptr
    {
        double total_time = 0;
        size_t recorded_allocs = 0, recorded_heap = 0;

        for (size_t iter = 0; iter < ITERATIONS; ++iter) {
            SmartArraySequence<std::unique_ptr<int>> seq(N);

            MemoryTracker::start();
            auto t0 = std::chrono::high_resolution_clock::now();

            for (size_t i = 0; i < N; ++i) {
                seq.Append(std::unique_ptr<int>(new int(static_cast<int>(i))));
                g_sink += *seq[static_cast<int>(i)];
            }
            seq.Clear();

            auto t1 = std::chrono::high_resolution_clock::now();
            MemoryTracker::stop();

            total_time += std::chrono::duration<double, std::micro>(t1 - t0).count();
            if (iter == 0) {
                recorded_allocs = MemoryTracker::allocation_count;
                recorded_heap = MemoryTracker::total_allocated_bytes;
            }
        }
        results.Append(BenchmarkResult{"std::unique_ptr", sizeof(std::unique_ptr<int>), recorded_allocs, recorded_heap, total_time / ITERATIONS});
    }

    // 4. SharedPtr
    {
        double total_time = 0;
        size_t recorded_allocs = 0, recorded_heap = 0;

        for (size_t iter = 0; iter < ITERATIONS; ++iter) {
            SmartArraySequence<SharedPtr<int>> seq(N);

            MemoryTracker::start();
            auto t0 = std::chrono::high_resolution_clock::now();

            for (size_t i = 0; i < N; ++i) {
                seq.Append(SharedPtr<int>(new int(static_cast<int>(i))));
                g_sink += *seq[static_cast<int>(i)];
            }
            seq.Clear();

            auto t1 = std::chrono::high_resolution_clock::now();
            MemoryTracker::stop();

            total_time += std::chrono::duration<double, std::micro>(t1 - t0).count();
            if (iter == 0) {
                recorded_allocs = MemoryTracker::allocation_count;
                recorded_heap = MemoryTracker::total_allocated_bytes;
            }
        }
        results.Append(BenchmarkResult{"Custom SharedPtr", sizeof(SharedPtr<int>), recorded_allocs, recorded_heap, total_time / ITERATIONS});
    }

    // 5. std::shared_ptr
    {
        double total_time = 0;
        size_t recorded_allocs = 0, recorded_heap = 0;

        for (size_t iter = 0; iter < ITERATIONS; ++iter) {
            SmartArraySequence<std::shared_ptr<int>> seq(N);

            MemoryTracker::start();
            auto t0 = std::chrono::high_resolution_clock::now();

            for (size_t i = 0; i < N; ++i) {
                seq.Append(std::shared_ptr<int>(new int(static_cast<int>(i))));
                g_sink += *seq[static_cast<int>(i)];
            }
            seq.Clear();

            auto t1 = std::chrono::high_resolution_clock::now();
            MemoryTracker::stop();

            total_time += std::chrono::duration<double, std::micro>(t1 - t0).count();
            if (iter == 0) {
                recorded_allocs = MemoryTracker::allocation_count;
                recorded_heap = MemoryTracker::total_allocated_bytes;
            }
        }
        results.Append(BenchmarkResult{"std::shared_ptr (new)", sizeof(std::shared_ptr<int>), recorded_allocs, recorded_heap, total_time / ITERATIONS});
    }

    // 6. std::make_shared
    {
        double total_time = 0;
        size_t recorded_allocs = 0, recorded_heap = 0;

        for (size_t iter = 0; iter < ITERATIONS; ++iter) {
            SmartArraySequence<std::shared_ptr<int>> seq(N);

            MemoryTracker::start();
            auto t0 = std::chrono::high_resolution_clock::now();

            for (size_t i = 0; i < N; ++i) {
                seq.Append(std::make_shared<int>(static_cast<int>(i)));
                g_sink += *seq[static_cast<int>(i)];
            }
            seq.Clear();

            auto t1 = std::chrono::high_resolution_clock::now();
            MemoryTracker::stop();

            total_time += std::chrono::duration<double, std::micro>(t1 - t0).count();
            if (iter == 0) {
                recorded_allocs = MemoryTracker::allocation_count;
                recorded_heap = MemoryTracker::total_allocated_bytes;
            }
        }
        results.Append(BenchmarkResult{"std::make_shared", sizeof(std::shared_ptr<int>), recorded_allocs, recorded_heap, total_time / ITERATIONS});
    }

    PrintBenchmarkTable("Allocation & Deallocation Lifecycle (Small Scale)", N, results);
}

// Большое число аллокаций
TEST(BenchmarkTest, LargeAllocations) {
    const size_t N = 100000;
    SmartArraySequence<BenchmarkResult> results;

    // 1. Raw Pointer
    {
        int** ptrs = new int*[N];
        MemoryTracker::start();
        auto t0 = std::chrono::high_resolution_clock::now();

        for (size_t i = 0; i < N; ++i) {
            ptrs[i] = new int(static_cast<int>(i));
            g_sink += *ptrs[i];
        }
        for (size_t i = 0; i < N; ++i) {
            delete ptrs[i];
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        MemoryTracker::stop();
        delete[] ptrs;

        double us = std::chrono::duration<double, std::micro>(t1 - t0).count();
        results.Append(BenchmarkResult{"Raw Pointer", sizeof(int*), MemoryTracker::allocation_count, MemoryTracker::total_allocated_bytes, us});
    }

    // 2. UniquePtr
    {
        SmartArraySequence<UniquePtr<int>> seq(N);

        MemoryTracker::start();
        auto t0 = std::chrono::high_resolution_clock::now();

        for (size_t i = 0; i < N; ++i) {
            seq.Append(UniquePtr<int>(new int(static_cast<int>(i))));
            g_sink += *seq[static_cast<int>(i)];
        }
        seq.Clear();

        auto t1 = std::chrono::high_resolution_clock::now();
        MemoryTracker::stop();

        double us = std::chrono::duration<double, std::micro>(t1 - t0).count();
        results.Append(BenchmarkResult{"Custom UniquePtr", sizeof(UniquePtr<int>), MemoryTracker::allocation_count, MemoryTracker::total_allocated_bytes, us});
    }

    // 3. std::unique_ptr
    {
        SmartArraySequence<std::unique_ptr<int>> seq(N);

        MemoryTracker::start();
        auto t0 = std::chrono::high_resolution_clock::now();

        for (size_t i = 0; i < N; ++i) {
            seq.Append(std::unique_ptr<int>(new int(static_cast<int>(i))));
            g_sink += *seq[static_cast<int>(i)];
        }
        seq.Clear();

        auto t1 = std::chrono::high_resolution_clock::now();
        MemoryTracker::stop();

        double us = std::chrono::duration<double, std::micro>(t1 - t0).count();
        results.Append(BenchmarkResult{"std::unique_ptr", sizeof(std::unique_ptr<int>), MemoryTracker::allocation_count, MemoryTracker::total_allocated_bytes, us});
    }

    // 4. SharedPtr
    {
        SmartArraySequence<SharedPtr<int>> seq(N);

        MemoryTracker::start();
        auto t0 = std::chrono::high_resolution_clock::now();

        for (size_t i = 0; i < N; ++i) {
            seq.Append(SharedPtr<int>(new int(static_cast<int>(i))));
            g_sink += *seq[static_cast<int>(i)];
        }
        seq.Clear();

        auto t1 = std::chrono::high_resolution_clock::now();
        MemoryTracker::stop();

        double us = std::chrono::duration<double, std::micro>(t1 - t0).count();
        results.Append(BenchmarkResult{"Custom SharedPtr", sizeof(SharedPtr<int>), MemoryTracker::allocation_count, MemoryTracker::total_allocated_bytes, us});
    }

    // 5. std::shared_ptr
    {
        SmartArraySequence<std::shared_ptr<int>> seq(N);

        MemoryTracker::start();
        auto t0 = std::chrono::high_resolution_clock::now();

        for (size_t i = 0; i < N; ++i) {
            seq.Append(std::shared_ptr<int>(new int(static_cast<int>(i))));
            g_sink += *seq[static_cast<int>(i)];
        }
        seq.Clear();

        auto t1 = std::chrono::high_resolution_clock::now();
        MemoryTracker::stop();

        double us = std::chrono::duration<double, std::micro>(t1 - t0).count();
        results.Append(BenchmarkResult{"std::shared_ptr (new)", sizeof(std::shared_ptr<int>), MemoryTracker::allocation_count, MemoryTracker::total_allocated_bytes, us});
    }

    // 6. std::make_shared
    {
        SmartArraySequence<std::shared_ptr<int>> seq(N);

        MemoryTracker::start();
        auto t0 = std::chrono::high_resolution_clock::now();

        for (size_t i = 0; i < N; ++i) {
            seq.Append(std::make_shared<int>(static_cast<int>(i)));
            g_sink += *seq[static_cast<int>(i)];
        }
        seq.Clear();

        auto t1 = std::chrono::high_resolution_clock::now();
        MemoryTracker::stop();

        double us = std::chrono::duration<double, std::micro>(t1 - t0).count();
        results.Append(BenchmarkResult{"std::make_shared", sizeof(std::shared_ptr<int>), MemoryTracker::allocation_count, MemoryTracker::total_allocated_bytes, us});
    }

    PrintBenchmarkTable("Allocation & Deallocation Lifecycle (Large Scale)", N, results);
}

// Замер накладных расходов на копирование (Copying & Ref-Count Overhead)
TEST(BenchmarkTest, CopyingOverhead) {
    const size_t N = 100000;
    std::cout << "\n======================================================================================================\n";
    std::cout << " BENCHMARK: Copying & Ref-Count Overhead (N = " << N << " copies)\n";
    std::cout << "======================================================================================================\n";

    // 1. Raw pointer copy
    {
        int* raw = new int(42);
        SmartArraySequence<int*> seq(N);

        auto t0 = std::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < N; ++i) {
            seq.Append(raw);
            g_sink += *seq[static_cast<int>(i)];
        }
        auto t1 = std::chrono::high_resolution_clock::now();

        delete raw;
        double us = std::chrono::duration<double, std::micro>(t1 - t0).count();
        std::cout << std::left << std::setw(25) << "Raw Pointer Copy:"
                  << std::right << std::setw(10) << std::fixed << std::setprecision(1) << us << " us\n";
    }

    // 2. SharedPtr copy
    {
        SharedPtr<int> sp(new int(42));
        SmartArraySequence<SharedPtr<int>> seq(N);

        auto t0 = std::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < N; ++i) {
            seq.Append(sp); // копирование и увеличение счетчика
            g_sink += *seq[static_cast<int>(i)];
        }
        seq.Clear(); // декремент и освобождение копий
        auto t1 = std::chrono::high_resolution_clock::now();

        double us = std::chrono::duration<double, std::micro>(t1 - t0).count();
        std::cout << std::left << std::setw(25) << "Custom SharedPtr Copy:"
                  << std::right << std::setw(10) << std::fixed << std::setprecision(1) << us << " us  [Non-atomic]\n";
    }

    // 3. std::shared_ptr copy
    {
        std::shared_ptr<int> sp = std::make_shared<int>(42);
        SmartArraySequence<std::shared_ptr<int>> seq(N);

        auto t0 = std::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < N; ++i) {
            seq.Append(sp);
            g_sink += *seq[static_cast<int>(i)];
        }
        seq.Clear();
        auto t1 = std::chrono::high_resolution_clock::now();

        double us = std::chrono::duration<double, std::micro>(t1 - t0).count();
        std::cout << std::left << std::setw(25) << "std::shared_ptr Copy:"
                  << std::right << std::setw(10) << std::fixed << std::setprecision(1) << us << " us  [Atomic (std)]\n";
    }
    std::cout << "======================================================================================================\n\n";
}

#include "PCH.h"
#include "doctest.h"

#include <cstddef>
#include <cstdint>
#include <stdexcept>

#include "../Core/Memory/Memory.h"
#include "../STL.h"

TEST_SUITE("Memory") {
    TEST_CASE("Allocate returns an aligned pointer and tracks its lifetime") {
        constexpr std::size_t AllocationSize = 64;
        constexpr std::size_t Alignment = 64;

        const Memory::FMemoryStats Before = Memory::GetStats();

        void* Pointer = Memory::Allocate(AllocationSize, Alignment, Memory::EMemoryTag::UObject);
        const Memory::FMemoryStats During = Memory::GetStats();

        CHECK(Pointer != nullptr);
        CHECK_EQ(reinterpret_cast<std::uintptr_t>(Pointer) % Alignment, 0);
        CHECK_EQ(During.AllocatedBytes, Before.AllocatedBytes + AllocationSize);
        CHECK_EQ(During.ActiveAllocationCount, Before.ActiveAllocationCount + 1);
        CHECK_EQ(During.TotalAllocationCount, Before.TotalAllocationCount + 1);
        CHECK(During.PeakAllocatedBytes >= During.AllocatedBytes);

        Memory::Free(Pointer);

        const Memory::FMemoryStats After = Memory::GetStats();

        CHECK_EQ(After.AllocatedBytes, Before.AllocatedBytes);
        CHECK_EQ(After.ActiveAllocationCount, Before.ActiveAllocationCount);
        CHECK_EQ(After.TotalAllocationCount, Before.TotalAllocationCount + 1);
    }

    TEST_CASE("Allocate rejects a non-power-of-two alignment") {
        CHECK_THROWS_AS(
            Memory::Allocate(sizeof(int), 3, Memory::EMemoryTag::Container),
            std::invalid_argument);
    }

    TEST_CASE("TArray uses TEngineAllocator and releases its memory") {
        const Memory::FMemoryStats Before = Memory::GetStats();

        {
            TArray<int> Values;
            Values.reserve(16);
            Values.push_back(10);
            Values.push_back(20);

            const Memory::FMemoryStats During = Memory::GetStats();

            CHECK_EQ(Values.size(), 2);
            CHECK_EQ(Values[0], 10);
            CHECK_EQ(Values[1], 20);
            CHECK(During.AllocatedBytes > Before.AllocatedBytes);
            CHECK(During.ActiveAllocationCount > Before.ActiveAllocationCount);
            CHECK(During.TotalAllocationCount > Before.TotalAllocationCount);
        }

        const Memory::FMemoryStats After = Memory::GetStats();

        CHECK_EQ(After.AllocatedBytes, Before.AllocatedBytes);
        CHECK_EQ(After.ActiveAllocationCount, Before.ActiveAllocationCount);
        CHECK(After.TotalAllocationCount > Before.TotalAllocationCount);
    }
}

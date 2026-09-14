#include "PCH.h"
#include "UObject.h"
#include "Memory.h"

UObject::UObject()
	: Guid(FGuid::NewGuid()), Name(GetTypeInfo()->TypeName)
{
}

const FGuid& UObject::GetGuid() const
{
	return Guid;
}

FObjectHandle UObject::GetHandle() const
{
	return Handle;
}

void UObject::SetHandle(FObjectHandle InHandle)
{
	Handle = InHandle;
}

void UObject::RestoreGuid(const FGuid& InGuid)
{
	Guid = InGuid;
}

void* UObject::operator new(std::size_t Size)
{
    return Memory::Allocate(
        Size,
        alignof(std::max_align_t),
        Memory::EMemoryTag::UObject);
}

void UObject::operator delete(void* Ptr) noexcept
{
    Memory::Free(Ptr);
}

void* UObject::operator new(
    std::size_t Size,
    std::align_val_t Alignment)
{
    return Memory::Allocate(
        Size,
        static_cast<std::size_t>(Alignment),
        Memory::EMemoryTag::UObject);
}

void UObject::operator delete(
    void* Ptr,
    std::align_val_t) noexcept
{
    Memory::Free(Ptr);
}

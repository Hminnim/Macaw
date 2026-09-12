#include "pch.h"
#include "FName.h"
#include "city.h"

static constexpr uint32 FNameMaxBlockBits = 13;
static constexpr uint32 FNameBlockOffsetBits = 16;
static constexpr uint32 FNameMaxBlocks = 1 << FNameMaxBlockBits;
static constexpr uint32 FNameBlockOffsets = 1 << FNameBlockOffsetBits;

static constexpr uint32 EntryIdBits = FNameMaxBlockBits + FNameBlockOffsetBits;
static constexpr uint32 EntryIdMask = (1 << EntryIdBits) - 1;
static constexpr uint32 ProbeHashShift = EntryIdBits;
static constexpr uint32 ProbeHashMask = ~EntryIdMask;

// Unpacked FNameEntryId to Block and Offset
struct FNameEntryHandle
{
	uint32 Block = 0;
	uint32 Offset = 0;

	FNameEntryHandle(uint32 InBlock, uint32 InOffset) 
		: Block(InBlock), Offset(InOffset) {}
	FNameEntryHandle(FNameEntryId Id) 
		: Block(Id.ToUnstableInt() >> FNameBlockOffsetBits), Offset(Id.ToUnstableInt()& (FNameBlockOffsets - 1)) {}

	operator FNameEntryId() const
	{
		return FNameEntryId::FromUnstableInt(Block << FNameBlockOffsetBits | Offset);
	}

	explicit operator bool() const { return Block | Offset; }
};

// FNameSlot
// Hash and Id
struct FNameSlot
{
	FNameSlot() {}
	FNameSlot(FNameEntryId Value, uint32 ProbeHash) 
		: IdAndHash(Value.ToUnstableInt() | ProbeHash ){}

	FNameEntryId GetId() const { return FNameEntryId::FromUnstableInt(IdAndHash & EntryIdMask); }
	uint32 GetProbeHash() const { return IdAndHash & ProbeHashMask; }

	bool Used() const { return IdAndHash != 0;  }
private:
	uint32 IdAndHash = 0;
};

// FNameEntryAllocator
// Allocate memory to FNameEntry
class FNameEntryAllocator
{
public:
	enum { Stride = alignof(FNameEntry) };
	enum { BlockSizeBytes = Stride * FNameBlockOffsets};

	FNameEntryAllocator()
	{
		Blocks[0] = new uint8[BlockSizeBytes]();
		CurrentByteCursor = Stride;
	}

	~FNameEntryAllocator()
	{
		for (int32 Index = CurrentBlock; Index >= 0; --Index)
		{
			delete[] Blocks[Index];
		}
	}

	FNameEntryHandle Allocate(uint32 Bytes)
	{
		uint32 Step = (Bytes + Stride - 1) & ~(Stride - 1);

		if (CurrentByteCursor + Step > BlockSizeBytes)
		{
			AllocateNewBlock();
		}

		uint32 ByteOffset = CurrentByteCursor;
		CurrentByteCursor += Step;

		return FNameEntryHandle(CurrentBlock, ByteOffset / Stride);
	}

	FNameEntry& Resolve(FNameEntryHandle Handle) const
	{
		return *reinterpret_cast<FNameEntry*>(Blocks[Handle.Block] + Stride * Handle.Offset);
	}

	void AllocateNewBlock()
	{
		++CurrentBlock;
		CurrentByteCursor = 0;

		if (Blocks[CurrentBlock] == nullptr)
		{
			Blocks[CurrentBlock] = new uint8[BlockSizeBytes]();
		}
	}

private:
	uint32 CurrentBlock = 0;
	uint32 CurrentByteCursor = 0;
	uint8* Blocks[FNameMaxBlocks] = {};
};

// FNamePool

class FNamePool
{
public:
	static FNamePool& Get()
	{
		static FNamePool Instance;
		return Instance;
	}
	FNameEntryId Find(std::string_view NameString) const
	{
		uint32 FullHash = CalculateHash(NameString);
		uint32 CapacityMask = ComparisonHashBuckets.size() - 1;
		uint32 SlotIndex = FullHash & CapacityMask;
		uint32 ProbeHash = (FullHash << ProbeHashShift) & ProbeHashMask;

		while (ComparisonHashBuckets[SlotIndex].Used())
		{
			if (ComparisonHashBuckets[SlotIndex].GetProbeHash() == ProbeHash)
			{
				FNameEntryId ExistingId = ComparisonHashBuckets[SlotIndex].GetId();
				const FNameEntry& Entry = Resolve(ExistingId);

				std::string_view ExistingStr(Entry.GetName(), Entry.GetNameLength());
				if (ExistingStr.length() == NameString.length())
				{
					bool bIsMatch = true;
					for (size_t i = 0; i < ExistingStr.length(); ++i)
					{
						if (std::tolower(ExistingStr[i]) != std::tolower(NameString[i]))
						{
							bIsMatch = false;
							break;
						}
					}
					if (bIsMatch)
					{
						return ExistingId;
					}
				}				
			}
			SlotIndex = (SlotIndex + 1) & CapacityMask;
		}

		return FNameEntryId();
	}
	FNameEntryId Store(std::string_view NameString)
	{
		FNameEntryId ExistingId = FNamePool::Find(NameString);
		if (ExistingId.ToUnstableInt() != 0)
		{
			return ExistingId;
		}

		// Write Memory
		uint32 NeededByte = sizeof(FNameEntryHeader) + NameString.length() + 1; // 1 : null
		FNameEntryHandle NewHandle = Entries.Allocate(NeededByte);

		// Set header
		FNameEntry& NewEntry = Entries.Resolve(NewHandle);
		uint16* HeaderPtr = reinterpret_cast<uint16*>(&NewEntry);
		*HeaderPtr = static_cast<uint16>(NameString.length()) << 1;
		// Set string
		char* DataPtr = const_cast<char*>(NewEntry.GetName());
		std::memcpy(DataPtr, NameString.data(), NameString.length());
		DataPtr[NameString.length()] = '\0'; // null char

		uint32 FullHash = CalculateHash(NameString);
		uint32 CapacityMask = ComparisonHashBuckets.size() - 1;
		uint32 SlotIndex = FullHash & CapacityMask;
		uint32 ProbeHash = (FullHash << ProbeHashShift) & ProbeHashMask;

		while (ComparisonHashBuckets[SlotIndex].Used())
		{
			SlotIndex = (SlotIndex + 1) & CapacityMask;
		}
		
		ComparisonHashBuckets[SlotIndex] = FNameSlot(NewHandle, ProbeHash);

		return NewHandle;
	}
	const FNameEntry& Resolve(FNameEntryId Id) const
	{
		return Entries.Resolve(Id);
	}

private:
	FNamePool()
	{
		Initialize(8192);
	}

	void Initialize(uint32 InitialCapacity)
	{
		ComparisonHashBuckets.assign(InitialCapacity, FNameSlot());
	}

	// To Lower for Comparison
	uint32 CalculateHash(std::string_view NameString) const
	{
		// Stack buffer
		char LowerBuffer[NAME_SIZE];

		size_t Len = std::min(NameString.length(), size_t(NAME_SIZE - 1));

		for (size_t i = 0; i < Len; ++i)
		{
			LowerBuffer[i] = static_cast<char>(std::tolower(NameString[i]));
		}

		return CityHash32(LowerBuffer, Len);
	}

	FNameEntryAllocator Entries;

	TArray<FNameSlot> ComparisonHashBuckets;
	TArray<FNameSlot> DisplayHashBuckets;
};

FName::FName(const char* pStr)
{
	if (pStr)
	{
		ComparisonId = FNamePool::Get().Store(pStr);
	}
}

FName::FName(FString str)
{
	if (str.length() > 0)
	{
		ComparisonId = FNamePool::Get().Store(str);
	}
}

int32 FName::Compare(const FName& Rhs) const
{
	return 0;
}

bool FName::operator==(const FName& Rhs) const
{
	return this->ComparisonId == Rhs.ComparisonId;
}

FString FName::ToString() const
{
	if (ComparisonId.ToUnstableInt() == 0)
	{
		return FString("");
	}

	const FNameEntry& Entry = FNamePool::Get().Resolve(ComparisonId);

	return FString(Entry.GetName(), Entry.GetNameLength());
}

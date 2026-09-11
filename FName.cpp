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

// Unpacked FNameEntryId by BitMasking
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

	bool Used() const { return !IdAndHash;  }
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

class FNamePool
{
public:
	static FNamePool& Get()
	{
		static FNamePool Instance;
		return Instance;
	}

	FNameEntryId Store(std::string_view NameString)
	{

	}
	FNameEntryId FInd(std::string_view NameString) const
	{

	}
	const FNameEntry& Resolve(FNameEntryId Id) const
	{
		return Entries.Resolve(Id);
	}

private:
	FNameEntryAllocator Entries;

	TArray<FNameSlot> ComparisonHashBuckets;
	TArray<FNameSlot> DisplayHashBuckets;
};

FName::FName(char* pStr)
{
}

FName::FName(FString str)
{
}

int32 FName::Compare(const FName& Rhs) const
{
	return 0;
}

bool FName::operator==(const FName& Rhs) const
{
	return this->DisplayId == Rhs.ComparisonId;
}
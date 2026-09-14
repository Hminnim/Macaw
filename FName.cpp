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

// FNameHash
// Hash(uint32) and ProbeHash(uint32)
struct FNameHash
{
	uint32 Hash;
	uint32 ProbeHash;

	static uint64 GenerateHash(const char* Str, size_t Len)
	{
		return CityHash64(Str, Len);
	}

	FNameHash(const char* Str, int32 Len)
		: FNameHash(GenerateHash(Str, Len), Len)
	{}

	FNameHash()
		: Hash(0), ProbeHash(0)
	{}

	FNameHash(uint64 InHash, int32 Len)
	{
		uint32 Hi = static_cast<uint32>(InHash >> 32);
		uint32 Lo = static_cast<uint32>(InHash & 0xFFFFFFFF);

		Hash = Lo;
		ProbeHash = Hi & ProbeHashMask;
	}
};

// FNameValue
// Name(string_view), Hash(FNameHash)
struct FNameValue
{
	FNameValue(std::string_view InName)
		: Name(InName)
	{}

	FNameValue(std::string_view InName, FNameHash InHash)
		: Name(InName), Hash(InHash)
	{}

	std::string_view Name;
	FNameHash Hash;
};

// FNameComparisonValue
// Only for using lower(Not display)
struct FNameComparisonValue : public FNameValue
{
	FNameComparisonValue(std::string_view InName)
		: FNameValue(InName)
	{
		// Stack buffer
		char LowerBuffer[NAME_SIZE];

		size_t Len = std::min(InName.length(), size_t(NAME_SIZE - 1));

		for (size_t i = 0; i < Len; ++i)
		{
			LowerBuffer[i] = static_cast<char>(std::tolower(InName[i]));
		}

		Hash = FNameHash(LowerBuffer, static_cast<int32>(Len));
	}
};
// FNameDisplayValue
struct FNameDisplayValue : public FNameValue
{
	FNameDisplayValue(std::string_view InName)
		: FNameValue(InName)
	{
		Hash = FNameHash(InName.data(), static_cast<int32>(InName.length()));
	}
};

// FNamePool
// HashBuckets, Entries
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
		// Comparison
		FNameComparisonValue ComparisonValue(NameString);

		return FNamePool::FindValue(ComparisonHashBuckets, ComparisonValue, false);
	}
	void Store(std::string_view NameString, FNameEntryId& OutComparison, FNameEntryId& OutDisplay)
	{
		// Store Comparison
		FNameComparisonValue ComparisonValue(NameString);
		OutComparison = StoreValue(ComparisonHashBuckets, ComparisonValue, false);

		// Store Display
		FNameDisplayValue DisplayValue(NameString);
		OutDisplay = StoreValue(DisplayHashBuckets, DisplayValue, true);
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
		DisplayHashBuckets.assign(InitialCapacity, FNameSlot());
	}

	FNameEntryId FindValue(const TArray<FNameSlot>& Buckets, const FNameValue& InValue, bool bIsCaseSensitive) const
	{
		uint32 CapacityMask = static_cast<uint32>(Buckets.size() - 1);
		uint32 SlotIndex = InValue.Hash.Hash & CapacityMask;

		while (Buckets[SlotIndex].Used())
		{
			if (Buckets[SlotIndex].GetProbeHash() == InValue.Hash.ProbeHash)
			{
				FNameEntryId ExistingId = Buckets[SlotIndex].GetId();
				const FNameEntry& Entry = Resolve(ExistingId);

				const char* ExistingStr = Entry.GetName();
				if (Entry.GetNameLength() == InValue.Name.length())
				{
					bool bIsMatch = true;					
					if (bIsCaseSensitive)
					{
						bIsMatch = (std::memcmp(ExistingStr, InValue.Name.data(), InValue.Name.length()) == 0);
					}
					else
					{
						bIsMatch = (_strnicmp(ExistingStr, InValue.Name.data(), InValue.Name.length()) == 0);						
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

	FNameEntryId StoreValue(TArray<FNameSlot>& Buckets, const FNameValue& InValue, bool bIsCaseSensitive)
	{
		FNameEntryId ExistingId = FNamePool::FindValue(Buckets, InValue, bIsCaseSensitive);
		if (ExistingId.ToUnstableInt() != 0)
		{
			return ExistingId;
		}

		// Write Memory
		uint32 NeededByte = static_cast<uint32>(sizeof(FNameEntryHeader) + InValue.Name.length() + 1); // 1 : null terminator
		FNameEntryHandle NewHandle = Entries.Allocate(NeededByte);

		// Set header
		FNameEntry& NewEntry = Entries.Resolve(NewHandle);
		uint16* HeaderPtr = reinterpret_cast<uint16*>(&NewEntry);
		*HeaderPtr = static_cast<uint16>(InValue.Name.length()) << 1;
		// Set string
		char* DataPtr = const_cast<char*>(NewEntry.GetName());
		std::memcpy(DataPtr, InValue.Name.data(), InValue.Name.length());
		DataPtr[InValue.Name.length()] = '\0'; // null terminator

		uint32 CapacityMask = static_cast<uint32>(Buckets.size() - 1);
		uint32 SlotIndex = InValue.Hash.Hash & CapacityMask;

		while (Buckets[SlotIndex].Used())
		{
			SlotIndex = (SlotIndex + 1) & CapacityMask;
		}

		Buckets[SlotIndex] = FNameSlot(NewHandle, InValue.Hash.ProbeHash);

		return NewHandle;
	}

	FNameEntryAllocator Entries;

	TArray<FNameSlot> ComparisonHashBuckets;
	TArray<FNameSlot> DisplayHashBuckets;
};

FName::FName(const char* pStr)
{
	if (pStr)
	{
		FNamePool::Get().Store(pStr,ComparisonId, DisplayId);
	}
}

FName::FName(FString str)
{
	if (str.length() > 0)
	{
		FNamePool::Get().Store(str, ComparisonId, DisplayId);
	}
}

int32 FName::Compare(const FName& Rhs) const
{
	return this->ComparisonId.ToUnstableInt() - Rhs.ComparisonId.ToUnstableInt();
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

	const FNameEntry& Entry = FNamePool::Get().Resolve(DisplayId);

	return FString(Entry.GetName(), Entry.GetNameLength());
}

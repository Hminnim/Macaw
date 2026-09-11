#pragma once
#include "STL.h"
#include "Common.h"
#include <string_view>

// Max size of name, including the null terminator
enum {NAME_SIZE = 1024};

struct FNameEntryId
{
	constexpr FNameEntryId() : Value(0) {}
	
	constexpr uint32 ToUnstableInt() const { return Value; }
	static FNameEntryId FromUnstableInt(uint32 UnstableInt)
	{
		FNameEntryId Id;
		Id.Value = UnstableInt;
		return Id;
	}

	// operator
	bool operator==(const FNameEntryId& Rhs) const
	{
		return this->Value == Rhs.Value;
	}

	bool operator!=(const FNameEntryId& Rhs) const
	{
		return this->Value != Rhs.Value;
	}

private:
	uint32 Value;
};

struct FNameEntryHeader
{
	uint16 bIsWide : 1;
	uint16 Len = 15;
};

struct FNameEntry
{
private:
	FNameEntryHeader Header;
	uint8 NameData[0];

public:
	FNameEntry(const FNameEntry&) = delete;
	FNameEntry(FNameEntry&&) = delete;
	FNameEntry& operator=(const FNameEntry&) = delete;
	FNameEntry& operator=(FNameEntry&&) = delete;

	bool IsWide() const { return Header.bIsWide; }
	int32 GetNameLength() const { return Header.Len; }

	const char* GetName() const { return (char*)NameData; }
};

class FName
{
public:
	FName(char* pStr);
	FName(FString str);

	int32 Compare(const FName& Rhs) const;
	bool operator==(const FName& Rhs) const;

private:
	FNameEntryId DisplayId;
	FNameEntryId ComparisonId;
	int32 Number = 0;
};
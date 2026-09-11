#pragma once
#include "STL.h"
#include "Common.h"

struct FNameEntryID
{
	constexpr FNameEntryID() : Value(0) {}

private:
	uint32 Value;
};

class FName
{
public:
	FName(char* pStr);
	FName(FString str);

	int32 Compare(const FName& Rhs) const;
	bool operator==(const FName& Rhs) const;

private:
	int32 DisplayIndex;
	int32 ComparisonIndex;
	int32 Number;
};
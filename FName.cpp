#include "FName.h"

FName::FName(char* pStr)
{
}

FName::FName(FString str)
{
}

int32 FName::Compare(const FName& Rhs) const
{
	int32 A = this->ComparisonIndex;
	int32 B = Rhs.ComparisonIndex;

	return A - B;
}

bool FName::operator==(const FName& Rhs) const
{
	return this->ComparisonIndex == Rhs.ComparisonIndex;
}

class FNameEntryAllocator
{

};

class FNamePool
{
public:
	FNamePool();

private:

};


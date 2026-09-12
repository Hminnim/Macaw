#pragma once 

#include "Serialize/FArchive.h"

struct FAssetHandle {
	uint32 ID { std::numeric_limits<uint32>::max() };
	uint32 Generation { 0 };

	bool operator==(const FAssetHandle& Other) const {
		return ID == Other.ID && Generation == Other.Generation;
	}

	bool operator!=(const FAssetHandle& Other) const {
		return !(*this == Other);
	}

	operator bool() const {
		return ID != std::numeric_limits<uint32>::max();
	}

	void Serialize(FArchive& Archive);
};

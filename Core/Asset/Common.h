#pragma once 
#include "FAssetHandle.h"
#include "UAsset.h"

class IAssetQuery {
public:
	virtual ~IAssetQuery() = default;

public:
	virtual UAsset* GetUAsset(const FString& name) = 0;
	virtual FAssetHandle GetAsset(const FString& name) const = 0;
	virtual FAssetHandle GetAsset(const FGuid& ID) const = 0;

};
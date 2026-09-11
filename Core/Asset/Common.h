#pragma once 
#include "FAssetHandle.h"

class IAssetQuery{
public:
	virtual ~IAssetQuery() = default;

public:
	virtual FAssetHandle GetAsset(const FString& name) const = 0;
	virtual FAssetHandle GetAsset(const FGuid& ID) const = 0;

};
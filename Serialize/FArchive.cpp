#include "PCH.h"
#include "FArchive.h"

#include "../Core/Asset/FAssetRegistry.h"


FArchive::FArchive(EArchiveMode InMode) : Mode(InMode), AssetRegistry(nullptr) {
}

void FArchive::SetAssetRegistry(FAssetRegistry* InputAssetRegistry) {
	AssetRegistry = InputAssetRegistry;
}

FAssetRegistry* FArchive::GetAssetRegistry() {
	return AssetRegistry;
}

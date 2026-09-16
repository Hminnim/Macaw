#include "PCH.h"
#include "FMaterialChunkSignature.h"

bool FMaterialChunkSignatureBuilder::AddTexture(FAssetHandle TextureHandle) {
	if (Signature.TextureFieldCount >= MAX_MATERIAL_TEXTURE_FIELDS || !TextureHandle) {
		return false;
	}

	Signature.TextureHandles[Signature.TextureFieldCount] = TextureHandle;
	++Signature.TextureFieldCount;

	return true;
}

#include "PCH.h"
#include "FMaterialChunkSignature.h"

bool FMaterialChunkSignatureBuilder::AddTexture(FTextureLocation Location) {
	if (Signature.TextureFieldCount >= MAX_MATERIAL_TEXTURE_FIELDS || Location.ProfileId >= MAX_TEXTURE_PROFILE_COUNT) {
		return false;
	}

	const uint32 Shift = static_cast<uint32>(Signature.TextureFieldCount) * MATERIAL_CHUNK_PROFILE_ID_BITS;
	Signature.PackedProfileIds |= static_cast<uint64>(Location.ProfileId) << Shift;
	++Signature.TextureFieldCount;

	return true;
}

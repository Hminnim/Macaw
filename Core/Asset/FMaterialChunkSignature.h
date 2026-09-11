#pragma once

#include "FTextureProfile.h"

#include <functional>

inline constexpr uint8 MAX_MATERIAL_TEXTURE_FIELDS = 8;
inline constexpr uint8 MATERIAL_CHUNK_PROFILE_ID_BITS = 8;

static_assert(MAX_TEXTURE_PROFILE_COUNT <= (1u << MATERIAL_CHUNK_PROFILE_ID_BITS));

struct FMaterialChunkSignature {
	uint64 PackedProfileIds{ 0 };
	uint8 TextureFieldCount{ 0 };

	bool IsValid() const {
		return TextureFieldCount <= MAX_MATERIAL_TEXTURE_FIELDS;
	}

	uint16 GetProfileId(uint8 TextureFieldIndex) const {
		if (TextureFieldIndex >= TextureFieldCount) {
			return 0;
		}

		const uint32 Shift = static_cast<uint32>(TextureFieldIndex) * MATERIAL_CHUNK_PROFILE_ID_BITS;
		return static_cast<uint16>((PackedProfileIds >> Shift) & 0xffu);
	}

	size_t GetHash() const noexcept {
		size_t Hash = std::hash<uint64>{}(PackedProfileIds);
		return Hash ^ (std::hash<uint8>{}(TextureFieldCount) + static_cast<size_t>(0x9e3779b9u) + (Hash << 6) + (Hash >> 2));
	}

	bool operator==(const FMaterialChunkSignature& Other) const = default;
	bool operator!=(const FMaterialChunkSignature& Other) const = default;
};

class FMaterialChunkSignatureBuilder {
public:
	bool AddTexture(FTextureLocation Location);

	FMaterialChunkSignature Build() const { return Signature; }
	void Reset() { Signature = {}; }

private:
	FMaterialChunkSignature Signature{};
};

namespace std {
	template<>
	struct hash<FMaterialChunkSignature> {
		size_t operator()(const FMaterialChunkSignature& Signature) const noexcept {
			return Signature.GetHash();
		}
	};
}

#pragma once

#include "FAssetHandle.h"

#include <array>
#include <functional>

inline constexpr uint8 MAX_MATERIAL_TEXTURE_FIELDS = 8;

struct FMaterialChunkSignature {
	std::array<FAssetHandle, MAX_MATERIAL_TEXTURE_FIELDS> TextureHandles{};
	uint8 TextureFieldCount{ 0 };

	bool IsValid() const {
		return TextureFieldCount <= MAX_MATERIAL_TEXTURE_FIELDS;
	}

	FAssetHandle GetTextureHandle(uint8 TextureFieldIndex) const {
		if (TextureFieldIndex >= TextureFieldCount) {
			return {};
		}

		return TextureHandles[TextureFieldIndex];
	}

	size_t GetHash() const noexcept {
		size_t Hash = std::hash<uint8>{}(TextureFieldCount);

		for (uint8 TextureFieldIndex = 0; TextureFieldIndex < TextureFieldCount; ++TextureFieldIndex) {
			const FAssetHandle Handle = TextureHandles[TextureFieldIndex];
			Hash ^= std::hash<uint32>{}(Handle.ID) + static_cast<size_t>(0x9e3779b9u) + (Hash << 6) + (Hash >> 2);
			Hash ^= std::hash<uint32>{}(Handle.Generation) + static_cast<size_t>(0x9e3779b9u) + (Hash << 6) + (Hash >> 2);
		}

		return Hash;
	}

	bool operator==(const FMaterialChunkSignature& Other) const = default;
	bool operator!=(const FMaterialChunkSignature& Other) const = default;
};

class FMaterialChunkSignatureBuilder {
public:
	bool AddTexture(FAssetHandle TextureHandle);

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

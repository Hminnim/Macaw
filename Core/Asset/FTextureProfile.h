#pragma once

#include "Common.h"

#include <dxgiformat.h>

#include <cstddef>
#include <functional>


inline constexpr uint32 MAX_TEXTURE_PROFILE_COUNT = 16;


struct FTextureProfile {
	uint32 Width{ 0 };
	uint32 Height{ 0 };
	DXGI_FORMAT Format{ DXGI_FORMAT_UNKNOWN };

	bool IsValid() const {
		return Width != 0
			&& Height != 0
			&& Format != DXGI_FORMAT_UNKNOWN;
	}

	size_t GetHash() const noexcept {
		size_t Hash = std::hash<uint32>{}(Width);
		Hash = HashCombine(Hash, Height);
		return HashCombine(Hash, static_cast<uint32>(Format));
	}

	bool operator==(const FTextureProfile& Other) const = default;
	bool operator!=(const FTextureProfile& Other) const = default;

private:
	static size_t HashCombine(size_t Seed, uint32 Value) noexcept {
		return Seed ^ (std::hash<uint32>{}(Value) + static_cast<size_t>(0x9e3779b9u) + (Seed << 6) + (Seed >> 2));
	}
};

namespace std {
	template<>
	struct hash<FTextureProfile> {
		size_t operator()(const FTextureProfile& Profile) const noexcept {
			return Profile.GetHash();
		}
	};
}

// Typed CPU representation of a texture-array location.
struct FTextureLocation {
	uint16 ProfileId{ 0 };
	uint16 SliceId{ 0 };

	bool operator==(const FTextureLocation& Other) const = default;
	bool operator!=(const FTextureLocation& Other) const = default;
};

using FPackedTextureLocation = uint32;

static_assert(sizeof(FTextureLocation) == sizeof(FPackedTextureLocation), "FTextureLocation must remain a 16-bit profile id plus a 16-bit slice id.");

constexpr FPackedTextureLocation PackTextureLocation(FTextureLocation Location) {
	return (static_cast<FPackedTextureLocation>(Location.ProfileId) << 16) | static_cast<FPackedTextureLocation>(Location.SliceId);
}

constexpr FTextureLocation UnpackTextureLocation(FPackedTextureLocation PackedLocation) {
	return { static_cast<uint16>(PackedLocation >> 16), static_cast<uint16>(PackedLocation & 0xffffu) };
}

constexpr uint16 GetTextureProfileId(FPackedTextureLocation PackedLocation) {
	return UnpackTextureLocation(PackedLocation).ProfileId;
}

constexpr uint16 GetTextureSliceId(FPackedTextureLocation PackedLocation) {
	return UnpackTextureLocation(PackedLocation).SliceId;
}


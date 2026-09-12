#pragma once
#include "UAsset.h"
#include "FAssetHandle.h"
#include "../../FVector.h"
#include <array>
#include <cstdint>

struct FFontCharacter
{
	uint32_t StartU = 0;
	uint32_t StartV = 0;

	uint32_t USize = 0;
	uint32_t VSize = 0;

	float Advance = 0.0f;
};

struct FFontUVRect
{
	FVector2 UVMin{};
	FVector2 UVMax{};
};

class UFont : public UAsset
{
private:
	float AtlasHeight = 0.0f;
	float AtlasWidth = 0.0f;
	static constexpr uint32_t ASCIICharacterCount = 256;
	std::array<FFontCharacter, ASCIICharacterCount> Characters{};
	FAssetHandle AtlasTextureHandle{};
public:
	UFont() = default;
	~UFont() override = default;

	JG_DECLARE_DERIVED_TYPEINFO(UFont, UAsset);

	void BuildFixedGrid(FAssetHandle InAtlasTextureHandle, uint32_t InAtlasWidth, uint32_t InAtlasHeight, uint32_t InCellWidth, uint32_t InCellHeight, uint8_t InFirstCharacter, uint8_t InLastCharacter, float InDefaultAdvance);
	FFontCharacter* FindCharacter(uint8_t Character);
	FFontUVRect GetUV(uint8_t Character);
	FAssetHandle GetAtlasTextureHandle(){ return AtlasTextureHandle; }
};
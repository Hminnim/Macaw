#include "PCH.h"
#include "Core/Asset/UFont.h"

void UFont::BuildFixedGrid(FAssetHandle InAtlasTextureHandle, uint32_t InAtlasWidth, uint32_t InAtlasHeight, uint32_t InCellWidth, uint32_t InCellHeight, uint8_t InFirstCharacter, uint8_t InLastCharacter, float InDefaultAdvance)
{
	if (InAtlasHeight == 0 || InAtlasWidth == 0 || InCellHeight == 0 || InCellWidth == 0)
	{
		return;
	}
	if (InFirstCharacter > InLastCharacter || InLastCharacter >= ASCIICharacterCount)
	{
		return;
	}
	if (InDefaultAdvance <= 0.0f)
	{
		return;
	}

	AtlasTextureHandle = InAtlasTextureHandle;
	AtlasHeight = InAtlasHeight;
	AtlasWidth = InAtlasWidth;

	uint32_t RowCount = static_cast<uint32_t>(AtlasHeight / InCellHeight);
	uint32_t ColumnCount = static_cast<uint32_t>(AtlasWidth / InCellWidth);

	for (uint32_t Character = InFirstCharacter; Character <= InLastCharacter; Character++)
	{
		uint32_t AtlasIndex = Character - InFirstCharacter;
		uint32_t RowIndex = AtlasIndex / ColumnCount;
		uint32_t ColumnIndex = AtlasIndex % ColumnCount;
		uint32_t StartU = ColumnIndex * InCellWidth; // 픽셀좌표
		uint32_t StartV = RowIndex * InCellHeight;   // 픽셀 좌표
		if (StartU + InCellWidth > InAtlasWidth || StartV + InCellHeight > InAtlasHeight)
		{
			continue;
		}
		Characters[Character] = {
			.StartU = StartU,
			.StartV = StartV,
			.USize = InCellWidth,
			.VSize = InCellHeight,
			.Advance = InDefaultAdvance
		};
	}
}
FFontCharacter* UFont::FindCharacter(uint8_t Character)
{
	if (Character >= ASCIICharacterCount)
	{
		return nullptr;
	}
	FFontCharacter& Result = Characters[Character];

	if (Result.USize == 0 || Result.VSize == 0)
	{
		return nullptr;
	}

	return &Result;
}
FFontUVRect UFont::GetUV(uint8_t Character)
{
	FFontCharacter* FontCharacter = FindCharacter(Character);
	if (FontCharacter == nullptr)
	{
		FontCharacter = FindCharacter(static_cast<std::uint8_t>('?'));
	}
	if (FontCharacter == nullptr || AtlasHeight == 0 || AtlasWidth == 0)
	{
		return {};
	}
	FFontUVRect Result{};
	Result.UVMin.x = (static_cast<float>(FontCharacter->StartU)) / static_cast<float>(AtlasWidth);
	Result.UVMin.y = (static_cast<float>(FontCharacter->StartV)) / static_cast<float>(AtlasHeight);
	Result.UVMax.x = (static_cast<float>(FontCharacter->StartU + FontCharacter->USize)) / static_cast<float>(AtlasWidth);
	Result.UVMax.y = (static_cast<float>(FontCharacter->StartV + FontCharacter->VSize)) / static_cast<float>(AtlasHeight);
	return Result;
}
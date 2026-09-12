#include "Core/Asset/UFont.h"
#include "PCH.h"

void UFont::BuildFixedGrid(uint32_t InAtlasWidth, uint32_t InAtlasHeight, uint32_t InCellWidth, uint32_t InCellHeight, uint8_t InFirstCharacter, uint8_t InLastCharacter)
{
	if (InAtlasHeight == 0 || InAtlasWidth == 0)
	{
		return;
	}

	AtlasHeight = InAtlasHeight;
	AtlasWidth = InAtlasWidth;

	uint32_t RowCount = AtlasHeight / InCellHeight;
	uint32_t ColumnCount = AtlasWidth / InCellWidth;

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
			.VSize = InCellHeight
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
	FFontUVRect Result;
	float UMin = FontCharacter->StartU / AtlasWidth;
	float VMin = FontCharacter->StartV / AtlasHeight;
	float UMax = UMin + FontCharacter->USize / AtlasWidth;
	float VMax = VMin + FontCharacter->VSize / AtlasHeight;
	Result.UVMax.x = UMax;
	Result.UVMax.y = VMax;
	Result.UVMin.x = UMin;
	Result.UVMin.y = VMin;
	return Result;
}
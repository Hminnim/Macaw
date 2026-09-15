#pragma once

#include "STL.h"
#include "Core/Base/TypeInfo.h"
#include "UFont.h"
#include "FMath.h"
#include <d3d11.h>
#include <wrl/client.h>
#include <ft2build.h>
#include FT_FREETYPE_H

class UFreeTypeFont : public UFont
{
private:
    // FreeType 시스템 핸들
    FT_Library Library = nullptr;
    // 현재 로드된 실제 폰트 파일 하나
    FT_Face Face = nullptr;
    // Unicode 문자 -> GlyphIndex 매핑
    TMap<char32_t, uint32_t> CodePointToGlyphIndex;
    // GlyphIndex -> 렌더링 가능한 상태인가?(비트맵으로 저장되어 있는가?)
    TMap<uint32_t, FFontGlyph> GlyphCache;
    // 현재 Font Face 전체에 공통되는 수직 배치 정보
    FFontMetrics FontMetrics{};
    // AtlasWidth * AtlasHeight 사이즈의 배열로 2차원 픽셀 좌표의 색값을 1차원 배열로 기록해 해당 픽셀에 글씨가 있는지 없는지 판단
    TArray<uint8_t> AtlasPixels;
    // 빈 Atlas 텍스처 너비
    uint32_t AtlasWidth = 0;
    // 빈 Atlas 텍스처 높이
    uint32_t AtlasHeight = 0; 
    // 현재 Atlas 행에서 다음 glyph가 배치될 X 시작 위치
    uint32_t NextAtlasX = 0;
    // 현재 glyph를 배치하고 있는 Atlas 행의 Y 시작 위치
    uint32_t NextAtlasY = 0;
    // 현재 행에서 가장 높은 Bitmap 높이
    uint32_t CurrentRowHeight = 0;
    // Atlas에서 Glyph 상하좌우 패딩
    static constexpr uint32_t AtlasPadding = 1;
    // CPU Atlas가 변경되었음 -> GPU Texture도 갱신해야함
    bool bAtlasDirty = false;
    bool bInitialized = false;

    // GPU에 존재하는 실제 텍스처
    Microsoft::WRL::ComPtr<ID3D11Texture2D> AtlasTexture;
    // 셰이더가 AtlasTexture를 읽기 위한 View
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> AtlasSRV;

    void Reset();
    bool CreateAtlasTexture(ID3D11Device* Device);

public:
	JG_DECLARE_DERIVED_TYPEINFO(UFreeTypeFont, UFont);

	UFreeTypeFont() = default;
	~UFreeTypeFont() override;

    void Initialize(ID3D11Device* Device, const std::filesystem::path& MetadataPath) override;
    const FFontGlyph* GetOrCreateGlyph(char32_t CodePoint) override;
    const FFontMetrics& GetFontMetrics() const override;
    void FlushAtlas(ID3D11DeviceContext* Context) override;
    ID3D11ShaderResourceView* GetAtlasSRV() const override { return AtlasSRV.Get(); }
    const FFontGlyph* FindGlyph(char32_t CodePoint) const;

    bool AllocateAtlasRect(uint32 BitmapWidth,uint32 BitmapHeight, uint32& OutAtlasX, uint32& OutAtlasY);
    bool CopyBitmapToAtlas(FT_Bitmap& Bitmap, uint32 AtlasX, uint32 AtlasY);
};
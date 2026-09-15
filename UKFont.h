#pragma once

#include "STL.h"
#include "Core/Base/TypeInfo.h"
#include "Core/Asset/UFont.h"
#include "FMath.h"
#include <d3d11.h>
#include <wrl/client.h>
#include <ft2build.h>
#include FT_FREETYPE_H

struct FKGlyph
{
    // 현재 폰트에서 이 Glyph를 식별하는 번호
    uint32_t GlyphIndex = 0;
    // 이 Glyph의 Bitmap이 저장된 Atlas 텍스처 픽셀 좌상단 위치
    uint32_t AtlasX = 0; 
    uint32_t AtlasY = 0;
    // 이 Glyph의 Bitmap 사이즈
    uint32_t BitmapWidth = 0;
    uint32_t BitmapHeight = 0;
    // 로컬 좌표 할때 사용하는 값들
    // 이 Glyhp를 배치할때 현재 PenX 위치에 얼마나 더해야하는가?
    int32_t BearingX = 0;
    // 이 Glyhp를 배치할때 현재 BaseLine(PenY) 위치에 얼마나 더해야하는가?
    int32_t BearingY = 0;
    // 현재 Glyph를 처리한 뒤 다음 Glyph의 PenX 위치를 얼마나 이동시킬것인가?
    float AdvanceX = 0.0f;
    // BaseLine(PenY)는 LineHeight 만큼 옮기면 되므로 잘 쓰지 않는값. 세로쓰기때 좋음
    float AdvanceY = 0.0f;
    // 해당 Bitmap의 Atlas 텍스쳐 UV 좌상단, 우하단 좌표
    FVector2 UVMin{};
    FVector2 UVMax{};
    // 이 Glyph 데이터가 정상적으로 만들어진 것인가? (GlyphIndex != 0)
    bool bValid = false;
};
// 현재 Font Face + Font Size 전체에 공통되는 값
struct FKFontMetrics
{
    // 폰트 픽셀 크기
    float BakePixelHeight = 0.0f;
    // 텍스트 박스 상하 범위
    // Baseline 기준으로 폰트가 위쪽으로 올라갈 수 있는 대표적인 높이
    float Ascender = 0.0f;
    // Baseline 에서 아래쪽으로 내려가는 영역
    float Descender = 0.0f;
    // 한줄의 Baseline에서 다음 줄의 Baseline까지 이동해야하는 거리
    float LineHeight = 0.0f;
};

class UKFont : public UFont
{
private:
    // FreeType 시스템 핸들
    FT_Library Library = nullptr;
    // 현재 로드된 실제 폰트 파일 하나
    FT_Face Face = nullptr;
    // Unicode 문자 -> GlyphIndex 매핑
    TMap<char32_t, uint32_t> CodePointToGlyphIndex;
    // GlyphIndex -> 렌더링 가능한 상태인가?(비트맵으로 저장되어 있는가?)
    TMap<uint32_t, FKGlyph> GlyphCache;
    // 현재 Font Face 전체에 공통되는 수직 배치 정보
    FKFontMetrics FontMetrics{};
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
	JG_DECLARE_DERIVED_TYPEINFO(UKFont, UFont);

	UKFont() = default;
	~UKFont() override;

	void Initialize(ID3D11Device* Device, const std::filesystem::path& MetaDataPath) override;
    // CPU Atlas가 바뀌면 GPU에 복사
    void FlushAtlas(ID3D11DeviceContext* Context) override;
    ID3D11ShaderResourceView* GetRuntimeAtlasSRV() const override { return AtlasSRV.Get(); }

	const FKGlyph* FindGlyph(char32_t CodePoint) const;
    const FKGlyph* GetOrCreateGlyph(char32_t CodePoint);
    const FKFontMetrics& GetFontMetrics() const;
    bool AllocateAtlasRect(uint32_t BitmapWidth, uint32_t BitmapHeight, uint32_t& OutAtlasX, uint32_t& OutAtlasY);
    bool CopyBitmapToAtlas(FT_Bitmap& Bitmap, uint32_t AtlasX, uint32_t AtlasY);
};
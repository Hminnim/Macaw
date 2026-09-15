#pragma once
#include "UAsset.h"
#include "FAssetHandle.h"
#include "../../FVector.h"
#include <array>
#include <cstdint>

struct ID3D11DeviceContext;
struct ID3D11ShaderResourceView;

struct FFontGlyph
{
    uint32 GlyphIndex = 0;

    uint32 AtlasX = 0;
    uint32 AtlasY = 0;
    uint32 BitmapWidth = 0;
    uint32 BitmapHeight = 0;

    int32 BearingX = 0;
    int32 BearingY = 0;
    float AdvanceX = 0.0f;
    float AdvanceY = 0.0f;

    FVector2 UVMin{};
    FVector2 UVMax{};
};

struct FFontMetrics
{
    float BakePixelHeight = 0.0f;
    float Ascender = 0.0f;
    float Descender = 0.0f;
    float LineHeight = 0.0f;
};

class UFont : public UAsset
{
public:
    UFont() = default;
    ~UFont() override = default;

    JG_DECLARE_ABSTRACT_DERIVED_TYPEINFO(UFont, UAsset)

    virtual const FFontGlyph* GetOrCreateGlyph(char32_t CodePoint) = 0;
    virtual const FFontMetrics& GetFontMetrics() const = 0;
    virtual void FlushAtlas(ID3D11DeviceContext* Context) = 0;
    virtual ID3D11ShaderResourceView* GetAtlasSRV() const = 0;
};
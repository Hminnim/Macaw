#include "PCH.h"
#include "Scene/Component/UKTextRenderComponent.h"

#include "UKFont.h"

#include "Scene/AActor.h"
#include "Scene/UWorld.h"
#include "Core/Asset/FAssetRegistry.h"

#include <Windows.h>

namespace
{
    bool DecodeKoreanUTF8(const FString& Text, TArray<char32_t>& OutCodePoints)
    {
        OutCodePoints.clear();

        if (Text.empty())
        {
            return true;
        }

        const int WideLength = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, Text.data(), static_cast<int>(Text.size()), nullptr, 0);

        if (WideLength <= 0)
        {
            return false;
        }

        std::wstring WideText;
        WideText.resize(WideLength);

        const int ConvertedLength = MultiByteToWideChar( CP_UTF8, MB_ERR_INVALID_CHARS, Text.data(), static_cast<int>(Text.size()), WideText.data(), WideLength);

        if (ConvertedLength != WideLength)
        {
            return false;
        }
        // 현대 한글 U+AC00~U+D7A3은 UTF-16 한 칸에 들어간다.
        for (wchar_t Character : WideText)
        {
            OutCodePoints.push_back(static_cast<char32_t>(Character));
        }
        return true;
    }
}

void UKTextRenderComponent::RebuildTextGeometry()
{
    Vertices.clear();

    if (!FontHandle || Text.empty() || CharacterHeight <= 0.0f)
    {
        return;
    }

    AActor* Owner = GetOwner();

    if (Owner == nullptr || Owner->GetWorld() == nullptr)
    {
        return;
    }

    FAssetRegistry* AssetRegistry = Owner->GetWorld()->GetAssetRegistry();

    if (AssetRegistry == nullptr)
    {
        return;
    }

    UKFont* Font = AssetRegistry->ResolveAsset<UKFont>(FontHandle);

    if (Font == nullptr)
    {
        return;
    }

    const FKFontMetrics& Metrics = Font->GetFontMetrics();

    if (Metrics.LineHeight <= 0.0f)
    {
        return;
    }

    TArray<char32_t> CodePoints;

    if (!DecodeKoreanUTF8(Text, CodePoints))
    {
        return;
    }

    /*
     * FreeType 픽셀 좌표를 World 좌표로 변환하는 비율.
     */
    const float PixelToWorld = CharacterHeight / Metrics.LineHeight;

    float PenX = 0.0f;
    float BaselineY = 0.0f;

    for (char32_t CodePoint : CodePoints)
    {
        if (CodePoint == U'\r')
        {
            continue;
        }

        if (CodePoint == U'\n')
        {
            PenX = 0.0f;
            BaselineY -= CharacterHeight + LineSpacing;

            continue;
        }

        const FKGlyph* Glyph = Font->GetOrCreateGlyph(CodePoint);

        if (Glyph == nullptr)
        {
            Glyph = Font->GetOrCreateGlyph(U'\uFFFD');
        }

        if (Glyph == nullptr)
        {
            continue;
        }

        // 공백은 Bitmap이 없으므로 Vertex를 만들지 않는다.하지만 아래에서 AdvanceX는 적용한다.
        if (Glyph->BitmapWidth > 0 && Glyph->BitmapHeight > 0)
        {
            FTextVertex Vertex{};
            // Shader가 LocalPosition을 Glyph Quad의 왼쪽 위 좌표로 사용한다.
            Vertex.LocalPosition.x = PenX + static_cast<float>(Glyph->BearingX) * PixelToWorld;
            Vertex.LocalPosition.y = BaselineY + static_cast<float>(Glyph->BearingY) * PixelToWorld;
            Vertex.Size.x = static_cast<float>(Glyph->BitmapWidth) * PixelToWorld;
            Vertex.Size.y = static_cast<float>( Glyph->BitmapHeight) * PixelToWorld;
            Vertex.UVMin = Glyph->UVMin;
            Vertex.UVMax = Glyph->UVMax;

            Vertices.push_back(Vertex);
        }
        PenX += Glyph->AdvanceX * PixelToWorld + LetterSpacing;
    }
    if (Vertices.empty())
    {
        return;
    }
    // 셰이더가 사용하는 실제 Glyph Quad들의 경계로 텍스트 중심을 계산한다.
    // FreeType의 Bearing 때문에 첫 글자의 Left/Top이 0이라는 보장이 없다.
    const FTextVertex& FirstVertex = Vertices.front();
    float MinLeft = FirstVertex.LocalPosition.x;
    float MaxRight = FirstVertex.LocalPosition.x + FirstVertex.Size.x;
    float MaxTop = FirstVertex.LocalPosition.y;
    float MinBottom = FirstVertex.LocalPosition.y - FirstVertex.Size.y;

    for (const FTextVertex& Vertex : Vertices)
    {
        MinLeft = std::min(MinLeft, Vertex.LocalPosition.x);
        MaxRight = std::max(MaxRight, Vertex.LocalPosition.x + Vertex.Size.x);
        MaxTop = std::max(MaxTop, Vertex.LocalPosition.y);
        MinBottom = std::min(MinBottom, Vertex.LocalPosition.y - Vertex.Size.y);
    }

    const float CenterX = (MinLeft + MaxRight) * 0.5f;
    const float CenterY = (MinBottom + MaxTop) * 0.5f;

    for (FTextVertex& Vertex : Vertices)
    {
        Vertex.LocalPosition.x -= CenterX;
        Vertex.LocalPosition.y -= CenterY;
    }

    UpdatePickingBoxFromTextGeometry();
}

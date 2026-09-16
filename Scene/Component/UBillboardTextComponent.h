#pragma once

#include "UPrimitiveComponent.h"

#include "Core/Asset/FAssetHandle.h"
#include "Core/Base/FRenderProbe.h"
#include "STL.h"

class UFont;
class FPropertyEditorContext;

class UBillboardTextComponent : public UPrimitiveComponent
{
public:
    UBillboardTextComponent() = default;
    ~UBillboardTextComponent() override = default;

    // UNameTagComponent가 상속해야 하므로 final을 붙이지 않는다.
    JG_DECLARE_DERIVED_TYPEINFO(UBillboardTextComponent, UPrimitiveComponent);

    void SetFontHandle(FAssetHandle InFontHandle);
    void SetPipelineHandle(FAssetHandle InPipelineHandle);

    void SetText(const FString& InText);
    void SetColor(const FVector4& InColor);

    void SetCharacterHeight(float InCharacterHeight);
    void SetLetterSpacing(float InLetterSpacing);
    void SetLineSpacing(float InLineSpacing);

    FAssetHandle GetFontHandle() const;
    FAssetHandle GetPipelineHandle() const;

    const FString& GetText() const;
    const FVector4& GetColor() const;

    float GetCharacterHeight() const;
    float GetLetterSpacing() const;
    float GetLineSpacing() const;

    const TArray<FTextVertex>& GetVertices() const;

    // 기존 FTextProbe를 직접 생성한다.
    bool MakeTextRender(FTextProbe& OutProbe) const;

    void OnRegister() override;
    void OnUnregister() override;

    void DrawPanels(FPropertyEditorContext& Context) override;

protected:
    virtual bool TryGetTextWorld(FMatrix& OutWorld) const;
    void Serialize(FArchive& Archive) override;
    // Dynamic Font에서 글리프를 요청하고 FTextVertex 배열을 다시 생성한다.
    void RebuildTextGeometry();

protected:
    FAssetHandle FontHandle{};
    FAssetHandle PipelineHandle{};

    FString Text{};

    FVector4 Color{1.0f,1.0f,1.0f,1.0f};

    float CharacterHeight = 1.0f;
    float LetterSpacing = 0.0f;
    float LineSpacing = 0.0f;

    TArray<FTextVertex> Vertices{};
};
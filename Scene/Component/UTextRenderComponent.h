#pragma once
#include <stdint.h>
#include "UPrimitiveComponent.h"
#include "../../Core/Asset/UFont.h"
#include "../../Core/Asset/FAssetHandle.h"
#include "../../STL.h"

class UTextRenderComponent : public UPrimitiveComponent
{
public:
	UTextRenderComponent() = default;
	~UTextRenderComponent() override = default;

	JG_DECLARE_DERIVED_TYPEINFO(UTextRenderComponent, UPrimitiveComponent);

	void SetFontHandle(FAssetHandle InFontHandle);
	void SetPipelineHandle(FAssetHandle InPipelineHandle);

	void SetText(const FString& InText);
	void SetColor(const FVector4& InColor);

	void SetCharacterHeight(float InCharacterHeight);
	void SetLetterSpacing(float InLetterSpacing);
	void SetLineSpacing(float InLineSpacing);
	
	const FAssetHandle GetFontHandle() const;
	const FString& GetText() const;
	const TArray<FTextVertex>& GetVertex() const;

	void OnRegister() override;
	void OnUnregister() override;

	virtual void RebuildTextGeometry();

	bool MakeTextRender(FTextProbe& OutProbe) const;

protected:
	FAssetHandle FontHandle{};
	FAssetHandle PipelineHandle{};
	FString Text = {};
	FVector4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
	float CharacterHeight = 1.0f;
	float LetterSpacing = 0.0f;
	float LineSpacing = 0.0f;
	TArray<FTextVertex> Vertices{};
};
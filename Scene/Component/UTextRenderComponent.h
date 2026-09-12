#pragma once
#include <stdint.h>
#include "UPrimitiveComponent.h"
#include "../../Core/Asset/UFont.h"
#include "../../STL.h"

struct FTextVertex
{
	FVector3 Position{};
	FVector2 UV{};
};

struct UTextRenderComponent : public UPrimitiveComponent
{
public:
	UTextRenderComponent() = default;
	~UTextRenderComponent() override = default;

	JG_DECLARE_DERIVED_TYPEINFO(UTextRenderComponent, UPrimitiveComponent);

	void SetFont(UFont* Font);
	void SetText(FString& Text);
	void SetCharacterHeight(float CharacterHeight);
	void SetLetterSpacing(float LetterSpacing);
	void SetLineSpacing(float LineSpacing);
	

private:
	UFont* Font;
	FString Text = {};
	uint16_t Size = 10.0f;
	FVector4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
	float CharacterHeight = 1.0f;
	float LetterSpacing = 0.0f;
	float LineSpacing = 0.0f;
};
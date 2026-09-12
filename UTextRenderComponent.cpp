#include "PCH.h"
#include "Scene/Component/UTextRenderComponent.h"

#include "Scene/AActor.h"
#include "Scene/UWorld.h"

#include "Core/Asset/FAssetRegistry.h"
#include "Core/Asset/UFont.h"

void UTextRenderComponent::SetFontHandle(FAssetHandle InFontHandle)
{
	FontHandle = InFontHandle;
	RebuildTextGeometry();
}
void UTextRenderComponent::SetText(const FString& InText)
{
	if (Text == InText)
	{
		return;
	}
	Text = InText;
	RebuildTextGeometry();
}
void UTextRenderComponent::SetPipelineHandle(FAssetHandle InPipelineHandle)
{
	PipelineHandle = InPipelineHandle;
}
void UTextRenderComponent::SetColor(const FVector4& InColor)
{
	Color = InColor;
}
void UTextRenderComponent::SetCharacterHeight(float InCharacterHeight)
{
	CharacterHeight = InCharacterHeight;
	RebuildTextGeometry();
}
void UTextRenderComponent::SetLetterSpacing(float InLetterSpacing)
{
	LetterSpacing = InLetterSpacing;
	RebuildTextGeometry();
}
void UTextRenderComponent::SetLineSpacing(float InLineSpacing)
{
	LineSpacing = InLineSpacing;
	RebuildTextGeometry();
}
const FAssetHandle UTextRenderComponent::GetFontHandle() const
{
	return FontHandle;
}
const FString& UTextRenderComponent::GetText() const
{
	return Text;
}
const TArray<FTextVertex>& UTextRenderComponent::GetVertex() const
{
	return Vertices;
}
void UTextRenderComponent::OnCreate()
{
	UPrimitiveComponent::OnCreate();

	AActor* Owner = GetOwner();

	if (Owner != nullptr && Owner->GetWorld() != nullptr)
	{
		Owner->GetWorld()->RegisterTextRenderable(this);
	}

	RebuildTextGeometry();
}
void UTextRenderComponent::OnDestroy()
{
	AActor* Owner = GetOwner();

	if (Owner != nullptr && Owner->GetWorld() != nullptr)
	{
		Owner->GetWorld()->UnregisterTextRenderable(this);
	}

	UPrimitiveComponent::OnDestroy();
}
bool UTextRenderComponent::MakeTextRender(FTextProbe& OutProbe) const
{
	if (!IsActive() || !IsVisible() || !FontHandle || !PipelineHandle || Vertices.empty())
	{
		return false;
	}

	OutProbe.World = GetWorldMatrix();
	OutProbe.FontHandle = FontHandle;
	OutProbe.PipelineHandle = PipelineHandle;
	OutProbe.Color = Color;
	OutProbe.Vertices = Vertices;

	return true;
}
void UTextRenderComponent::RebuildTextGeometry()
{
	Vertices.clear();
	if (!FontHandle || Text.empty() || CharacterHeight <= 0.0f)
	{
		return;
	}
	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return;
	}
	UWorld* World = Owner->GetWorld();
	if (World == nullptr)
	{
		return;
	}
	FAssetRegistry* AssetRegistry = World->GetAssetRegistry();
	if (AssetRegistry == nullptr)
	{
		return;
	}
	UFont* Font = AssetRegistry->ResolveAsset<UFont>(FontHandle);
	if (Font == nullptr)
	{
		return;
	}
	float PenX = 0.0f;
	float PenY = 0.0f;
	for (const char RawCharacter : Text)
	{
		if (RawCharacter == '\r')
		{
			continue;
		}
		if (RawCharacter == '\n')
		{
			PenX = 0.0f;
			PenY -= CharacterHeight + LineSpacing;
			continue;
		}
		std::uint8_t Character = static_cast<std::uint8_t>(static_cast<unsigned char>(RawCharacter));
		FFontCharacter* FontCharacter = Font->FindCharacter(Character);
		if (FontCharacter == nullptr)
		{
			Character = static_cast<std::uint8_t>('?');
			FontCharacter = Font->FindCharacter(Character);
		}
		if (FontCharacter == nullptr || FontCharacter->VSize == 0)
		{
			continue;
		}
		const float PixelToWorld = CharacterHeight / static_cast<float>(FontCharacter->VSize);
		const float CharacterWidth = static_cast<float>(FontCharacter->USize) * PixelToWorld;
		const float CharacterAdvance = FontCharacter->Advance * PixelToWorld;
		if (Character != static_cast<std::uint8_t>(' '))
		{
			FFontUVRect UV = Font->GetUV(Character);
			Vertices.emplace_back(FTextVertex{
				.LocalPosition = FVector2{ PenX,PenY },
				.Size = FVector2{ CharacterWidth,CharacterHeight },
				.UVMin = UV.UVMin,
				.UVMax = UV.UVMax
				}
			);
		}
		PenX += CharacterAdvance + LetterSpacing;
	}
}
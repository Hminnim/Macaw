#include "PCH.h"
#include "Scene/Component/UTextRenderComponent.h"
#include "Render/Panel/FPropertyEditorContext.h"

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
void UTextRenderComponent::DrawPanels(FPropertyEditorContext& Context)
{
	UPrimitiveComponent::DrawPanels(Context);
	Context.DrawTextRenderComponentProperties(*this);
}
const FAssetHandle UTextRenderComponent::GetPipelineHandle() const
{
	return PipelineHandle;
}
const FString& UTextRenderComponent::GetText() const
{
	return Text;
}
const FVector4& UTextRenderComponent::GetColor() const
{
	return Color;
}
float UTextRenderComponent::GetCharacterHeight() const
{
	return CharacterHeight;
}
float UTextRenderComponent::GetLetterSpacing() const
{
	return LetterSpacing;
}
float UTextRenderComponent::GetLineSpacing() const
{
	return LineSpacing;
}
const TArray<FTextVertex>& UTextRenderComponent::GetVertex() const
{
	return Vertices;
}
void UTextRenderComponent::OnRegister()
{
	UPrimitiveComponent::OnRegister();

	AActor* Owner = GetOwner();

	if (Owner != nullptr && Owner->GetWorld() != nullptr)
	{
		Owner->GetWorld()->RegisterTextRenderable(this);
	}

	RebuildTextGeometry();
}
void UTextRenderComponent::OnUnregister()
{
	AActor* Owner = GetOwner();

	if (Owner != nullptr && Owner->GetWorld() != nullptr)
	{
		Owner->GetWorld()->UnregisterTextRenderable(this);
	}

	UPrimitiveComponent::OnUnregister();
}
bool UTextRenderComponent::MakeTextRender(FTextProbe& OutProbe) const
{
	if (!IsActive() || !IsVisible() || !FontHandle || !PipelineHandle || Vertices.empty())
	{
		return false;
	}

	OutProbe.World = GetComponentToWorld();
	OutProbe.FontHandle = FontHandle;
	OutProbe.PipelineHandle = PipelineHandle;
	OutProbe.Color = Color;
	OutProbe.Vertices = Vertices;

	return true;
}
void UTextRenderComponent::Serialize(FArchive& Archive)
{
	UPrimitiveComponent::Serialize(Archive);

	auto SerializeAssetHandle = [&Archive](std::string_view Name, FAssetHandle& Handle) {
		FString Guid;
		FAssetRegistry* Registry = Archive.GetAssetRegistry();
		if (Archive.IsSaving() && Registry != nullptr && Handle) {
			if (UAsset* Asset = Registry->ResolveAsset<UAsset>(Handle)) {
				Guid = Asset->GetGuid().ToString();
			}
		}
		Archive.Serialize(Name, Guid);
		if (Archive.IsLoading()) {
			Handle = {};
			FGuid AssetGuid;
			if (Registry != nullptr && AssetGuid.Parse(Guid)) {
				Handle = Registry->GetAsset(AssetGuid);
			}
		}
	};

	SerializeAssetHandle("GuidFontHandle", FontHandle);
	SerializeAssetHandle("GuidPipelineHandle", PipelineHandle);
	Archive.Serialize("Text", Text);
	Archive.Serialize("Color", Color);
	Archive.Serialize("CharacterHeight", CharacterHeight);
	Archive.Serialize("LetterSpacing", LetterSpacing);
	Archive.Serialize("LineSpacing", LineSpacing);
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
	float MaxRight = -1.0f;
	float MaxBottom = 1.0f;
	for (const char RawCharacter : Text)
	{
		if (RawCharacter == '\r')
		{
			continue;
		}
		if (RawCharacter == '\n')
		{
			PenX = 0.0f;
			PenY -= CharacterHeight + LineSpacing - 0.2f; //Todo : 하드 코딩 수정
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
		MaxRight = std::max(MaxRight, PenX + CharacterWidth);
		MaxBottom = std::min(MaxBottom, PenY - CharacterHeight);
	}
	const float CenterX = MaxRight / 2.0f;
	const float CenterY = MaxBottom / 2.0f;
	for (FTextVertex& Vertex : Vertices)
	{
		Vertex.LocalPosition.x -= CenterX;
		Vertex.LocalPosition.y -= CenterY;
	}
}

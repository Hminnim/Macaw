#include "PCH.h"
#include "Scene/Component/UTextRenderComponent.h"
#include "Render/Panel/FPropertyEditorContext.h"

#include "Scene/AActor.h"
#include "Scene/UWorld.h"

#include "Core/Asset/FAssetRegistry.h"
#include "Core/Asset/UFont.h"
#include "Render/Pipeline/UPipeline.h"

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
	Context.DrawText("Text", GetText(), [this](const FString& TextValue) {
		SetText(TextValue);
	});
	Context.DrawColor("Color", GetColor(), [this](const FVector4& ColorValue) {
		SetColor(ColorValue);
	});
	Context.DrawFloat("Character Height", GetCharacterHeight(), 0.01f, 0.001f, FLT_MAX, [this](float CharacterHeightValue) {
		SetCharacterHeight(CharacterHeightValue);
	});
	Context.DrawFloat("Letter Spacing", GetLetterSpacing(), 0.01f, 0.0f, 0.0f, [this](float LetterSpacingValue) {
		SetLetterSpacing(LetterSpacingValue);
	});
	Context.DrawFloat("Line Spacing", GetLineSpacing(), 0.01f, 0.0f, 0.0f, [this](float LineSpacingValue) {
		SetLineSpacing(LineSpacingValue);
	});

	AActor* Owner = GetOwner();
	UWorld* World = Owner != nullptr ? Owner->GetWorld() : nullptr;
	FAssetRegistry* Registry = World != nullptr ? World->GetAssetRegistry() : nullptr;
	if (Registry == nullptr) {
		Context.DrawDisabledText("Font/Pipeline: Asset registry unavailable");
		return;
	}
	Context.DrawAssetPicker("Font", *Registry, *UFont::StaticTypeInfo(), GetFontHandle(), [this](FAssetHandle Handle) {
		SetFontHandle(Handle);
	});
	Context.DrawAssetPicker("Text Pipeline", *Registry, *UPipeline::StaticTypeInfo(), GetPipelineHandle(), [this](FAssetHandle Handle) {
		SetPipelineHandle(Handle);
	});
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

	UpdatePickingBoxFromTextGeometry();
}

void UTextRenderComponent::UpdatePickingBoxFromTextGeometry()
{
	if (Vertices.empty())
	{
		return;
	}

	float MinX = std::numeric_limits<float>::max();
	float MinY = std::numeric_limits<float>::max();
	float MaxX = std::numeric_limits<float>::lowest();
	float MaxY = std::numeric_limits<float>::lowest();
	for (const FTextVertex& Vertex : Vertices)
	{
		MinX = std::min(MinX, Vertex.LocalPosition.x);
		MaxX = std::max(MaxX, Vertex.LocalPosition.x + Vertex.Size.x);
		MinY = std::min(MinY, Vertex.LocalPosition.y - Vertex.Size.y);
		MaxY = std::max(MaxY, Vertex.LocalPosition.y);
	}

	SetPickingBox(DirectX::BoundingOrientedBox{
		DirectX::XMFLOAT3{ (MinX + MaxX) * 0.5f, (MinY + MaxY) * 0.5f, 0.0f },
		DirectX::XMFLOAT3{
			std::max((MaxX - MinX) * 0.5f, 0.001f),
			std::max((MaxY - MinY) * 0.5f, 0.001f),
			0.01f
		},
		DirectX::XMFLOAT4{ 0.0f, 0.0f, 0.0f, 1.0f }
	});
}

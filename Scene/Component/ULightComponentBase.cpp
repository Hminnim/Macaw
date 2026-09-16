#include "PCH.h"

#include "ULightComponentBase.h"

#include "Render/Panel/FPropertyEditorContext.h"

const FVector3& ULightComponentBase::GetLightColor() const {
    return LightColor;
}

void ULightComponentBase::SetLightColor(const FVector3& InLightColor) {
    LightColor = InLightColor;
}

float ULightComponentBase::GetIntensity() const {
    return Intensity;
}

void ULightComponentBase::SetIntensity(float InIntensity) {
    Intensity = std::max(InIntensity, 0.0f);
}

bool ULightComponentBase::IsVisible() const {
    return bVisible;
}

void ULightComponentBase::SetVisible(bool bInVisible) {
    bVisible = bInVisible;
}

void ULightComponentBase::DrawPanels(FPropertyEditorContext& Context) {
    USceneComponent::DrawPanels(Context);

    if (!Context.BeginCategory("Light")) {
        return;
    }

    Context.DrawColor("Color", FVector4{ LightColor, 1.0f }, [this](const FVector4& Color) {
        SetLightColor(FVector3{ Color.x, Color.y, Color.z });
    });
    Context.DrawFloat("Intensity", GetIntensity(), 0.1f, 0.0f, FLT_MAX, [this](float InIntensity) {
        SetIntensity(InIntensity);
    });
    Context.DrawBool("Visible", IsVisible(), [this](bool bInVisible) {
        SetVisible(bInVisible);
    });
}

void ULightComponentBase::Serialize(FArchive& Archive) {
    USceneComponent::Serialize(Archive);

    Archive.Serialize("LightColor", LightColor);
    Archive.Serialize("Intensity", Intensity);
    Archive.Serialize("bVisible", bVisible);
}

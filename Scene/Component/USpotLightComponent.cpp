#include "PCH.h"

#include "USpotLightComponent.h"

#include "Render/Panel/FPropertyEditorContext.h"

#include <numbers>

namespace {
    constexpr float MinimumConeAngle = 0.0f;
    constexpr float MaximumConeAngle = 89.9f;

    float ToRadians(float Degrees) {
        return Degrees * (std::numbers::pi_v<float> / 180.0f);
    }
}

ELightType USpotLightComponent::GetLightType() const {
    return ELightType::Spot;
}

float USpotLightComponent::GetInnerConeAngle() const {
    return InnerConeAngle;
}

float USpotLightComponent::GetOuterConeAngle() const {
    return OuterConeAngle;
}

void USpotLightComponent::SetInnerConeAngle(float InInnerConeAngle) {
    InnerConeAngle = std::clamp(InInnerConeAngle, MinimumConeAngle, OuterConeAngle);
}

void USpotLightComponent::SetOuterConeAngle(float InOuterConeAngle) {
    OuterConeAngle = std::clamp(InOuterConeAngle, InnerConeAngle, MaximumConeAngle);
}

void USpotLightComponent::MakeLightProbe(FLightProbe& OutProbe) const {
    UPointLightComponent::MakeLightProbe(OutProbe);
    OutProbe.InnerConeCos = std::cos(ToRadians(InnerConeAngle));
    OutProbe.OuterConeCos = std::cos(ToRadians(OuterConeAngle));
}

void USpotLightComponent::DrawPanels(FPropertyEditorContext& Context) {
    UPointLightComponent::DrawPanels(Context);

    if (!Context.BeginCategory("Spot Light")) {
        return;
    }

    Context.DrawFloat("Inner Cone Angle", GetInnerConeAngle(), 0.1f, MinimumConeAngle, GetOuterConeAngle(), [this](float InInnerConeAngle) {
        SetInnerConeAngle(InInnerConeAngle);
    });
    Context.DrawFloat("Outer Cone Angle", GetOuterConeAngle(), 0.1f, GetInnerConeAngle(), MaximumConeAngle, [this](float InOuterConeAngle) {
        SetOuterConeAngle(InOuterConeAngle);
    });
}

void USpotLightComponent::Serialize(FArchive& Archive) {
    UPointLightComponent::Serialize(Archive);
    Archive.Serialize("InnerConeAngle", InnerConeAngle);
    Archive.Serialize("OuterConeAngle", OuterConeAngle);

    if (Archive.IsLoading()) {
        SetInnerConeAngle(InnerConeAngle);
        SetOuterConeAngle(OuterConeAngle);
    }
}

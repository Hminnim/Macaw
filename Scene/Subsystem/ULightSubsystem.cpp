#include "PCH.h"

#include "ULightSubsystem.h"

#include "Scene/Component/ULightComponent.h"

void ULightSubsystem::RegisterComponent(ULightComponent* Component) {
    if (Component == nullptr || ContainsComponent(Component)) {
        return;
    }

    Components.push_back(Component);
}

void ULightSubsystem::UnregisterComponent(ULightComponent* Component) {
    std::erase(Components, Component);
}

void ULightSubsystem::BuildLightProbes(FRenderProbe& Probe) const {
    Probe.LightProbes.clear();
    Probe.LightProbes.reserve(Components.size());

    for (const ULightComponent* Component : Components) {
        if (Component == nullptr || !Component->IsActive() || !Component->IsVisible()) {
            continue;
        }

        FLightProbe LightProbe{};
        Component->MakeLightProbe(LightProbe);
        Probe.LightProbes.push_back(LightProbe);
    }
}

bool ULightSubsystem::ContainsComponent(const ULightComponent* Component) const {
    return std::ranges::find(Components, Component) != Components.end();
}

const TArray<ULightComponent*>& ULightSubsystem::GetRegisteredComponents() const {
    return Components;
}

void ULightSubsystem::OnDeinitialize() {
    Components.clear();
}

#include "PCH.h"

#include "UTextSubsystem.h"

#include "Scene/AActor.h"
#include "Scene/UWorld.h"
#include "Scene/Component/UTextRenderComponent.h"
#include "Scene/FWorldEditorContext.h"

#include "../../Render/Pipeline/UPipeline.h"

void UTextSubsystem::RegisterComponent(UTextRenderComponent* Component) {
    if (Component == nullptr || ContainsComponent(Component)) {
        return;
    }

    Components.push_back(Component);
}

void UTextSubsystem::UnregisterComponent(UTextRenderComponent* Component) {
    std::erase(Components, Component);
}

void UTextSubsystem::BuildRenderProbes(FRenderProbe& Probe) const {
    Probe.TextProbes.clear();

    for (const UTextRenderComponent* Component : Components) {
        FTextProbe TextProbe{};
        Component->MakeTextRender(TextProbe);
        Probe.TextProbes.push_back(TextProbe);
    }
}

bool UTextSubsystem::ContainsComponent(const UTextRenderComponent* Component) const {
    return std::ranges::find(Components, Component) != Components.end();
}

const TArray<UTextRenderComponent*>& UTextSubsystem::GetRegisteredComponents() const {
    return Components;
}

void UTextSubsystem::OnDeinitialize() {
    Components.clear();
}
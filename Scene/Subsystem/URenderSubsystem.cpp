#include "PCH.h"

#include "URenderSubsystem.h"

#include "Scene/AActor.h"
#include "Scene/Component/UStaticMeshComponent.h"

void URenderSubsystem::RegisterComponent(UStaticMeshComponent* Component) {
    if (Component == nullptr || ContainsComponent(Component)) {
        return;
    }

    Components.push_back(Component);
}

void URenderSubsystem::UnregisterComponent(UStaticMeshComponent* Component) {
    std::erase(Components, Component);
}

void URenderSubsystem::BuildRenderProbes(FRenderProbe& Probe, const AActor* HighlightedActor) const {
    Probe.ActorProbes.clear();
    Probe.GizmoProbes.clear();

    for (const UStaticMeshComponent* Component : Components) {
        FActorProbe ActorProbe{};
        Component->MakeRender(ActorProbe);

        if (HighlightedActor != nullptr && Component->GetOwner() == HighlightedActor) {
            ActorProbe.Flags |= 0x0000'0001;
        }

        Probe.ActorProbes.push_back(ActorProbe);
    }
}

bool URenderSubsystem::ContainsComponent(const UStaticMeshComponent* Component) const {
    return std::ranges::find(Components, Component) != Components.end();
}

const TArray<UStaticMeshComponent*>& URenderSubsystem::GetRegisteredComponents() const {
    return Components;
}

void URenderSubsystem::OnDeinitialize() {
    Components.clear();
}

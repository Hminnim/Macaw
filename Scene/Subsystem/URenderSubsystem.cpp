#include "PCH.h"

#include "URenderSubsystem.h"

#include "Scene/AActor.h"
#include "Scene/UWorld.h"
#include "Scene/Component/UStaticMeshComponent.h"
#include "Scene/FWorldEditorContext.h"

#include "../../Render/Pipeline/UPipeline.h"

void URenderSubsystem::RegisterComponent(UStaticMeshComponent* Component) {
    if (Component == nullptr || ContainsComponent(Component)) {
        return;
    }

    Components.push_back(Component);
}

void URenderSubsystem::UnregisterComponent(UStaticMeshComponent* Component) {
    std::erase(Components, Component);
}

void URenderSubsystem::BuildRenderProbes(FAssetRegistry* AssetRegistry, FRenderProbe& Probe) const {
    Probe.ActorProbes.clear();
    Probe.GizmoProbes.clear();

    const FWorldEditorContext* EditorContext = GetWorld()->GetEditorContext();
    const AActor* SelectedActor = EditorContext != nullptr ? EditorContext->GetSelectedActor() : nullptr;
    for (const UStaticMeshComponent* Component : Components) {
        FActorProbe ActorProbe{};
        Component->MakeRender(ActorProbe);

        if (not Component->IsActive() or not Component->IsVisible()) continue;

        if (AssetRegistry != nullptr) {
            if (UPipeline* Pipeline = AssetRegistry->ResolveAsset<UPipeline>(Component->GetPipelineHandle())) {
                Pipeline->SetRenderMode(static_cast<ERenderMode>(EditorContext->GetRenderModeState()));
            }
        }


        if (SelectedActor != nullptr && Component->GetOwner() == SelectedActor) {
            ActorProbe.Flags |= static_cast<uint32>(ERenderObjectFlags::Selected);
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

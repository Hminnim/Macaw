#include "PCH.h"

#include "UPickingSubsystem.h"

#include "Scene/Component/UMeshComponent.h"
#include "Scene/Component/UPrimitiveComponent.h"

void UPickingSubsystem::RegisterComponent(UPrimitiveComponent* Component) {
    if (Component == nullptr || ContainsComponent(Component)) {
        return;
    }

    Components.emplace_back(Component);
}

void UPickingSubsystem::UnregisterComponent(UPrimitiveComponent* Component) {
    std::erase_if(Components, [Component](const TObjectRef<UPrimitiveComponent>& ComponentRef) {
        return ComponentRef.Get() == Component;
    });
}

bool UPickingSubsystem::Raycast(const FRay& Ray, UPrimitiveComponent*& OutComponent, float& OutDistance) const {
    OutComponent = nullptr;
    OutDistance = std::numeric_limits<float>::max();

    for (const TObjectRef<UPrimitiveComponent>& ComponentRef : Components) {
        UPrimitiveComponent* Component = ComponentRef.Get();
        if (Component == nullptr || !Component->IsActive() || !Component->IsVisible()) {
            continue;
        }

        DirectX::BoundingOrientedBox WorldBox;
        Component->GetPickingBox().Transform(WorldBox, Component->GetComponentToWorld().ToSimpleMath());

        float BroadPhaseDistance = 0.0f;
        if (!WorldBox.Intersects(Ray.position, Ray.direction, BroadPhaseDistance)) {
            continue;
        }

        float HitDistance = BroadPhaseDistance;
        if (Component->GetTypeInfo()->IsA(UMeshComponent::StaticTypeInfo())) {
            auto* MeshComponent = static_cast<UMeshComponent*>(Component);
            if (!MeshComponent->RaycastMesh(Ray, HitDistance)) {
                continue;
            }
        }

        if (HitDistance < OutDistance) {
            OutComponent = Component;
            OutDistance = HitDistance;
        }
    }

    return OutComponent != nullptr;
}

bool UPickingSubsystem::ContainsComponent(const UPrimitiveComponent* Component) const {
    return std::ranges::any_of(Components, [Component](const TObjectRef<UPrimitiveComponent>& ComponentRef) {
        return ComponentRef.Get() == Component;
    });
}

const TArray<TObjectRef<UPrimitiveComponent>>& UPickingSubsystem::GetRegisteredComponents() const {
    return Components;
}

void UPickingSubsystem::OnDeinitialize() {
    Components.clear();
}

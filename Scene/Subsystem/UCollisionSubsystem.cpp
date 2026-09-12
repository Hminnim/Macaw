#include "PCH.h"

#include "UCollisionSubsystem.h"

#include "Scene/Component/UCollisionComponent.h"

void UCollisionSubsystem::RegisterComponent(UCollisionComponent* Component) {
    if (Component == nullptr || ContainsComponent(Component)) {
        return;
    }

    Components.emplace_back(Component);
}

void UCollisionSubsystem::UnregisterComponent(UCollisionComponent* Component) {
    std::erase_if(Components, [Component](const TObjectRef<UCollisionComponent>& ComponentRef) {
        return ComponentRef.Get() == Component;
    });
}

bool UCollisionSubsystem::Raycast(const FRay& Ray, UCollisionComponent*& OutComponent, float& OutDistance) const {
    OutComponent = nullptr;
    OutDistance = std::numeric_limits<float>::max();

    for (const TObjectRef<UCollisionComponent>& ComponentRef : Components) {
        UCollisionComponent* Component = ComponentRef.Get();
        if (Component == nullptr) {
            continue;
        }

        float Distance = 0.0f;
        if (Component->Raycast(Ray, Distance) && Distance < OutDistance) {
            OutDistance = Distance;
            OutComponent = Component;
        }
    }

    return OutComponent != nullptr;
}

bool UCollisionSubsystem::ContainsComponent(const UCollisionComponent* Component) const {
    return std::ranges::any_of(Components, [Component](const TObjectRef<UCollisionComponent>& ComponentRef) {
        return ComponentRef.Get() == Component;
    });
}

const TArray<TObjectRef<UCollisionComponent>>& UCollisionSubsystem::GetRegisteredComponents() const {
    return Components;
}

void UCollisionSubsystem::OnDeinitialize() {
    Components.clear();
}

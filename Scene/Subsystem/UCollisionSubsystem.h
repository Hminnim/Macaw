#pragma once

#include "UWorldSubsystem.h"

#include "Core/Base/TObjectRef.h"

class UCollisionComponent;

/// <summary>Provides world-space raycasts over registered CollisionComponents.</summary>
class UCollisionSubsystem : public UWorldSubsystem {
public:
    UCollisionSubsystem() = default;
    ~UCollisionSubsystem() override = default;

    JG_DECLARE_DERIVED_TYPEINFO(UCollisionSubsystem, UWorldSubsystem)

    void RegisterComponent(UCollisionComponent* Component);
    void UnregisterComponent(UCollisionComponent* Component);
    bool Raycast(const FRay& Ray, UCollisionComponent*& OutComponent, float& OutDistance) const;

    bool ContainsComponent(const UCollisionComponent* Component) const;
    const TArray<TObjectRef<UCollisionComponent>>& GetRegisteredComponents() const;

protected:
    void OnDeinitialize() override;

private:
    TArray<TObjectRef<UCollisionComponent>> Components;
};

#pragma once

#include "UWorldSubsystem.h"

#include "Core/Base/TObjectRef.h"

class UPrimitiveComponent;

/// <summary>Provides editor picking over registered primitive component volumes.</summary>
class UPickingSubsystem : public UWorldSubsystem {
public:
    UPickingSubsystem() = default;
    ~UPickingSubsystem() override = default;

    JG_DECLARE_DERIVED_TYPEINFO(UPickingSubsystem, UWorldSubsystem)

    void RegisterComponent(UPrimitiveComponent* Component);
    void UnregisterComponent(UPrimitiveComponent* Component);
    bool Raycast(const FRay& Ray, UPrimitiveComponent*& OutComponent, float& OutDistance) const;

    bool ContainsComponent(const UPrimitiveComponent* Component) const;
    const TArray<TObjectRef<UPrimitiveComponent>>& GetRegisteredComponents() const;

protected:
    void OnDeinitialize() override;

private:
    TArray<TObjectRef<UPrimitiveComponent>> Components;
};

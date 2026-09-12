#pragma once

#include "UWorldSubsystem.h"

#include "Core/Base/FRenderProbe.h"

class UStaticMeshComponent;

/// <summary>Builds render probes from registered StaticMeshComponents.</summary>
class URenderSubsystem : public UWorldSubsystem {
public:
    URenderSubsystem() = default;
    ~URenderSubsystem() override = default;

    JG_DECLARE_DERIVED_TYPEINFO(URenderSubsystem, UWorldSubsystem)

    void RegisterComponent(UStaticMeshComponent* Component);
    void UnregisterComponent(UStaticMeshComponent* Component);
    void BuildRenderProbes(FRenderProbe& Probe) const;

    bool ContainsComponent(const UStaticMeshComponent* Component) const;
    const TArray<UStaticMeshComponent*>& GetRegisteredComponents() const;

protected:
    void OnDeinitialize() override;

private:
    TArray<UStaticMeshComponent*> Components;
};

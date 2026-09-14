#pragma once

#include "UWorldSubsystem.h"

#include "Core/Base/FRenderProbe.h"

class UTextRenderComponent;

/// <summary>Builds render probes from registered StaticMeshComponents.</summary>
class UTextSubsystem : public UWorldSubsystem {
public:
    UTextSubsystem() = default;
    ~UTextSubsystem() override = default;

    JG_DECLARE_DERIVED_TYPEINFO(UTextSubsystem, UWorldSubsystem);

    void RegisterComponent(UTextRenderComponent* Component);
    void UnregisterComponent(UTextRenderComponent* Component);
    void BuildRenderProbes(FRenderProbe& Probe) const;

    bool ContainsComponent(const UTextRenderComponent* Component) const;
    const TArray<UTextRenderComponent*>& GetRegisteredComponents() const;

protected:
    void OnDeinitialize() override;

private:
    TArray<UTextRenderComponent*> Components;
};

#pragma once

#include "Core/Base/UObject.h"

class UWorld;

/// <summary>A World-owned RAII service that follows the World lifetime.</summary>
class UWorldSubsystem : public UObject {
public:
    UWorldSubsystem() = default;
    ~UWorldSubsystem() override = default;

    JG_DECLARE_ABSTRACT_DERIVED_TYPEINFO(UWorldSubsystem, UObject)

    void Initialize(UWorld* World);
    void Deinitialize();

    UWorld* GetWorld() const;
    bool IsInitialized() const;

protected:
    virtual void OnInitialize();
    virtual void OnDeinitialize();

private:
    UWorld* World = nullptr;
    bool bInitialized = false;
};

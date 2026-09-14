#pragma once

#include "UWorldSubsystem.h"

class UCameraComponent;

/// <summary>Maintains the World's active main camera.</summary>
class UCameraSubsystem : public UWorldSubsystem {
public:
    UCameraSubsystem() = default;
    ~UCameraSubsystem() override = default;

    JG_DECLARE_DERIVED_TYPEINFO(UCameraSubsystem, UWorldSubsystem)

    void SetMainCamera(UCameraComponent* Camera);
    void ClearMainCamera(UCameraComponent* Camera);
    UCameraComponent* GetMainCamera() const;

protected:
    void OnDeinitialize() override;

private:
    UCameraComponent* MainCamera = nullptr;
};

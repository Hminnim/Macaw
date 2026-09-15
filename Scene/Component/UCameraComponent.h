#pragma once

#include "USceneComponent.h"
#include "Serialize/FArchive.h"

class UCameraComponent : public USceneComponent {
public:
    UCameraComponent();
    ~UCameraComponent() override = default;

    JG_DECLARE_DERIVED_TYPEINFO(UCameraComponent, USceneComponent)

    FMatrix GetViewMatrix() const;
    FMatrix GetProjectionMatrix() const;
    FMatrix GetViewProjectionMatrix() const;

    float GetFOV() const;
    float GetAspectRatio() const;
    float GetNearPlane() const;
    float GetFarPlane() const;

    void SetFOV(float InFOV);
    void SetAspectRatio(float InAspectRatio);
    void SetNearPlane(float InNearPlane);
    void SetFarPlane(float InFarPlane);
    void DrawPanels(FPropertyEditorContext& Context) override;

    void OnRegister() override;
    void OnUnregister() override;

protected:
    void Serialize(FArchive& Archive) override;

private:
    float FOV = 1.0472f;       // 약 60도, 라디안
    float AspectRatio = 16.0f / 9.0f;
    float NearPlane = 0.1f;
    float FarPlane = 1000.0f;
};

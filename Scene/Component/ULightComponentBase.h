#pragma once

#include "USceneComponent.h"

class ULightComponentBase : public USceneComponent {
public:
    ULightComponentBase() = default;
    ~ULightComponentBase() override = default;

    JG_DECLARE_ABSTRACT_DERIVED_TYPEINFO(ULightComponentBase, USceneComponent)

    const FVector3& GetLightColor() const;
    void SetLightColor(const FVector3& InLightColor);

    float GetIntensity() const;
    void SetIntensity(float InIntensity);

    bool IsVisible() const;
    void SetVisible(bool bInVisible);

    void DrawPanels(FPropertyEditorContext& Context) override;

protected:
    void Serialize(FArchive& Archive) override;

private:
    FVector3 LightColor{ 1.0f, 1.0f, 1.0f };
    float Intensity{ 1.0f };
    bool bVisible{ true };
};

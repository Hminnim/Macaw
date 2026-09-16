#pragma once

#include "ULocalLightComponent.h"

class UPointLightComponent : public ULocalLightComponent {
public:
    UPointLightComponent() = default;
    ~UPointLightComponent() override = default;

    JG_DECLARE_DERIVED_TYPEINFO(UPointLightComponent, ULocalLightComponent)

    ELightType GetLightType() const override;
};

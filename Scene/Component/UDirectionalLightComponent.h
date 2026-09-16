#pragma once

#include "ULightComponent.h"

class UDirectionalLightComponent : public ULightComponent {
public:
    UDirectionalLightComponent() = default;
    ~UDirectionalLightComponent() override = default;

    JG_DECLARE_DERIVED_TYPEINFO(UDirectionalLightComponent, ULightComponent)

    ELightType GetLightType() const override;
};

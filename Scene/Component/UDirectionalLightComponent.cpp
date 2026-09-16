#include "PCH.h"

#include "UDirectionalLightComponent.h"

ELightType UDirectionalLightComponent::GetLightType() const {
    return ELightType::Directional;
}

#pragma once

#include "UTextRenderComponent.h"

class UKTextRenderComponent final : public UTextRenderComponent
{
public:
    JG_DECLARE_DERIVED_TYPEINFO(UKTextRenderComponent, UTextRenderComponent);

    void RebuildTextGeometry() override;
};
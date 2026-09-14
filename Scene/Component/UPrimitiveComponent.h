#pragma once

#include "UActorComponent.h"
#include "USceneComponent.h"

#include "../../Core/Base/FRenderProbe.h"

class UPrimitiveComponent : public USceneComponent {
public:
    UPrimitiveComponent() = default;
    ~UPrimitiveComponent() override = default;

    JG_DECLARE_ABSTRACT_DERIVED_TYPEINFO(UPrimitiveComponent, USceneComponent)

    bool IsVisible() const;
    void SetVisible(bool bInVisible);

    virtual void MakeRender(FActorProbe& OutProbe) const {};

protected:
    void Serialize(FArchive& Archive) override;

private:
    bool bVisible = true;
};

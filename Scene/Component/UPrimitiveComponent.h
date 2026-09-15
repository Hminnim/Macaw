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
    void DrawPanels(FPropertyEditorContext& Context) override;

    virtual void MakeRender(FActorProbe& OutProbe) const {};

	void SetPickingBox(const DirectX::BoundingOrientedBox& Box) { PickingBox = Box; }
	DirectX::BoundingOrientedBox& GetPickingBox() { return PickingBox; }

protected:
    void Serialize(FArchive& Archive) override;

private:
    bool bVisible = true;
    DirectX::BoundingOrientedBox PickingBox{ DirectX::XMFLOAT3{0.f,0.f,0.f}, DirectX::XMFLOAT3{1.f,1.f,1.f}, DirectX::XMFLOAT4{0.f,0.f,0.f, 1.f} };
};

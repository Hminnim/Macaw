#pragma once

#include "UPrimitiveComponent.h"
#include "Core/Base/TypeInfo.h"
#include "Serialize/FArchive.h"

class ILineRenderer;
enum class ELineDepthMode : uint8;

class UCollisionComponent : public UPrimitiveComponent {
public:
    UCollisionComponent() = default;
    ~UCollisionComponent() override = default;

    void OnRegister() override;
    void OnUnregister() override;

    bool IsCollisionEnabled() const;
    void SetCollisionEnabled(bool bEnabled);
    void DrawPanels(FPropertyEditorContext& Context) override;

    bool Raycast(const FRay& Ray, float& OutDistance) const;
    virtual void DrawEditorBounds(ILineRenderer& LineRenderer, ELineDepthMode DepthMode) const = 0;

    void MakeRender(FActorProbe& OutProbe) const override;

    JG_DECLARE_ABSTRACT_DERIVED_TYPEINFO(UCollisionComponent, UPrimitiveComponent)

    virtual bool RaycastBounds(const FRay& Ray, float& OutDistance) const = 0;
    virtual class UMeshComponent* GetMeshComponent() const { return nullptr; }

protected:
    void Serialize(FArchive& Archive) override;

private:
    bool bCollisionEnabled = true;
};

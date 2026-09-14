#pragma once

#include "UPrimitiveComponent.h"
#include "Core/Base/TypeInfo.h"
#include "Serialize/FArchive.h"

class UCollisionComponent : public UPrimitiveComponent {
public:
    UCollisionComponent() = default;
    ~UCollisionComponent() override = default;

    void OnRegister() override;
    void OnUnregister() override;

    bool IsCollisionEnabled() const;
    void SetCollisionEnabled(bool bEnabled);

    bool Raycast(const FRay& Ray, float& OutDistance) const;

    void MakeRender(FActorProbe& OutProbe) const override;

    JG_DECLARE_ABSTRACT_DERIVED_TYPEINFO(UCollisionComponent, UPrimitiveComponent)

    virtual bool RaycastBounds(const FRay& Ray, float& OutDistance) const = 0;
    virtual class UMeshComponent* GetMeshComponent() const { return nullptr; }
    virtual FVector3 GetBoundsCenter() const = 0;
    virtual FVector3 GetExtent() const = 0;
    virtual FQuat GetBoundsOrientation() const = 0;
    virtual void SetExtent(const FVector3& InExtent) = 0;

protected:
    void Serialize(FArchive& Archive) override;

private:
    bool bCollisionEnabled = true;
};

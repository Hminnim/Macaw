#pragma once

#include "Core/Base/FTransform.h"
#include "Core/Base/TObjectRef.h"
#include "Serialize/FArchive.h"
#include "UActorComponent.h"

class USceneComponent : public UActorComponent
{
public:
	USceneComponent() = default;
	~USceneComponent() override = default;

	virtual void OnUnregister() override;
	void RemoveChild(USceneComponent* InChild);

	JG_DECLARE_DERIVED_TYPEINFO(USceneComponent, UActorComponent)

	FTransform& GetTransform();
	const FTransform& GetTransform() const;

	void AttachTo(USceneComponent* InParent);
	FMatrix GetWorldMatrix() const;
	USceneComponent* GetParent() const;
	const std::vector<TObjectRef<USceneComponent>>& GetChildren() const;

protected:
	void Serialize(FArchive& Archive) override;



private:
	FTransform Transform;

	TObjectRef<USceneComponent> Parent;
	std::vector<TObjectRef<USceneComponent>> Children;
};

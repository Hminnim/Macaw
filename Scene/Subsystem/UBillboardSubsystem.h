#pragma once
#include "Scene//Subsystem//UWorldSubsystem.h"

#include "Core/Base//FRenderProbe.h"

class UBillboardComponent;

class UBillboardSubsystem : public UWorldSubsystem
{
public:
	UBillboardSubsystem() = default;
	~UBillboardSubsystem() override = default;

	JG_DECLARE_DERIVED_TYPEINFO(UBillboardSubsystem, UWorldSubsystem);

	void RegisterComponent(UBillboardComponent* Component);
	void UnregisterComponent(UBillboardComponent* Component);
	void BuildRenderProbes(FAssetRegistry* AssetRegistry, FRenderProbe& Probe) const;

	bool ContainsComponent(const UBillboardComponent* Component);
	const TArray<UBillboardComponent*>& GetRegisteredComponents() const;

protected:
	void OnDeinitialize() override;

private:
	TArray<UBillboardComponent*> Components;
};
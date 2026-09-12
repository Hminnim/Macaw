#pragma once

#include "Core/Base/UObject.h"
#include "Serialize/FArchive.h"

class AActor;
class UWorld;

class UActorComponent : public UObject {
public:
    UActorComponent() = default;
    ~UActorComponent() override = default;

    JG_DECLARE_DERIVED_TYPEINFO(UActorComponent, UObject)

    AActor* GetOwner() const;

    virtual void OnRegister();
    virtual void Tick(float DeltaTime);
    virtual void OnUnregister();

    bool IsActive() const;
    void SetActive(bool bInActive);

	bool IsRegistered() const;
	UWorld* GetBelongingWorld() const;

    void RegisterComponent(UWorld* world);
	void UnregisterComponent();

    virtual bool ResolveLoadedReferences();
protected:
    void Serialize(FArchive& Archive) override;

private:
    friend class AActor;

    void SetOwner(AActor* InOwner);

private:
	AActor* Owner{ nullptr };
	UWorld* ParentWorld{ nullptr };

	bool bActive{ true };
	bool bRegistered{ false };
};

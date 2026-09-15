#pragma once

#include "Core/Base/UObject.h"
#include "Serialize/FArchive.h"

class AActor;
class FPropertyEditorContext;
class UWorld;

class UActorComponent : public UObject {
public:
    UActorComponent() = default;
    ~UActorComponent() override = default;

	UActorComponent(const UActorComponent&) = delete;
	UActorComponent& operator=(const UActorComponent&) = delete;

	UActorComponent(UActorComponent&&) = default;
	UActorComponent& operator=(UActorComponent&&) = default;

public:
    JG_DECLARE_DERIVED_TYPEINFO(UActorComponent, UObject)

    AActor* GetOwner() const;

    virtual void OnRegister();
    virtual void InitializeComponent();
    virtual void BeginPlay();
    virtual void EndPlay();
    virtual void Tick(float DeltaTime);
    virtual void OnUnregister();
    virtual void DrawPanels(FPropertyEditorContext& Context);

    bool IsActive() const;
    void SetActive(bool bInActive);

	bool IsRegistered() const;
	bool IsInitialized() const;
	bool HasBegunPlay() const;
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
	bool bInitialized{ false };
	bool bHasBegunPlay{ false };
};

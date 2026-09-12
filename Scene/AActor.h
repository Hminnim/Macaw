#pragma once

#include "Common.h"

#include "Core/Base/UObject.h"
#include "Scene/Component/UActorComponent.h"
#include "Scene/Component/USceneComponent.h"

class UWorld;

class AActor : public UObject {
public:
    AActor() = default;
    ~AActor() override;

	AActor(const AActor&) = delete;
	AActor& operator=(const AActor&) = delete;

	AActor(AActor&&) = default;
	AActor& operator=(AActor&&) = default;

public:
    JG_DECLARE_DERIVED_TYPEINFO(AActor, UObject)

    template<typename T>
    requires std::is_base_of_v<UActorComponent, T>
    T* AddComponent() {
        std::unique_ptr<T> NewComponent = std::make_unique<T>();
        T* ComponentPtr = NewComponent.get();

        ComponentPtr->SetOwner(this);
        UObjectSystem::Register(ComponentPtr);

        Components.push_back(std::move(NewComponent));

        if (World != nullptr) {
            ComponentPtr->RegisterComponent(World);

            if (bHasBegunPlay) {
                ComponentPtr->InitializeComponent();
                ComponentPtr->BeginPlay();
            }
        }

        return ComponentPtr;
    }

    template<typename T>
    requires std::is_base_of_v<UActorComponent, T>
    T* GetComponent() {
        for (const auto& Component : Components)
        {
            if (Component->GetTypeInfo()->IsA(T::StaticTypeInfo()))
            {
                return static_cast<T*>(Component.get());
            }
        }

        return nullptr;
    }

    bool DestroyComponent(UActorComponent* component);
    const std::vector<std::unique_ptr<UActorComponent>>& GetComponents() const;

    USceneComponent* GetRootComponent();
    const USceneComponent* GetRootComponent() const;

    void SetWorld(UWorld* InWorld);
    UWorld* GetWorld() const;
    bool SetRootComponent(USceneComponent* InRootComponent);

    FTransform GetActorTransform() const;
    bool SetActorTransform(const FTransform& InTransform);
    FVector3 GetActorLocation() const;
    bool SetActorLocation(const FVector3& InLocation);

    bool HasBegunPlay() const;
    virtual void Tick(float DeltaTime);

    bool PreLoadComponents(FArchive& Archive);
    bool ResolveLoadedReferences();

protected:
    virtual void OnAddedToWorld();
    virtual void InitializeComponents();
    virtual void BeginPlay();
    virtual void EndPlay();
    virtual void OnRemovedFromWorld();

    void Serialize(FArchive& Archive) override;

private:
    std::vector<std::unique_ptr<UActorComponent>> Components{};
    USceneComponent* RootComponent = nullptr;
    FGuid PendingRootComponentGuid{};

    UWorld* World = nullptr;
    bool bHasBegunPlay = false;
};

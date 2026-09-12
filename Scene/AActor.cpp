#include "PCH.h"
#include "AActor.h"
#include "Scene/UWorld.h"
#include "Component/USceneComponent.h"
#include "../Core/Base/TypeRegistry.h"

const std::vector<std::unique_ptr<UActorComponent>>& AActor::GetComponents() const {
    return Components;
}

AActor::~AActor() {
    SetWorld(nullptr);

    for (std::unique_ptr<UActorComponent>& Component : Components) {
        Component->UnregisterComponent();
        UObjectSystem::Unregister(Component.get(), Component->GetHandle());
    }
}

bool AActor::DestroyComponent(UActorComponent* Component) {
    if (Component == nullptr) {
        return false;
    }

    auto It = std::ranges::find_if(Components, [Component](const std::unique_ptr<UActorComponent>& Ptr) {
            return Ptr.get() == Component;
        });

    if (It == Components.end()) {
        return false;
    }

    Component->UnregisterComponent();
    UObjectSystem::Unregister(Component, Component->GetHandle());


    if (RootComponent == Component) {
        RootComponent = nullptr;
    }

    Components.erase(It);

    return true;
}

USceneComponent* AActor::GetRootComponent() {
    return RootComponent;
}

const USceneComponent* AActor::GetRootComponent() const {
    return RootComponent;
}

bool AActor::SetRootComponent(USceneComponent* InRootComponent) {
    if (InRootComponent != nullptr) {
        const bool bIsOwnedComponent = std::ranges::any_of(
            Components,
            [InRootComponent](const std::unique_ptr<UActorComponent>& Component) {
                return Component.get() == InRootComponent;
            }
        );

        if (!bIsOwnedComponent) {
            return false;
        }
    }

    RootComponent = InRootComponent;
    return true;
}

void AActor::SetWorld(UWorld* InWorld) {
    if (World == InWorld) {
        return;
    }

    if (World != nullptr) {
        if (bHasBegunPlay) {
            EndPlay();
            bHasBegunPlay = false;
        }

        for (const std::unique_ptr<UActorComponent>& Component : Components) {
            Component->UnregisterComponent();
        }

        OnRemovedFromWorld();
        World = nullptr;
    }

    if (InWorld == nullptr) {
        return;
    }

    World = InWorld;
    OnAddedToWorld();

    for (const std::unique_ptr<UActorComponent>& Component : Components) {
        Component->RegisterComponent(World);
    }

    InitializeComponents();
    BeginPlay();
    bHasBegunPlay = true;
}

UWorld* AActor::GetWorld() const {
    return World;
}

FTransform AActor::GetActorTransform() const {
    if (RootComponent == nullptr) {
        return {};
    }

    return RootComponent->GetTransform();
}

bool AActor::SetActorTransform(const FTransform& InTransform) {
    if (RootComponent == nullptr) {
        return false;
    }

    RootComponent->GetTransform() = InTransform;
    return true;
}

FVector3 AActor::GetActorLocation() const {
    return GetActorTransform().GetPosition();
}

bool AActor::SetActorLocation(const FVector3& InLocation) {
    if (RootComponent == nullptr) {
        return false;
    }

    RootComponent->GetTransform().SetPosition(InLocation);
    return true;
}

bool AActor::HasBegunPlay() const {
    return bHasBegunPlay;
}

void AActor::Tick(float DeltaTime) {
    if (!bHasBegunPlay) {
        return;
    }

    for (const std::unique_ptr<UActorComponent>& Component : Components) {
        if (Component->IsActive()) {
            Component->Tick(DeltaTime);
        }
    }
}



void AActor::Serialize(FArchive& Archive) {
    UObject::Serialize(Archive);

    // components
    size_t ArraySize = Components.size();
    Archive.BeginArrayScope("Components", ArraySize);

    for (size_t i = 0; i < ArraySize; ++i) {
        Archive.BeginObjectScope(std::to_string(i));
        Components[i]->Serialize(Archive);
        Archive.EndObjectScope();
    }
    Archive.EndArrayScope();

    // root component
    FString GuidRootComponent;
    if (RootComponent != nullptr) {
        GuidRootComponent = RootComponent->GetGuid().ToString();
    }

    Archive.Serialize("GuidRootComponent", GuidRootComponent);
    if (Archive.IsLoading()) {
        RootComponent = nullptr;
        PendingRootComponentGuid = {};

        if (!GuidRootComponent.empty() && !PendingRootComponentGuid.Parse(GuidRootComponent)) {
            PendingRootComponentGuid = {};
        }
    }
}

void AActor::OnAddedToWorld() {
}

void AActor::InitializeComponents() {
    for (const std::unique_ptr<UActorComponent>& Component : Components) {
        if (Component->IsRegistered() && !Component->IsInitialized()) {
            Component->InitializeComponent();
        }
    }
}

void AActor::BeginPlay() {
    for (const std::unique_ptr<UActorComponent>& Component : Components) {
        if (Component->IsRegistered() && !Component->HasBegunPlay()) {
            Component->BeginPlay();
        }
    }
}

void AActor::EndPlay() {
    for (auto It = Components.rbegin(); It != Components.rend(); ++It) {
        UActorComponent* Component = It->get();
        if (Component->HasBegunPlay()) {
            Component->EndPlay();
        }
    }
}

void AActor::OnRemovedFromWorld() {
}

bool AActor::PreLoadComponents(FArchive& Archive) {
    size_t ArraySize = 0;
    Archive.BeginArrayScope("Components", ArraySize);

    Components.clear();
    Components.reserve(ArraySize);

    for (size_t i = 0; i < ArraySize; ++i) {
        Archive.BeginObjectScope(std::to_string(i));

        FString TypeName;
        Archive.Serialize("TypeName", TypeName);

        const FTypeInfo* Type = TypeRegistry::Find(TypeName);
        if (Type == nullptr || Type->Creator == nullptr) {
            Archive.EndObjectScope();
            Archive.EndArrayScope();
            return false;
        }

        std::unique_ptr<UObject> CreatedObject = Type->Creator();
        if (CreatedObject == nullptr ||
            !CreatedObject->GetTypeInfo()->IsA(UActorComponent::StaticTypeInfo())) {
            Archive.EndObjectScope();
            Archive.EndArrayScope();
            return false;
        }

        std::unique_ptr<UActorComponent> Component(
            static_cast<UActorComponent*>(CreatedObject.release())
        );
        Component->SetOwner(this);

        FGuid ComponentGuid;
        Archive.Serialize("Guid", ComponentGuid);
        UObjectSystem::RegisterWithGuid(Component.get(), ComponentGuid);
        Components.push_back(std::move(Component));

        Archive.EndObjectScope();
    }

    Archive.EndArrayScope();
    return true;
}

bool AActor::ResolveLoadedReferences() {
    if (PendingRootComponentGuid.IsValid()) {
        UObject* ResolvedObject = UObjectSystem::Resolve(
            UObjectSystem::FindHandleByGuid(PendingRootComponentGuid)
        );

        if (ResolvedObject == nullptr ||
            !ResolvedObject->GetTypeInfo()->IsA(USceneComponent::StaticTypeInfo())) {
            return false;
        }

        RootComponent = static_cast<USceneComponent*>(ResolvedObject);
    }

    for (const std::unique_ptr<UActorComponent>& Component : Components) {
        if (!Component->ResolveLoadedReferences()) {
            return false;
        }
    }

    return true;
}

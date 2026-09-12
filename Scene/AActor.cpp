#include "PCH.h"
#include "AActor.h"
#include "Scene/UWorld.h"
#include "Component/USceneComponent.h"
#include "../Core/Base/TypeRegistry.h"

const std::vector<std::unique_ptr<UActorComponent>>& AActor::GetComponents() const {
    return Components;
}

AActor::~AActor() {
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

void AActor::SetRootComponent(USceneComponent* InRootComponent) {
    RootComponent = InRootComponent;
}

void AActor::SetWorld(UWorld* InWorld) {
    if (InWorld == nullptr) {
        return;
    }

    World = InWorld;
    for (const std::unique_ptr<UActorComponent>& Component : Components) {
        Component->RegisterComponent(World);
    }
}

UWorld* AActor::GetWorld() const {
    return World;
}

void AActor::Tick(float DeltaTime) {
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

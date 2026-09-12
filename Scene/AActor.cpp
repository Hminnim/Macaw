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
        }
    );

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
        const bool bIsOwnedComponent = std::ranges::any_of(Components, [InRootComponent](const std::unique_ptr<UActorComponent>& Component) {
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

    return RootComponent->GetComponentTransform();
}

bool AActor::SetActorTransform(const FTransform& Transform) {
    if (RootComponent == nullptr) {
        return false;
    }

    return RootComponent->SetWorldTransform(Transform);
}

FVector3 AActor::GetActorLocation() const {
    if (RootComponent == nullptr) {
        return FVector3::Zero;
    }

    return RootComponent->GetComponentLocation();
}

bool AActor::SetActorLocation(const FVector3& Location) {
    if (RootComponent == nullptr) {
        return false;
    }

    return RootComponent->SetWorldLocation(Location);
}

bool AActor::SetActorLocationAndRotation(const FVector3& Location, const FRotator& Rotation) {
    return RootComponent != nullptr && RootComponent->SetWorldLocationAndRotation(Location, Rotation);
}

FRotator AActor::GetActorRotation() const {
    if (RootComponent == nullptr) {
        return FRotator::Zero;
    }

    return RootComponent->GetComponentRotation();
}

bool AActor::SetActorRotation(const FRotator& Rotation) {
    return RootComponent != nullptr && RootComponent->SetWorldRotation(Rotation);
}

FVector3 AActor::GetActorScale3D() const {
    if (RootComponent == nullptr) {
        return { 1.0f, 1.0f, 1.0f };
    }

    return RootComponent->GetComponentScale();
}

bool AActor::SetActorScale3D(const FVector3& Scale) {
    return RootComponent != nullptr && RootComponent->SetWorldScale3D(Scale);
}

FTransform AActor::GetActorRelativeTransform() const {
    return RootComponent != nullptr ? RootComponent->GetRelativeTransform() : FTransform{};
}

bool AActor::SetActorRelativeTransform(const FTransform& Transform) {
    if (RootComponent == nullptr) {
        return false;
    }

    RootComponent->SetRelativeTransform(Transform);
    return true;
}

FVector3 AActor::GetActorRelativeLocation() const {
    return RootComponent != nullptr ? RootComponent->GetRelativeLocation() : FVector3::Zero;
}

bool AActor::SetActorRelativeLocation(const FVector3& Location) {
    if (RootComponent == nullptr) {
        return false;
    }

    RootComponent->SetRelativeLocation(Location);
    return true;
}

bool AActor::SetActorRelativeLocationAndRotation(const FVector3& Location, const FRotator& Rotation) {
    if (RootComponent == nullptr) {
        return false;
    }

    RootComponent->SetRelativeLocationAndRotation(Location, Rotation);
    return true;
}

FRotator AActor::GetActorRelativeRotation() const {
    return RootComponent != nullptr ? RootComponent->GetRelativeRotation() : FRotator::Zero;
}

bool AActor::SetActorRelativeRotation(const FRotator& Rotation) {
    if (RootComponent == nullptr) {
        return false;
    }

    RootComponent->SetRelativeRotation(Rotation);
    return true;
}

FVector3 AActor::GetActorRelativeScale3D() const {
    return RootComponent != nullptr ? RootComponent->GetRelativeScale3D() : FVector3{ 1.0f, 1.0f, 1.0f };
}

bool AActor::SetActorRelativeScale3D(const FVector3& Scale) {
    if (RootComponent == nullptr) {
        return false;
    }

    RootComponent->SetRelativeScale3D(Scale);
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

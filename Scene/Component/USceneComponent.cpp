#include "PCH.h"

#include "USceneComponent.h"
#include "Render/Panel/FPropertyEditorContext.h"

namespace {
    bool DecomposeWorldTransform(const FMatrix& WorldMatrix, FVector3& OutScale, FQuat& OutRotation, FVector3& OutTranslation) {
        FMatrix TransformMatrix = WorldMatrix;

        // Undo FTransform's mesh-source basis before extracting the Z-up
        // transform quaternion and scale.  The basis is its own inverse.
        const float Row0[3]{ TransformMatrix.m[0][0], TransformMatrix.m[0][1], TransformMatrix.m[0][2] };
        const float Row1[3]{ TransformMatrix.m[1][0], TransformMatrix.m[1][1], TransformMatrix.m[1][2] };
        const float Row2[3]{ TransformMatrix.m[2][0], TransformMatrix.m[2][1], TransformMatrix.m[2][2] };
        for (uint32 Column = 0; Column < 3; ++Column) {
            TransformMatrix.m[0][Column] = -Row0[Column];
            TransformMatrix.m[1][Column] = Row2[Column];
            TransformMatrix.m[2][Column] = Row1[Column];
        }

        return TransformMatrix.Decompose(OutScale, OutRotation, OutTranslation);
    }

    bool ApplyWorldMatrix(USceneComponent& Component, const FMatrix& DesiredWorld) {
        FVector3 Scale{};
        FQuat Rotation{};
        FVector3 Translation{};
        if (!DecomposeWorldTransform(DesiredWorld, Scale, Rotation, Translation)) {
            return false;
        }

        return Component.SetWorldTransform(FTransform{ Translation, Rotation, Scale });
    }
}

FTransform& USceneComponent::GetRelativeTransform() {
    return Transform;
}

const FTransform& USceneComponent::GetRelativeTransform() const {
    return Transform;
}

void USceneComponent::SetRelativeTransform(const FTransform& Transform) {
    this->Transform = Transform;
}

void USceneComponent::DrawPanels(FPropertyEditorContext& Context) {
    UActorComponent::DrawPanels(Context);
    Context.DrawSceneComponentProperties(*this);
}

void USceneComponent::SetRelativeLocation(const FVector3& Location) {
    Transform.SetPosition(Location);
}

void USceneComponent::SetRelativeLocationAndRotation(const FVector3& Location, const FRotator& Rotation) {
    Transform.SetPosition(Location);
    Transform.SetRotation(Rotation);
}

FVector3 USceneComponent::GetRelativeLocation() const {
    return Transform.GetPosition();
}

void USceneComponent::SetRelativeRotation(const FRotator& Rotation) {
    Transform.SetRotation(Rotation);
}

FRotator USceneComponent::GetRelativeRotation() const {
    return Transform.GetRotation();
}

void USceneComponent::SetRelativeScale3D(const FVector3& Scale) {
    Transform.SetScale(Scale);
}

FVector3 USceneComponent::GetRelativeScale3D() const {
    return Transform.GetScale();
}

void USceneComponent::Serialize(FArchive& Archive) {
    UActorComponent::Serialize(Archive);

    Archive.SerializeStruct("Transform", Transform);

    FGuid ParentGuid{};
    if (Archive.IsSaving() && GetParent() != nullptr) {
        ParentGuid = GetParent()->GetGuid();
    }

    Archive.Serialize("Parent", ParentGuid);
    if (Archive.IsLoading()) {
        PendingParentGuid = ParentGuid;
    }
}


void USceneComponent::OnUnregister() {
    for (TObjectRef<USceneComponent>& ChildRef : Children) {
        if (USceneComponent* Child = ChildRef.Get()) {
            Child->Parent.Reset();
        }
    }
    Children.clear();

    if (USceneComponent* ParentComponent = Parent.Get()) {
        ParentComponent->RemoveChild(this);
    }
    Parent.Reset();

    UActorComponent::OnUnregister();
}

void USceneComponent::RemoveChild(USceneComponent* InChild) {
    if (!InChild) return;

    std::erase_if(Children, [InChild](const TObjectRef<USceneComponent>& ChildRef) {
        return ChildRef.Get() == InChild;
    });

    if (InChild->Parent.Get() == this) {
        InChild->Parent.Reset();
    }
}

bool USceneComponent::AttachToComponent(USceneComponent* ParentComponent, EAttachmentTransformRule Rule) {
    if (ParentComponent == this || Parent.Get() == ParentComponent) {
        return false;
    }

    for (USceneComponent* Ancestor = ParentComponent; Ancestor != nullptr; Ancestor = Ancestor->GetParent()) {
        if (Ancestor == this) {
            return false;
        }
    }

    const FTransform PreviousWorldTransform = GetComponentTransform();

    if (USceneComponent* PreviousParent = Parent.Get()) {
        PreviousParent->RemoveChild(this);
    }

    Parent.Set(ParentComponent);

    if (ParentComponent != nullptr) {
        ParentComponent->Children.emplace_back(this);
    }

    if (Rule == EAttachmentTransformRule::KeepWorldTransform) {
        return SetWorldTransform(PreviousWorldTransform);
    }

    return true;
}

bool USceneComponent::DetachFromComponent(EAttachmentTransformRule Rule) {
    return AttachToComponent(nullptr, Rule);
}

bool USceneComponent::SetWorldTransform(const FTransform& WorldTransform) {
    FTransform DesiredWorldTransform = WorldTransform;
    DesiredWorldTransform.SetAbsoluteLocation(Transform.IsAbsoluteLocation());
    DesiredWorldTransform.SetAbsoluteRotation(Transform.IsAbsoluteRotation());
    DesiredWorldTransform.SetAbsoluteScale(Transform.IsAbsoluteScale());

    if (USceneComponent* ParentComponent = Parent.Get()) {
        FTransform RelativeTransform;
        if (!DesiredWorldTransform.MakeRelativeTo(ParentComponent->GetComponentTransform(), RelativeTransform)) {
            return false;
        }

        Transform = RelativeTransform;
        return true;
    }

    Transform = DesiredWorldTransform;
    return true;
}

bool USceneComponent::SetWorldTransform(const FMatrix& WorldTransform) {
    return ApplyWorldMatrix(*this, WorldTransform);
}

bool USceneComponent::SetWorldLocation(const FVector3& Location) {
    FTransform DesiredWorldTransform = GetComponentTransform();
    DesiredWorldTransform.SetPosition(Location);
    return SetWorldTransform(DesiredWorldTransform);
}

bool USceneComponent::SetWorldLocationAndRotation(const FVector3& Location, const FRotator& Rotation) {
    FTransform DesiredWorldTransform = GetComponentTransform();
    DesiredWorldTransform.SetPosition(Location);
    DesiredWorldTransform.SetRotation(Rotation);
    return SetWorldTransform(DesiredWorldTransform);
}

bool USceneComponent::SetWorldRotation(const FRotator& Rotation) {
    FTransform DesiredWorldTransform = GetComponentTransform();
    DesiredWorldTransform.SetRotation(Rotation);
    return SetWorldTransform(DesiredWorldTransform);
}

bool USceneComponent::SetWorldScale3D(const FVector3& Scale) {
    FTransform DesiredWorldTransform = GetComponentTransform();
    DesiredWorldTransform.SetScale(Scale);
    return SetWorldTransform(DesiredWorldTransform);
}

USceneComponent* USceneComponent::GetParent() const {
    return Parent.Get();
}

const std::vector<TObjectRef<USceneComponent>>& USceneComponent::GetChildren() const {
    return Children;
}

FTransform USceneComponent::GetComponentTransform() const {
    if (USceneComponent* ParentComponent = Parent.Get()) {
        return Transform.Compose(ParentComponent->GetComponentTransform());
    }

    return Transform;
}

FMatrix USceneComponent::GetComponentToWorld() const {
    return GetComponentTransform().ToMatrixWithScale();
}

FVector3 USceneComponent::GetComponentLocation() const {
    return GetComponentTransform().GetPosition();
}

FRotator USceneComponent::GetComponentRotation() const {
    return GetComponentTransform().GetRotation();
}

FVector3 USceneComponent::GetComponentScale() const {
    return GetComponentTransform().GetScale();
}

bool USceneComponent::ResolveLoadedReferences() {
    if (!UActorComponent::ResolveLoadedReferences()) {
        return false;
    }

    if (!PendingParentGuid.IsValid()) {
        return true;
    }

    UObject* ResolvedObject = UObjectSystem::Resolve(UObjectSystem::FindHandleByGuid(PendingParentGuid));
    if (ResolvedObject == nullptr ||
        !ResolvedObject->GetTypeInfo()->IsA(USceneComponent::StaticTypeInfo())) {
        return false;
    }

    USceneComponent* ParentComponent = static_cast<USceneComponent*>(ResolvedObject);
    if (!AttachToComponent(ParentComponent)) {
        return false;
    }

    PendingParentGuid = {};
    return true;
}



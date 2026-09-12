#include "PCH.h"

#include "USceneComponent.h"

FTransform& USceneComponent::GetTransform()
{
    return Transform;
}

const FTransform& USceneComponent::GetTransform() const
{
    return Transform;
}

void USceneComponent::Serialize(FArchive& Archive)
{
    UActorComponent::Serialize(Archive);

    Archive.SerializeStruct("Transform", Transform);
}


void USceneComponent::OnUnregister()
{
    for (TObjectRef<USceneComponent>& ChildRef : Children)
    {
        if (USceneComponent* Child = ChildRef.Get())
        {
            Child->Parent.Reset();
        }
    }
    Children.clear();

    if (USceneComponent* ParentComponent = Parent.Get())
    {
        ParentComponent->RemoveChild(this);
    }
    Parent.Reset();

    UActorComponent::OnUnregister();
}

void USceneComponent::RemoveChild(USceneComponent* InChild)
{
    if (!InChild) return;

    std::erase_if(Children,
        [InChild](const TObjectRef<USceneComponent>& ChildRef)
        {
            return ChildRef.Get() == InChild;
        });

    if (InChild->Parent.Get() == this)
    {
        InChild->Parent.Reset();
    }
}

void USceneComponent::AttachTo(USceneComponent* InParent)
{
    if (InParent == this || Parent.Get() == InParent)
    {
        return;
    }

    for (USceneComponent* Ancestor = InParent;
        Ancestor != nullptr;
        Ancestor = Ancestor->GetParent())
    {
        if (Ancestor == this)
        {
            return;
        }
    }

    if (USceneComponent* PreviousParent = Parent.Get())
    {
        PreviousParent->RemoveChild(this);
    }

    Parent.Set(InParent);

    if (InParent != nullptr)
    {
        InParent->Children.emplace_back(this);
    }
}

USceneComponent* USceneComponent::GetParent() const
{
    return Parent.Get();
}

const std::vector<TObjectRef<USceneComponent>>& USceneComponent::GetChildren() const
{
    return Children;
}

FMatrix USceneComponent::GetWorldMatrix() const
{
    FMatrix LocalMatrix = Transform.GetWorldMatrix();

    USceneComponent* ParentComponent = Parent.Get();
    if (ParentComponent == nullptr)
    {
        return LocalMatrix;
    }

    return LocalMatrix * ParentComponent->GetWorldMatrix();
}



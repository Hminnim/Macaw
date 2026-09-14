#include "PCH.h"
#include "UPrimitiveComponent.h"

bool UPrimitiveComponent::IsVisible() const {
    return bVisible;
}

void UPrimitiveComponent::SetVisible(bool bInVisible) {
    bVisible = bInVisible;
}

void UPrimitiveComponent::Serialize(FArchive& Archive) {
    USceneComponent::Serialize(Archive);

    Archive.Serialize("bVisible", bVisible);
}
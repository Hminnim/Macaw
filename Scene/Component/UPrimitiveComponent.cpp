#include "PCH.h"
#include "UPrimitiveComponent.h"
#include "Render/Panel/FPropertyEditorContext.h"

#include "Scene/AActor.h"
#include "Scene/Subsystem/UPickingSubsystem.h"
#include "Scene/UWorld.h"

bool UPrimitiveComponent::IsVisible() const {
    return bVisible;
}

void UPrimitiveComponent::SetVisible(bool bInVisible) {
    bVisible = bInVisible;
}

void UPrimitiveComponent::OnRegister() {
    USceneComponent::OnRegister();

    AActor* Owner = GetOwner();
    if (Owner != nullptr && Owner->GetWorld() != nullptr) {
        Owner->GetWorld()->GetPickingSubsystem().RegisterComponent(this);
    }
}

void UPrimitiveComponent::OnUnregister() {
    AActor* Owner = GetOwner();
    if (Owner != nullptr && Owner->GetWorld() != nullptr) {
        Owner->GetWorld()->GetPickingSubsystem().UnregisterComponent(this);
    }

    USceneComponent::OnUnregister();
}

void UPrimitiveComponent::Serialize(FArchive& Archive) {
    USceneComponent::Serialize(Archive);

    Archive.Serialize("bVisible", bVisible);
}

void UPrimitiveComponent::DrawPanels(FPropertyEditorContext& Context) {
    USceneComponent::DrawPanels(Context);
    Context.DrawBool("Visible", IsVisible(), [this](bool bVisible) {
        SetVisible(bVisible);
    });
}

#include "PCH.h"
#include "UPrimitiveComponent.h"
#include "Render/Panel/FPropertyEditorContext.h"

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

void UPrimitiveComponent::DrawPanels(FPropertyEditorContext& Context) {
    USceneComponent::DrawPanels(Context);
    Context.DrawBool("Visible", IsVisible(), [this](bool bVisible) {
        SetVisible(bVisible);
    });
}

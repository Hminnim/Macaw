#include "PCH.h"
#include "UBillboardComponent.h"

bool UBillboardComponent::CanRenderBillBoard() const
{
    return IsActive() && IsVisible();
}

bool UBillboardComponent::TryGetBillBoardWorld(FMatrix& OutWorld) const
{
    if (!CanRenderBillBoard())
    {
        return false;
    }

    OutWorld = GetComponentToWorld();
    return true;
}
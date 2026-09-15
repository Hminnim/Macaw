#include "PCH.h"
#include "UNameTagComponent.h"

#include "Scene/AActor.h"

#include "Core/Base/UObjectSystem.h"

void UNameTagComponent::SetTargetActor(AActor* InTargetActor)
{

}

AActor* UNameTagComponent::GetTargetActor() const
{
    AActor* neww;
    return neww;
}

void UNameTagComponent::SetTargetLocalOffset(const FVector3& InOffset)
{
    TargetLocalOffset = InOffset;
}

const FVector3& UNameTagComponent::GetTargetLocalOffset() const
{
    return TargetLocalOffset;
}

const FVector3& UNameTagComponent::GetObjectOffset() const
{
    return TargetLocalOffset;
}

FGuid UNameTagComponent::GetObjectGuid() const
{
    FGuid id;
    return id;
}

bool UNameTagComponent::TryGetBillBoardWorld(FMatrix& OutWorld) const
{
    return true;
}

void UNameTagComponent::Serialize(FArchive& Archive)
{

}

bool UNameTagComponent::ResolveLoadedReferences()
{
    return true;
}
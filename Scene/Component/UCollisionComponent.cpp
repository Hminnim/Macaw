#include "PCH.h"
#include "UCollisionComponent.h"
#include "Render/Panel/FPropertyEditorContext.h"
#include "UMeshComponent.h"
#include "Scene/AActor.h"
#include "Scene/UWorld.h"
#include "Scene/Subsystem/UCollisionSubsystem.h"
#include "Core/Asset/UMesh.h"
#include "Core/Asset/FAssetRegistry.h"

#include "../../Core/Console/Console.h"


bool UCollisionComponent::IsCollisionEnabled() const
{
    return bCollisionEnabled;
}

void UCollisionComponent::SetCollisionEnabled(bool bEnabled)
{
    bCollisionEnabled = bEnabled;
}

void UCollisionComponent::DrawPanels(FPropertyEditorContext& Context)
{
    UPrimitiveComponent::DrawPanels(Context);
    Context.DrawBool("Collision Enabled", IsCollisionEnabled(), [this](bool bEnabled) {
        SetCollisionEnabled(bEnabled);
    });
}

void UCollisionComponent::OnRegister()
{
    AActor* Owner = GetOwner();

    if (Owner != nullptr && Owner->GetWorld() != nullptr)
    {
        Owner->GetWorld()->GetCollisionSubsystem().RegisterComponent(this);
    }
}

void UCollisionComponent::OnUnregister() {
    AActor* Owner = GetOwner();

    if (Owner != nullptr && Owner->GetWorld() != nullptr) {
        Owner->GetWorld()->GetCollisionSubsystem().UnregisterComponent(this);
    }

    UPrimitiveComponent::OnUnregister();
}

bool UCollisionComponent::Raycast(const FRay& Ray, float& OutDistance) const {
    if (!bCollisionEnabled || !RaycastBounds(Ray, OutDistance)) {
        return false;
    }

    UMeshComponent* Mesh = GetMeshComponent();
    return Mesh == nullptr || Mesh->RaycastMesh(Ray, OutDistance);
}

void UCollisionComponent::MakeRender(FActorProbe& Probe) const
{
}

void UCollisionComponent::Serialize(FArchive& Archive) {
    UPrimitiveComponent::Serialize(Archive);

    Archive.Serialize("bCollisionEnabled", bCollisionEnabled);
}

#include "PCH.h"
#include "UStaticMeshComponent.h"

#include "Core/Base/FRenderProbe.h"
#include "Scene/AActor.h"
#include "Scene/UWorld.h"
#include "../../Serialize/FArchive.h"
#include "../../Core/Asset/FAssetRegistry.h"

FAssetHandle UStaticMeshComponent::GetMeshHandle() const { return MeshHandle; }

FAssetHandle UStaticMeshComponent::GetMaterialHandle() const { return MaterialHandle; }

FAssetHandle UStaticMeshComponent::GetPipelineHandle() const { return PipelineHandle; }

void UStaticMeshComponent::SetMeshHandle(FAssetHandle InHandle)
{
    MeshHandle = InHandle;
}

void UStaticMeshComponent::SetMaterialHandle(FAssetHandle InHandle)
{
    MaterialHandle = InHandle;
}

void UStaticMeshComponent::SetPipelineHandle(FAssetHandle InHandle)
{
    PipelineHandle = InHandle;
}

void UStaticMeshComponent::OnRegister()
{
    AActor* Owner = GetOwner();

    if (Owner != nullptr && Owner->GetWorld() != nullptr)
    {
        Owner->GetWorld()->RegisterRenderable(this);
    }
}

void UStaticMeshComponent::OnUnregister()
{
    AActor* Owner = GetOwner();

    if (Owner != nullptr && Owner->GetWorld() != nullptr)
    {
        Owner->GetWorld()->UnregisterRenderable(this);
    }

    UPrimitiveComponent::OnUnregister();
}

void UStaticMeshComponent::MakeRender(FActorProbe& OutProbe) const
{
    if (!IsActive() || !IsVisible())
    {
        return;
    }

    OutProbe = FActorProbe{
        GetComponentToWorld(),
        MeshHandle,
        MaterialHandle,
        PipelineHandle,
		0x0000'0000
    };
}


void UStaticMeshComponent::Serialize(FArchive& Archive)
{
    UPrimitiveComponent::Serialize(Archive);

    FString GuidMeshHandle;
    if (MeshHandle.ID != std::numeric_limits<uint32>::max())
        GuidMeshHandle = Archive.GetAssetRegistry()->ResolveAsset<UAsset>(MeshHandle)->GetGuid().ToString();
    Archive.Serialize("GuidMeshHandle", GuidMeshHandle);
    if (Archive.IsLoading())
    {
        FGuid Guid;
        Guid.Parse(GuidMeshHandle);

        MeshHandle = Archive.GetAssetRegistry()->GetAsset(Guid);
    }

    FString GuidMaterialHandle;
    if (MaterialHandle.ID != std::numeric_limits<uint32>::max())
        GuidMaterialHandle = Archive.GetAssetRegistry()->ResolveAsset<UAsset>(MaterialHandle)->GetGuid().ToString();
    Archive.Serialize("GuidMaterialHandle", GuidMaterialHandle);
    if (Archive.IsLoading())
    {
        FGuid Guid;
        Guid.Parse(GuidMaterialHandle);

        MaterialHandle = Archive.GetAssetRegistry()->GetAsset(Guid);
    }

    FString GuidPipelineHandle;
    if (PipelineHandle.ID != std::numeric_limits<uint32>::max())
        GuidPipelineHandle = Archive.GetAssetRegistry()->ResolveAsset<UAsset>(PipelineHandle)->GetGuid().ToString();
    Archive.Serialize("GuidPipelineHandle", GuidPipelineHandle);
    if (Archive.IsLoading())
    {
        FGuid Guid;
        Guid.Parse(GuidPipelineHandle);

        PipelineHandle = Archive.GetAssetRegistry()->GetAsset(Guid);
    }
}

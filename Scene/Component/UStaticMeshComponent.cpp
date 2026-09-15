#include "PCH.h"
#include "UStaticMeshComponent.h"
#include "Render/Panel/FPropertyEditorContext.h"

#include "Core/Base/FRenderProbe.h"
#include "Scene/AActor.h"
#include "Scene/UWorld.h"
#include "Scene/Subsystem/URenderSubsystem.h"
#include "../../Serialize/FArchive.h"
#include "../../Core/Asset/FAssetRegistry.h"
#include "Core/Asset/UMaterial.h"
#include "Render/Pipeline/UPipeline.h"

FAssetHandle UStaticMeshComponent::GetMaterialHandle() const { return MaterialHandle; }

FAssetHandle UStaticMeshComponent::GetPipelineHandle() const { return PipelineHandle; }

void UStaticMeshComponent::SetMaterialHandle(FAssetHandle InHandle)
{
    MaterialHandle = InHandle;
    EnsureDefaultRenderAssets();
}

void UStaticMeshComponent::SetPipelineHandle(FAssetHandle InHandle)
{
    PipelineHandle = InHandle;
    EnsureDefaultRenderAssets();
}

void UStaticMeshComponent::DrawPanels(FPropertyEditorContext& Context)
{
    UMeshComponent::DrawPanels(Context);
    AActor* Owner = GetOwner();
    UWorld* World = Owner != nullptr ? Owner->GetWorld() : nullptr;
    FAssetRegistry* Registry = World != nullptr ? World->GetAssetRegistry() : nullptr;
    if (Registry == nullptr) {
        Context.DrawDisabledText("Material/Pipeline: Asset registry unavailable");
        return;
    }
    Context.DrawAssetPicker("Material", *Registry, *UMaterial::StaticTypeInfo(), GetMaterialHandle(), [this](FAssetHandle Handle) {
        SetMaterialHandle(Handle);
    });
    Context.DrawAssetPicker("Pipeline", *Registry, *UPipeline::StaticTypeInfo(), GetPipelineHandle(), [this](FAssetHandle Handle) {
        SetPipelineHandle(Handle);
    });
}

void UStaticMeshComponent::OnRegister()
{
    UMeshComponent::OnRegister();

    EnsureDefaultRenderAssets();

    AActor* Owner = GetOwner();

    if (Owner != nullptr && Owner->GetWorld() != nullptr)
    {
        Owner->GetWorld()->GetRenderSubsystem().RegisterComponent(this);
    }
}

void UStaticMeshComponent::EnsureDefaultRenderAssets()
{
    AActor* Owner = GetOwner();
    UWorld* World = Owner != nullptr ? Owner->GetWorld() : nullptr;
    FAssetRegistry* Registry = World != nullptr ? World->GetAssetRegistry() : nullptr;
    if (Registry == nullptr) {
        return;
    }

    if (Registry->ResolveAsset<UMaterial>(MaterialHandle) == nullptr) {
        MaterialHandle = Registry->EnsureDefaultStaticMeshMaterial();
    }

    if (Registry->ResolveAsset<UPipeline>(PipelineHandle) == nullptr) {
        PipelineHandle = Registry->EnsureDefaultStaticMeshPipeline();
    }
}

void UStaticMeshComponent::OnUnregister()
{
    AActor* Owner = GetOwner();

    if (Owner != nullptr && Owner->GetWorld() != nullptr)
    {
        Owner->GetWorld()->GetRenderSubsystem().UnregisterComponent(this);
    }

    UMeshComponent::OnUnregister();
}

void UStaticMeshComponent::MakeRender(FActorProbe& OutProbe) const
{
    if (!IsActive() || !IsVisible())
    {
        return;
    }

    OutProbe = FActorProbe{
        GetComponentToWorld(),
        GetMeshHandle(),
        MaterialHandle,
        PipelineHandle,
		0x0000'0000
    };
}


void UStaticMeshComponent::Serialize(FArchive& Archive)
{
    UMeshComponent::Serialize(Archive);

    FString GuidMaterialHandle;
    if (MaterialHandle.ID != std::numeric_limits<uint32>::max()) {
        if (UAsset* Asset = Archive.GetAssetRegistry()->ResolveAsset<UAsset>(MaterialHandle)) {
            GuidMaterialHandle = Asset->GetGuid().ToString();
        }
    }

    Archive.Serialize("GuidMaterialHandle", GuidMaterialHandle);
    if (Archive.IsLoading())
    {
        FGuid Guid;
        if (Guid.Parse(GuidMaterialHandle)) {
            MaterialHandle = Archive.GetAssetRegistry()->GetAsset(Guid);
        }
    }

    FString GuidPipelineHandle;
    if (PipelineHandle.ID != std::numeric_limits<uint32>::max()) {
        if (UAsset* Asset = Archive.GetAssetRegistry()->ResolveAsset<UAsset>(PipelineHandle)) {
            GuidPipelineHandle = Asset->GetGuid().ToString();
        }
    }
    Archive.Serialize("GuidPipelineHandle", GuidPipelineHandle);
    if (Archive.IsLoading())
    {
        FGuid Guid;
        if (Guid.Parse(GuidPipelineHandle)) {
            PipelineHandle = Archive.GetAssetRegistry()->GetAsset(Guid);
        }
    }
}

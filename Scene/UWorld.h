#pragma once

#include <filesystem>
#include <memory>
#include <optional>

#include <d3d11.h>

#include "AActor.h"
#include "Component/UCameraComponent.h"
#include "Component/UStaticMeshComponent.h"
#include "Component/UCollisionComponent.h"
#include "Core/Asset/FAssetRegistry.h"
#include "Core/Asset/UMesh.h"
#include "Core/Base/TObjectRef.h"
#include "Core/Channel/FStateChannel.h"

#include "Common.h"
#include "Core/Base/UObject.h"
#include "Core/Base/UObjectSystem.h"
#include "Core/Base/FRenderProbe.h"
#include "FWorldEditorContext.h"
#include "FKeyboardCameraMoveRequestMessage.h"
#include "FMouseCameraRotateRequestMessage.h"
#include "FMousePickRequestMessage.h"
#include "Render/Panel/FEditorInfo.h"

#include "../Render/RenderWindowInfo.h"

#include "../Serialize/FEditorConfigManager.h"

class AActor;
class UCameraComponent;
class UStaticMeshComponent;
struct ID3D11Device;
class FAssetRegistry;
class UCameraSubsystem;
class UCollisionSubsystem;
class UPickingSubsystem;
class URenderSubsystem;
class UBillboardSubsystem;
class UTextSubsystem;
class ULightSubsystem;

class UWorld : public UObject
{
public:
    UWorld();
    ~UWorld() override;

    AActor* AddActor(std::unique_ptr<AActor> InActor);

    template<typename T>
    requires std::is_base_of_v<AActor, T>
    T* AdoptActor() {
        std::unique_ptr<T> NewActor = std::make_unique<T>();

        T* ActorPtr = NewActor.get();

        if (AddActor(std::move(NewActor)) == nullptr) {
            return nullptr;
        }

        ActorPtr->SetName(MakeUniqueObjectName(ActorPtr->GetTypeInfo()->TypeName));

        return ActorPtr;
    }

    bool SpawnActor(const FAssetHandle& MeshHandle, const FAssetHandle& PipelineHandle, const FAssetHandle& MaterialHandle, const FVector3& Position);
    bool DestroyActor(AActor* Actor);
    void FlushPendingDestroyActors();

    const TArray<std::unique_ptr<AActor>>& GetActors() const;
    FRenderProbe& BuildRenderProbe();
    
    void SetEditorContext(FWorldEditorContext* InEditorContext);
    FWorldEditorContext* GetEditorContext() const noexcept;

    void Tick(float DeltaTime);

    URenderSubsystem& GetRenderSubsystem();
    const URenderSubsystem& GetRenderSubsystem() const;
    UCollisionSubsystem& GetCollisionSubsystem();
    const UCollisionSubsystem& GetCollisionSubsystem() const;
    UPickingSubsystem& GetPickingSubsystem();
    const UPickingSubsystem& GetPickingSubsystem() const;
    UCameraSubsystem& GetCameraSubsystem();
    const UCameraSubsystem& GetCameraSubsystem() const;
    UBillboardSubsystem& GetBillboardSubsystem();
    const UBillboardSubsystem& GetBillboardSubsystem() const;

    UTextSubsystem& GetTextSubsystem();
    const UTextSubsystem& GetTextSubsystem() const;
    ULightSubsystem& GetLightSubsystem();
    const ULightSubsystem& GetLightSubsystem() const;

    bool SaveScene(const FString& SceneName, FAssetRegistry* AssetRegistry);
    bool LoadScene(const std::filesystem::path& ScenePath, ID3D11Device* Device, FAssetRegistry* AssetRegistry);

	JG_DECLARE_DERIVED_TYPEINFO(UWorld, UObject);

    void HandleMousePickRequest(const FMousePickRequestMessage& Message);
    void HandleMouseCameraRotateRequest(const FMouseCameraRotateRequestMessage& Message);
    void HandleKeyboardCameraMoveRequest(const FKeyboardCameraMoveRequestMessage& Message);
    void HandleSpawnComponent(const FMessageSpawnComponent& Message, FAssetRegistry& AssetRegistry);

	void UpdateEditorCameraState();
    void SetAssetRegistry(FAssetRegistry* InAssetRegistry);
	void SetWindowInfoReader(FStateChannel<RenderWindowInfo>::FReader InReader) { WindowInfoReader = InReader; }

    FAssetRegistry* GetAssetRegistry() const;

    void ResetWorld(FAssetRegistry* AssetRegistry, ID3D11Device* Device);

    FName MakeUniqueObjectName(std::string_view SourceName);
    AActor* FindActorByName(FName InName) const;

    FEditorSettings& GetSettings() { return Settings; }
private:
	void InitializeSubsystems();
	void DeinitializeSubsystems();

    void PublishEditorCameraState();

private:
    TArray<std::unique_ptr<AActor>> Actors;
    TArray<AActor*> PendingDestroyActors;
   
    TArray<UStaticMeshComponent*> RenderableComponents;
    TArray<TObjectRef<UCollisionComponent>> CollisionComponents;

	FStateChannel<RenderWindowInfo>::FReader WindowInfoReader;

    FWorldEditorContext* EditorContext{ nullptr };
    FAssetRegistry* AssetRegistry{ nullptr };

    std::unique_ptr<URenderSubsystem> RenderSubsystem;
    std::unique_ptr<UCollisionSubsystem> CollisionSubsystem;
    std::unique_ptr<UPickingSubsystem> PickingSubsystem;
    std::unique_ptr<UCameraSubsystem> CameraSubsystem;

    std::unique_ptr<UBillboardSubsystem> BillboardSubsystem;
	std::unique_ptr<UTextSubsystem> TextSubsystem;
    std::unique_ptr<ULightSubsystem> LightSubsystem;

    FRenderProbe Probe{};

    FEditorSettings Settings;
};

#pragma once

#include <filesystem>
#include <memory>
#include <optional>

#include <d3d11.h>

#include "AActor.h"
#include "Component/UCameraComponent.h"
#include "Component/UStaticMeshComponent.h"
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

#include "Folder.h"

class AActor;
class UCameraComponent;
class UStaticMeshComponent;
class UBillboardTextComponent;
struct ID3D11Device;
class FAssetRegistry;
class UCameraSubsystem;
class UCollisionSubsystem;
class URenderSubsystem;

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

        return ActorPtr;
    }

    bool SpawnActor(const FAssetHandle& MeshHandle, const FAssetHandle& PipelineHandle, const FAssetHandle& MaterialHandle, const FVector3& Position);
    bool DestroyActor(AActor* Actor);
    void FlushPendingDestroyActors();

    const TArray<std::unique_ptr<AActor>>& GetActors() const;
    Folder* CreateFolder(FString InName, FGuid InParentFolderGuid = {});
    bool DestroyFolder(FGuid FolderGuid);
    bool SetFolderParent(FGuid FolderGuid, FGuid InParentFolderGuid);
    Folder* FindFolder(FGuid FolderGuid);
    const Folder* FindFolder(FGuid FolderGuid) const;
    const TArray<std::unique_ptr<Folder>>& GetFolders() const;
    bool SetActorFolder(AActor* Actor, FGuid FolderGuid);
    FRenderProbe& BuildRenderProbe();
    
    void SetEditorContext(FWorldEditorContext* InEditorContext);
    FWorldEditorContext* GetEditorContext() const noexcept;

    void Tick(float DeltaTime);

    URenderSubsystem& GetRenderSubsystem();
    const URenderSubsystem& GetRenderSubsystem() const;
    UCollisionSubsystem& GetCollisionSubsystem();
    const UCollisionSubsystem& GetCollisionSubsystem() const;
    UCameraSubsystem& GetCameraSubsystem();
    const UCameraSubsystem& GetCameraSubsystem() const;

    bool SaveScene(const FString& SceneName, FAssetRegistry* AssetRegistry);
    bool LoadScene(const std::filesystem::path& ScenePath, ID3D11Device* Device, FAssetRegistry* AssetRegistry);

	JG_DECLARE_DERIVED_TYPEINFO(UWorld, UObject);

    void HandleMousePickRequest(const FMousePickRequestMessage& Message);
    void HandleMouseCameraRotateRequest(const FMouseCameraRotateRequestMessage& Message);
    void HandleKeyboardCameraMoveRequest(const FKeyboardCameraMoveRequestMessage& Message);
    void HandleSpawnPrimitive(const FMessageSpawnPrimitive& Message, FAssetRegistry& AssetRegistry);

    void RegisterBillboardText(UBillboardTextComponent* Component);
    void UnregisterBillboardText(UBillboardTextComponent* Component);

	void UpdateEditorCameraState();
    void SetAssetRegistry(FAssetRegistry* InAssetRegistry);
	void SetWindowInfoReader(FStateChannel<RenderWindowInfo>::FReader InReader) { WindowInfoReader = InReader; }

    FAssetRegistry* GetAssetRegistry() const;

    void ResetWorld(FAssetRegistry* AssetRegistry, ID3D11Device* Device);

private:
	void InitializeSubsystems();
	void DeinitializeSubsystems();
    bool AdoptFolder(std::unique_ptr<Folder> InFolder);
    bool ValidateFolderHierarchy() const;

    void PublishEditorCameraState();
private:
    TArray<std::unique_ptr<Folder>> Folders;
    TArray<std::unique_ptr<AActor>> Actors;
    TArray<AActor*> PendingDestroyActors;
   
    TArray<UStaticMeshComponent*> RenderableComponents;
    TArray<TObjectRef<UCollisionComponent>> CollisionComponents;
    TArray<UBillboardTextComponent*> TextComponents{};

	FStateChannel<RenderWindowInfo>::FReader WindowInfoReader;

    FWorldEditorContext* EditorContext{ nullptr };
    FAssetRegistry* AssetRegistry{ nullptr };

    std::unique_ptr<URenderSubsystem> RenderSubsystem;
    std::unique_ptr<UCollisionSubsystem> CollisionSubsystem;
    std::unique_ptr<UCameraSubsystem> CameraSubsystem;
	// std::unique_ptr<TextRenderSubSystem> TextRenderSubsystem;

    FRenderProbe Probe{};
};

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
#include "FKeyboardCameraMoveRequestMessage.h"
#include "FMouseCameraRotateRequestMessage.h"
#include "FMousePickRequestMessage.h"
#include "FTransformEditRequestMessage.h"
#include "Render/Panel/FEditorInfo.h"

#include "../Render/RenderWindowInfo.h"

class UCameraSubsystem;
class UCollisionSubsystem;
class URenderSubsystem;
class FWorldEditorContext;

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

    bool SpawnActor(const FAssetHandle& MeshHandle, const FAssetHandle& PipelineHandle, const FAssetHandle& MaterialHandle,
        const FVector3& Position);
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
    UCameraSubsystem& GetCameraSubsystem();
    const UCameraSubsystem& GetCameraSubsystem() const;

    bool SaveScene(const FString& SceneName, FAssetRegistry* AssetRegistry);
    bool LoadScene(const std::filesystem::path& ScenePath, ID3D11Device* Device, FAssetRegistry* AssetRegistry);

	JG_DECLARE_DERIVED_TYPEINFO(UWorld, UObject);

    void InitializeEditorCameraState(
        FStateChannel<FMessageEditorCameraState>::FWriter InWriter,
        FStateChannel<FMessageEditorCameraState>::FReader InReader);

    void HandleMousePickRequest(const FMousePickRequestMessage& Message);

	void HandleMousePickReleaseRequest(const FMousePickReleaseRequestMessage& Message);

	void HandleTransformEditRequest(const FTransformEditRequestMessage& Message);

    void HandleMouseCameraRotateRequest(const FMouseCameraRotateRequestMessage& Message);

    void HandleKeyboardCameraMoveRequest(const FKeyboardCameraMoveRequestMessage& Message);

    void HandleSpawnPrimitive(const FMessageSpawnPrimitive& Message, FAssetRegistry& AssetRegistry);
    void HandleNewScene(const FMessageNewScene& Message);
    void HandleLoadScene(const FMessageLoadScene& Message);

    void HandleChangeGizmoMode(const FMessageChangeGizmoMode& Message);

    std::optional<FStateChannel<FMessageEditorCameraState>::FWriter> EditorCameraWriter;
    std::optional<FStateChannel<FMessageEditorCameraState>::FReader> EditorCameraReader;

	void UpdateEditorCameraState();
    void SetAssetRegistry(FAssetRegistry* InAssetRegistry);
	void SetWindowInfoReader(FStateChannel<RenderWindowInfo>::FReader InReader) { WindowInfoReader = InReader; }

    FAssetRegistry* GetAssetRegistry() const;

    void ResetWorld(FAssetRegistry* AssetRegistry, ID3D11Device* Device);

private:
	struct FActiveTransformEdit {
		std::uint64_t SessionId = 0;
		FObjectHandle TargetHandle{};
		FMatrix OriginalWorld{ FMatrix::Identity };
	};

	void InitializeSubsystems();
	void DeinitializeSubsystems();

    TArray<std::unique_ptr<AActor>> Actors;
    TArray<AActor*> PendingDestroyActors;


	FStateChannel<RenderWindowInfo>::FReader WindowInfoReader;

	std::optional<FActiveTransformEdit> ActiveTransformEdit;
	std::uint64_t TransformRevision = 1;

    FWorldEditorContext* EditorContext = nullptr;

    FAssetRegistry* AssetRegistry = nullptr;

    std::unique_ptr<URenderSubsystem> RenderSubsystem;
    std::unique_ptr<UCollisionSubsystem> CollisionSubsystem;
    std::unique_ptr<UCameraSubsystem> CameraSubsystem;
    FRenderProbe Probe{};

    void ApplyEditorCameraState();
    void PublishEditorCameraState();

    std::optional<FStateChannel<FMessageEditorCameraState>::FWriter>
        EditorCameraStateWriter;

    std::optional<FStateChannel<FMessageEditorCameraState>::FReader>
        EditorCameraStateReader;
};

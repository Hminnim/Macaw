#include "PCH.h"
#include "UWorld.h"

#include <algorithm>
#include <random>

#include "AActor.h"
#include "Component/UCameraComponent.h"
#include "Component/UStaticMeshComponent.h"
#include "Component/UCollisionComponent.h"
#include "FMouseCameraRotateRequestMessage.h"
#include "FMousePickRequestMessage.h"
#include "FWorldSelectionChangedMessage.h"
#include "FKeyboardCameraMoveRequestMessage.h"
#include "FTransformEditRequestMessage.h"
#include "Render/Panel/FEditorInfo.h"
#include "Core/Asset/UMesh.h"

#include "../Serialize/FArchiveJson.h"
#include "../Core/Base/TypeRegistry.h"
#include "../Core/Base/UObjectSystem.h"
#include "../Core/Asset/FAssetRegistry.h"
#include "../Core/Console/Console.h"

#include <d3d11.h>
#include <filesystem>
#include <fstream>
#include <ranges>
#include <rapidjson/document.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>

namespace {
	bool ApplyWorldMatrix(USceneComponent& Component, const FMatrix& DesiredWorld) {
		FMatrix LocalMatrix = DesiredWorld;
		if (USceneComponent* Parent = Component.GetParent()) {
			FMatrix parentInverse;
            if (!Parent->GetWorldMatrix().TryInverse(parentInverse)) return false;
            LocalMatrix = DesiredWorld * parentInverse;
		}

        
        const FVector3 LocalTranslation = LocalMatrix.Translation();

        LocalMatrix.Translation(FVector3::Zero);

        FMatrix AxisConversionInverse;

        if (!FMatrix::CreateYUpToZUp().TryInverse(AxisConversionInverse))
        {
            return false;
        }

        LocalMatrix = LocalMatrix * AxisConversionInverse;

        LocalMatrix.Translation(LocalTranslation);
        

        FVector3 Scale{};
        FQuat Rotation{};
        FVector3 Translation{};
		if (!LocalMatrix.Decompose(Scale, Rotation, Translation)) {
			return false;
		}

		FTransform& Transform = Component.GetTransform();
		Transform.SetPosition(Translation);
		Transform.SetRotation(FVector3(Rotation.ToEuler()));
		Transform.SetScale(Scale);
		return true;
	}
}

std::string GetFilePathFromExplorer()
{
    return "./scenes/test.json";
}

UWorld::~UWorld()
{
    for (const std::unique_ptr<AActor>& Actor : Actors)
    {
        UObjectSystem::Unregister (Actor.get(), Actor->GetHandle());
    }

    Actors.clear();
}

bool UWorld::SpawnActor(const FAssetHandle& MeshHandle, const FAssetHandle& PipelineHandle, const FAssetHandle& MaterialHandle, 
                        const FVector3& Position, UMesh *Mesh, FAssetRegistry* AssetRegistry)
{
    //std::unique_ptr<AActor> Actor;
    auto Actor = UWorld::AdoptActor<AActor>();

    UStaticMeshComponent* MeshComponent = Actor->AddComponent<UStaticMeshComponent>();
    UCollisionComponent* CollisionComponent = Actor->AddComponent<UCollisionComponent>();

    Actor->SetRootComponent(MeshComponent);

    CollisionComponent->AttachTo(MeshComponent);

    MeshComponent->SetMeshHandle(MeshHandle);
    MeshComponent->SetPipelineHandle(PipelineHandle);
    MeshComponent->SetMaterialHandle(MaterialHandle);

    MeshComponent->GetTransform().SetPosition(
        FVector3{
            Position.x,
            Position.y,
            Position.z
        });

    CollisionComponent->SetBounds(Mesh->GetLocalBoundingBox());
    FGuid Guid = Actor->GetGuid();
    
    //FUndoSystem::RecordObject(UObjectSystem::Resolve(UObjectSystem::FindHandleByGuid(Guid)), EUndoType::Spawn, AssetRegistry);

    return true;
}

bool UWorld::DestroyActor(AActor* Actor)
{
    if (Actor == nullptr)
    {
        return false;
    }

    auto It = std::ranges::find_if(Actors, [Actor](const std::unique_ptr<AActor>& Ptr)
    {
        return Ptr.get() == Actor;
    });

    if (It == Actors.end())
    {
        return false;
    }

    PendingDestroyActors.push_back(Actor);
    return true;
}

void UWorld::FlushPendingDestroyActors()
{
    for (AActor* Actor : PendingDestroyActors)
    {
        if (Actor == nullptr)
        {
            continue;
        }

        auto It = std::ranges::find_if(Actors, [Actor](const std::unique_ptr<AActor>& Ptr)
        {
            return Ptr.get() == Actor;
        });

        if (It == Actors.end())
        {
            continue;
        }

        UObjectSystem::Unregister(
            Actor,
            Actor->GetHandle());

        Actors.erase(It); 
    }

    PendingDestroyActors.clear();
}

const TArray<std::unique_ptr<AActor>>& UWorld::GetActors() const
{
    return Actors;
}

void UWorld::InitializeEditorEventSender(
    FMessageChannel::FSender&& InSender)
{
    EditorEventSender.emplace(std::move(InSender));
}

void UWorld::InitializeEditorCameraState(
    FStateChannel<FMessageEditorCameraState>::FWriter InWriter,
    FStateChannel<FMessageEditorCameraState>::FReader InReader)
{
    EditorCameraStateWriter.emplace(std::move(InWriter));
    EditorCameraStateReader.emplace(std::move(InReader));

    if (Camera != nullptr)
    {
        PublishEditorCameraState();
    }
}

FRenderProbe& UWorld::BuildRenderProbe() 
{
	Probe.ActorProbes.clear();
    Probe.GizmoProbes.clear();

    for (const UStaticMeshComponent* Component : RenderableComponents)
    {
		FActorProbe ActorProbe{};
		Component->MakeRender(ActorProbe);

		
		UCollisionComponent* SelectedCollision = SelectedCollider.Get();
		if (SelectedCollision != nullptr && Component->GetOwner() == SelectedCollision->GetOwner()) {
			ActorProbe.Flags |= 0x0000'0001; 
		}

		Probe.ActorProbes.push_back(ActorProbe);
    }

    if (Camera != nullptr)
    {
        Probe.MainCameraProbe.View =
            Camera->GetViewMatrix();

        Probe.MainCameraProbe.Projection =
            Camera->GetProjectionMatrix();

        Probe.MainCameraProbe.ViewProjection =
            Camera->GetViewProjectionMatrix();
    }
    return Probe;
}

void UWorld::Tick(float DeltaTime)
{
    if (WindowInfoReader.HasChanged()) {
		Camera->SetAspectRatio(static_cast<float>(WindowInfoReader.Read().ScreenWidth) / static_cast<float>(WindowInfoReader.Read().ScreenHeight));
    }

    ApplyEditorCameraState();
    PublishEditorSelectionState(); 

    for (const std::unique_ptr<AActor>& Actor : Actors)
    {
        Actor->Tick(DeltaTime);
    }

    // FlushPendingDestroyActors();
}

void UWorld::RegisterRenderable(UStaticMeshComponent* Component)
{
    if (Component == nullptr || std::ranges::find(RenderableComponents, Component) != RenderableComponents.end())
    {
        return;
    }

    RenderableComponents.push_back(Component);
}

void UWorld::UnregisterRenderable(UStaticMeshComponent* Component)
{
    std::erase(RenderableComponents, Component);
}

void UWorld::SetMainCamera(UCameraComponent* InCamera)
{
    Camera = InCamera;

    PublishEditorCameraState();
}

void UWorld::ClearMainCamera(UCameraComponent* InCamera)
{
    if (Camera == InCamera)
    {
        Camera = nullptr;
    }
}


bool UWorld::SaveScene(const FString& SceneName, FAssetRegistry* AssetRegistry)
{
    std::filesystem::path CurrentPath = std::filesystem::current_path();
    std::filesystem::path SceneDir = CurrentPath / "scenes";
    if (!std::filesystem::exists(SceneDir))
        std::filesystem::create_directories(SceneDir);
    std::filesystem::path FilePath = SceneDir / (SceneName.c_str() + std::string(".json"));

    rapidjson::Document Document;
    Document.SetObject();
    rapidjson::Document::AllocatorType& Allocator = Document.GetAllocator();


    FArchiveJson ArchiveSave(Document, Allocator);
	ArchiveSave.SetAssetRegistry(AssetRegistry);

	auto AssetList = AssetRegistry->GetAssetList();

    size_t ArraySize = static_cast<size_t>(AssetList.size());
    ArchiveSave.BeginArrayScope("Assets", ArraySize);

    for (size_t Index : std::views::iota(size_t{ 0 }, std::ranges::size(AssetList))) {
        UObject* Asset = AssetList[Index];

        ArchiveSave.BeginObjectScope(std::to_string(Index));
        Asset->Save(ArchiveSave);
        ArchiveSave.EndObjectScope();
    }

    ArchiveSave.EndArrayScope();

    ArraySize = static_cast<size_t>(Actors.size());
    ArchiveSave.BeginArrayScope("Actors", ArraySize);
    for (size_t CurrentIndex = 0, EndIndex = Actors.size(); CurrentIndex < EndIndex; ++CurrentIndex)
    {
        ArchiveSave.BeginObjectScope(std::to_string(CurrentIndex));
        Actors[CurrentIndex]->Save(ArchiveSave);
        ArchiveSave.EndObjectScope();
    }
    ArchiveSave.EndArrayScope();

    std::ofstream OutputFileStream(FilePath);
    if (!OutputFileStream.is_open())
        return false;

    rapidjson::OStreamWrapper StreamWrapper(OutputFileStream);
    rapidjson::PrettyWriter<rapidjson::OStreamWrapper> Writer(StreamWrapper);
    Document.Accept(Writer);
    OutputFileStream.close();

    return true;
}

bool UWorld::LoadScene(const std::filesystem::path& ScenePath, ID3D11Device* Device, FAssetRegistry* AssetRegistry)
{
    std::ifstream InputFileStream(ScenePath);
    if (!InputFileStream.is_open())
        return false;

    std::stringstream Buffer;
    Buffer << InputFileStream.rdbuf();
    std::string LoadedJsonString = Buffer.str();
    InputFileStream.close();

    rapidjson::Document LoadDocument;
    LoadDocument.Parse(LoadedJsonString.c_str());

    if (LoadDocument.HasParseError())
        return false;

    ResetWorld(AssetRegistry, Device);

    if (LoadDocument.HasMember("Assets") && LoadDocument["Assets"].IsArray())
    {
        for (const auto& AssetJson : LoadDocument["Assets"].GetArray())
        {
            FGuid AssetGuid;
            AssetGuid.Parse(AssetJson["Guid"].GetString());
            FString TypeName = AssetJson["TypeName"].GetString();

            FString AssetName = AssetJson["AssetName"].GetString();
            FString MetadataPath = AssetJson["AssetMetaDataPath"].GetString();

            auto EmptyAsset = TypeRegistry::Find(TypeName)->Creator();

            AssetRegistry->AdoptAsset(Device, AssetGuid, AssetName, MetadataPath, std::move(EmptyAsset));
        }
    }
    else
        return false;

    AssetRegistry->Finalize(); 

    if (LoadDocument.HasMember("Actors") && LoadDocument["Actors"].IsArray())
    {
        // ==================================================================
        // 모든 AActor 껍데기 생성 및 GUID 등록, World 설정
        // 이후 모든 액터 내부의 "컴포넌트 껍데기" 생성 및 GUID 등록
        // ==================================================================
        size_t ActorIndex = 0;

        for (auto& ActorJson : LoadDocument["Actors"].GetArray())
        {
            FGuid ActorGuid;
            ActorGuid.Parse(ActorJson["Guid"].GetString());
            FString TypeName = ActorJson["TypeName"].GetString();

            std::unique_ptr<UObject> CreatedObject = TypeRegistry::Find(TypeName)->Creator();
			std::unique_ptr<AActor> ActorPtr(static_cast<AActor*>(CreatedObject.release()));
            UObjectSystem::RegisterWithGuid(ActorPtr.get(), ActorGuid);

            FArchiveJson ArchiveLoad(static_cast<rapidjson::Value&>(ActorJson));
            ActorPtr->PreLoadComponents(ArchiveLoad);
            ActorPtr->SetWorld(this);

            Actors.emplace_back(std::move(ActorPtr));
            ++ActorIndex;
        }

        // ==================================================================
        // 진짜 직렬화
        // ==================================================================
        ActorIndex = 0; 
        for (auto& ActorJson : LoadDocument["Actors"].GetArray())
        {
            FArchiveJson ArchiveLoad(static_cast<rapidjson::Value&>(ActorJson));
            ArchiveLoad.SetAssetRegistry(AssetRegistry);
            Actors[ActorIndex]->Load(ArchiveLoad); 
            ++ActorIndex;
        }
    }
    else
        return false;

    return true;
}

void UWorld::HandleMousePickRequest(
    const FMousePickRequestMessage& Message)
{
    FObjectHandle SelectedComponentHandle{};

    if (Camera != nullptr &&
        WindowInfoReader.Read().Viewport.Width != 0 &&
        WindowInfoReader.Read().Viewport.Height != 0)
    {
        const float NdcX =
            (2.0f * static_cast<float>(Message.ScreenX) /
                static_cast<float>(WindowInfoReader.Read().Viewport.Width)) -
            1.0f;

        const float NdcY =
            1.0f -
            (2.0f * static_cast<float>(Message.ScreenY) /
                static_cast<float>(WindowInfoReader.Read().Viewport.Height));

        FMatrix InverseViewProjection;
        if (!Camera->GetViewProjectionMatrix().TryInverse(InverseViewProjection)) return;
        FVector3 RayOrigin, RayEnd;
        if (!InverseViewProjection.TransformCoord({NdcX, NdcY, 0.0f}, RayOrigin)
            || !InverseViewProjection.TransformCoord({NdcX, NdcY, 1.0f}, RayEnd)) return;
        FVector3 RayDirection = RayEnd - RayOrigin;

        if (RayDirection.LengthSquared() > 0.0f)
        {
            RayDirection.Normalize();

            float NearestDistance = std::numeric_limits<float>::max();
            UCollisionComponent* NearestCollision = nullptr;



            for (const TObjectRef<UCollisionComponent>& CollisionRef : CollisionComponents)
            {
                UCollisionComponent* CollisionComponent = CollisionRef.Get();

                if (CollisionComponent == nullptr)
                {
                    continue;
                }

				float dist = 0.0f;
                if (CollisionComponent->Raycast(FRay{ RayOrigin.ToSimpleMath(), RayDirection.ToSimpleMath() }, dist))
                {
                    if (dist < NearestDistance)
                    {
                        NearestDistance = dist;
                        NearestCollision = CollisionComponent;
                        Console::AddLog(Console::STDOutHandle, ELogLevel::Log, ELogCategory::Etc, "Raycast hit bounds of collision component %f", dist);
                    }
                }
            }

            if (NearestCollision != nullptr)
            {
				SelectedCollider.Set(NearestCollision);

				if (AActor* Owner = NearestCollision->GetOwner())
				{
					if (USceneComponent* RootComponent = Owner->GetRootComponent())
					{
						SelectedComponentHandle = RootComponent->GetHandle();
					}
				}
            }
            else {
				SelectedCollider.Reset();
				EditorSelectionState.GetWriter().Clear();
            }
        }

            
        
    }

    if (EditorEventSender.has_value())
    {
        EditorEventSender->TryEmplace<FWorldSelectionChangedMessage>(
            SelectedComponentHandle);
    }
}

void UWorld::HandleMousePickReleaseRequest(const FMousePickReleaseRequestMessage& Message)
{
   // SelectedCollider.GetWriter().Emplace(nullptr);

}

void UWorld::HandleTransformEditRequest(const FTransformEditRequestMessage& Message) {
	TObjectRef<USceneComponent> TargetRef{ Message.TargetHandle };
	USceneComponent* Target = TargetRef.Get();
	if (Target == nullptr) {
		ActiveTransformEdit.reset();
		return;
	}

	switch (Message.Phase) {
	case ETransformEditPhase::Begin:
		if (Message.ExpectedTransformRevision != TransformRevision) {
			return;
		}

		if (ActiveTransformEdit.has_value()) {
			return;
		}

		ActiveTransformEdit = FActiveTransformEdit{
			.SessionId = Message.SessionId,
			.TargetHandle = Message.TargetHandle,
			.OriginalWorld = Target->GetWorldMatrix()
		};
		break;

	case ETransformEditPhase::Update:
		if (!ActiveTransformEdit.has_value() || ActiveTransformEdit->SessionId != Message.SessionId || ActiveTransformEdit->TargetHandle != Message.TargetHandle) {
			return;
		}

		if (ApplyWorldMatrix(*Target, Message.DesiredWorld)) {
			++TransformRevision;
		}
		break;

	case ETransformEditPhase::Commit:
		if (ActiveTransformEdit.has_value() && ActiveTransformEdit->SessionId == Message.SessionId && ActiveTransformEdit->TargetHandle == Message.TargetHandle) {
			ActiveTransformEdit.reset();
		}
		break;

	case ETransformEditPhase::Cancel:
		if (ActiveTransformEdit.has_value() && ActiveTransformEdit->SessionId == Message.SessionId && ActiveTransformEdit->TargetHandle == Message.TargetHandle) {
			if (ApplyWorldMatrix(*Target, ActiveTransformEdit->OriginalWorld)) {
				++TransformRevision;
			}
			ActiveTransformEdit.reset();
		}
		break;
	}
}

void UWorld::PublishEditorSelectionState() {
	UCollisionComponent* Collision = SelectedCollider.Get();
	if (Collision == nullptr) {
		SelectedCollider.Reset();
		EditorSelectionState.GetWriter().Clear();
		return;
	}

	AActor* Owner = Collision->GetOwner();
	USceneComponent* Target = Owner != nullptr ? Owner->GetRootComponent() : nullptr;
	if (Target == nullptr) {
		SelectedCollider.Reset();
		EditorSelectionState.GetWriter().Clear();
		return;
	}

	EditorSelectionState.GetWriter().Emplace(FEditorSelectionState{
		.TransformTargetHandle = Target->GetHandle(),
		.PickedColliderHandle = Collision->GetHandle(),
		.TargetWorld = Target->GetWorldMatrix(),
		.ColliderWorld = Collision->GetWorldMatrix(),
		.BoundsCenter = Collision->GetBoundsCenter(),
		.BoundsExtent = Collision->GetExtent(),
		.BoundsOrientation = Collision->GetBoundsOrientation(),
		.TransformRevision = TransformRevision
	});
}


void UWorld::HandleMouseCameraRotateRequest(const FMouseCameraRotateRequestMessage& Message)
{
    if (Camera == nullptr)
    {
        return;
    }

    constexpr float RotationSensitivity = 0.003f;
    constexpr float MaximumPitch = 1.5f;

    FTransform& CameraTransform = Camera->GetTransform();
    FRotator Rotation = CameraTransform.GetRotation();

    Rotation.y += Message.DeltaX * RotationSensitivity;

    Rotation.x = std::clamp(
        Rotation.x - Message.DeltaY * RotationSensitivity,
        -MaximumPitch,
        MaximumPitch);

    CameraTransform.SetRotation(Rotation);

    PublishEditorCameraState();

    if (EditorCameraWriter.has_value())
    {
        EditorCameraWriter->Write(
            FMessageEditorCameraState
            {
                CameraTransform.GetPosition(),
                CameraTransform.GetRotation(),
                Camera->GetFOV()
            }
        );
    }
}

AActor* UWorld::AddActor(std::unique_ptr<AActor> InActor) 
{
    if (!InActor)
    {
        return nullptr;
    }

    AActor* Actor = InActor.get();

    // 아직 등록되지 않은 Actor만 등록
    if (UObjectSystem::Resolve(Actor->GetHandle()) != Actor)
    {
        UObjectSystem::Register(Actor);
    }

    // 먼저 World가 소유권을 확보
    Actors.push_back(std::move(InActor));

    // 컴포넌트 OnCreate 호출보다 먼저 World가 소유하고 있어야 함
    Actor->SetWorld(this);

    return Actor;
}

void UWorld::RegisterCollision(UCollisionComponent* Component)
{
    if (Component == nullptr)
    {
        return;
    }

    const auto FoundComponent = std::ranges::find_if(
        CollisionComponents,
        [Component](const TObjectRef<UCollisionComponent>& ComponentRef)
        {
            return ComponentRef.Get() == Component;
        });

    if (FoundComponent != CollisionComponents.end())
    {
        return;
    }

    CollisionComponents.emplace_back(Component);
}

void UWorld::UnregisterCollision(UCollisionComponent* Component)
{
    std::erase_if(
        CollisionComponents,
        [Component](const TObjectRef<UCollisionComponent>& ComponentRef)
        {
            UCollisionComponent* RegisteredComponent = ComponentRef.Get();

            return RegisteredComponent == nullptr || RegisteredComponent == Component;
        });
}

void UWorld::HandleKeyboardCameraMoveRequest(
    const FKeyboardCameraMoveRequestMessage& Message)
{
    if (Camera == nullptr || Message.DeltaTime <= 0.0f)
    {
        return;
    }

    const FMatrix CameraWorldMatrix = Camera->GetWorldMatrix();

    const FVector3 ForwardDirection = CameraWorldMatrix.Forward();
    const FVector3 RightDirection = CameraWorldMatrix.Right();


	const FVector3 Forward = ForwardDirection * Message.ForwardAxis;

    FVector3 MoveDirection = ForwardDirection * Message.ForwardAxis + RightDirection * Message.RightAxis;

    if (MoveDirection.LengthSquared() <= 0.0f)
    {
        return;
    }

    MoveDirection.Normalize();

    constexpr float CameraMoveSpeed = 5.0f;

    FTransform& CameraTransform = Camera->GetTransform();

    CameraTransform.SetPosition(CameraTransform.GetPosition() + MoveDirection * CameraMoveSpeed * Message.DeltaTime);

    PublishEditorCameraState();
}


void UWorld::HandleSpawnPrimitive(
    const FMessageSpawnPrimitive& Message, FAssetRegistry& AssetRegistry)
{
    static std::mt19937 RandomEngine{ std::random_device{}() };
    // test
    const FAssetHandle MeshHandle = AssetRegistry.GetAsset(Message.PrimitiveType);
    const FAssetHandle PipelineHandle = AssetRegistry.GetAsset("BasePipeline");
    

    const FAssetHandle Materials[] = {
        AssetRegistry.GetAsset("GreyMaterial"),
        AssetRegistry.GetAsset("RedMaterial"),
        AssetRegistry.GetAsset("GreenMaterial"),
        AssetRegistry.GetAsset("BlueMaterial"),
        AssetRegistry.GetAsset("YellowMaterial"),

        AssetRegistry.GetAsset("AmberMaterial"),
        AssetRegistry.GetAsset("BrownMaterial"),
        AssetRegistry.GetAsset("CyanMaterial"),
        AssetRegistry.GetAsset("LimeMaterial"),
        AssetRegistry.GetAsset("MagentaMaterial"),
        AssetRegistry.GetAsset("NavyMaterial"),
        AssetRegistry.GetAsset("OrangeMaterial"),
        AssetRegistry.GetAsset("PinkMaterial"),
        AssetRegistry.GetAsset("PurpleMaterial"),
        AssetRegistry.GetAsset("TealMaterial"),
        AssetRegistry.GetAsset("WhiteMaterial"),
    };

    int count = _countof(Materials); 

    std::uniform_int_distribution<decltype(count)> r(0, count - 1); 

    const FAssetHandle MaterialHandle = Materials[r(RandomEngine)];

    UMesh* Mesh = AssetRegistry.ResolveAsset<UMesh>(MeshHandle);

    if (Mesh == nullptr)
    {
        return;
    }


    std::uniform_real_distribution<float> RandomX(-5.0f, 5.0f);
    std::uniform_real_distribution<float> RandomY(-5.0f, 5.0f);
    std::uniform_real_distribution<float> RandomZ(-3.0f, 3.0f);

    constexpr FVector3 SpawnCenter{ 0.0f, 0.0f, 5.0f };

    for (uint32 Index = 0; Index < Message.SpawnCount;  ++Index)
    {
        SpawnActor(MeshHandle, PipelineHandle, MaterialHandle,
            FVector3{ SpawnCenter.x + RandomX(RandomEngine),SpawnCenter.y + RandomY(RandomEngine), SpawnCenter.z + RandomZ(RandomEngine) }, Mesh, &AssetRegistry);
    }
}


FAssetRegistry* UWorld::GetAssetRegistry() const {
    return AssetRegistry;
}

void UWorld::ResetWorld(FAssetRegistry* AssetRegistry, ID3D11Device* Device)
{
    for (auto &CurrentActor : Actors)
    {
        DestroyActor(CurrentActor.get());
    }
    FlushPendingDestroyActors();

    AssetRegistry->Reset();
    AssetRegistry->Initialize(Device);
}

void UWorld::HandleNewScene(
    const FMessageNewScene& Message)
{
    // 현재 Scene 초기화
}

void UWorld::HandleChangeGizmoMode(
    const FMessageChangeGizmoMode& Message)
{
    // Gizmo mode 변경
}

void UWorld::UpdateEditorCameraState()
{
    if (!EditorCameraReader.has_value() ||
        Camera == nullptr)
    {
        return;
    }

    auto Result =
        EditorCameraReader->ReadIfChanged();

    if (!Result.Changed ||
        Result.Value == nullptr)
    {
        return;
    }

    FTransform& Transform =
        Camera->GetTransform();

    Transform.SetPosition(
        Result.Value->Position);

    Transform.SetRotation(
        Result.Value->Rotation);

    Camera->SetFOV(
        Result.Value->FOV);
}

void UWorld::SetAssetRegistry(FAssetRegistry* InAssetRegistry) {
	AssetRegistry = InAssetRegistry;
}

void UWorld::ApplyEditorCameraState()
{
    if (!EditorCameraStateReader.has_value() || Camera == nullptr)
    {
        return;
    }

    auto Result = EditorCameraStateReader->ReadIfChanged();

    if (!Result.Changed ||
        Result.Value == nullptr)
    {
        return;
    }

    FTransform& CameraTransform = Camera->GetTransform();

    CameraTransform.SetPosition(Result.Value->Position);
    CameraTransform.SetRotation(Result.Value->Rotation);

    Camera->SetFOV(Result.Value->FOV);
}

void UWorld::PublishEditorCameraState()
{
    if (!EditorCameraStateWriter.has_value() || Camera == nullptr)
    {
        return;
    }

    const FTransform& CameraTransform =
        Camera->GetTransform();

    EditorCameraStateWriter->Write(
        FMessageEditorCameraState
        {
            CameraTransform.GetPosition(),
            CameraTransform.GetRotation(),
            Camera->GetFOV()
        }
    );
}

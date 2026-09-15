#include "PCH.h"
#include "UWorld.h"

#include <algorithm>
#include <random>

#include "AActor.h"
#include "Component/UCameraComponent.h"
#include "Component/UStaticMeshComponent.h"
#include "Subsystem/UCameraSubsystem.h"
#include "Subsystem/UCollisionSubsystem.h"
#include "Subsystem/UPickingSubsystem.h"
#include "Subsystem/URenderSubsystem.h"
#include "Component/UPrimitiveComponent.h"
#include "Component/UTextRenderComponent.h"
#include "FMouseCameraRotateRequestMessage.h"
#include "FMousePickRequestMessage.h"
#include "FWorldEditorContext.h"
#include "FKeyboardCameraMoveRequestMessage.h"
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

#include "../Serialize/FEditorConfigManager.h"

UWorld::UWorld() {
	InitializeSubsystems();
}

UWorld::~UWorld() {
	for (const std::unique_ptr<AActor>& Actor : Actors)
	{
		Actor->SetWorld(nullptr);
		UObjectSystem::Unregister (Actor.get(), Actor->GetHandle());
	}

	Actors.clear();
	DeinitializeSubsystems();
}

bool UWorld::SpawnActor(const FAssetHandle& MeshHandle, const FAssetHandle& PipelineHandle, const FAssetHandle& MaterialHandle, 
						const FVector3& Position)
{
	auto Actor = UWorld::AdoptActor<AActor>();

	UStaticMeshComponent* MeshComponent = Actor->AddComponent<UStaticMeshComponent>();
	Actor->SetRootComponent(MeshComponent);

	MeshComponent->SetMeshHandle(MeshHandle);
	MeshComponent->SetPipelineHandle(PipelineHandle);
	MeshComponent->SetMaterialHandle(MaterialHandle);

	MeshComponent->SetRelativeLocation(
		FVector3{
			Position.x,
			Position.y,
			Position.z
		});

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

		if (EditorContext != nullptr && EditorContext->GetSelectedActor() == Actor) {
			EditorContext->ClearSelection();
		}
		Actor->SetWorld(nullptr);
		UObjectSystem::Unregister(Actor, Actor->GetHandle());

		Actors.erase(It); 
	}

	PendingDestroyActors.clear();
}

const TArray<std::unique_ptr<AActor>>& UWorld::GetActors() const
{
	return Actors;
}

Folder* UWorld::CreateFolder(FString InName, FGuid InParentFolderGuid) {
	if (InParentFolderGuid.IsValid() && FindFolder(InParentFolderGuid) == nullptr) {
		return nullptr;
	}

	std::unique_ptr<Folder> NewFolder = std::make_unique<Folder>(std::move(InName));
	NewFolder->SetParentFolderGuid(InParentFolderGuid);
	Folder* FolderPtr = NewFolder.get();

	if (!AdoptFolder(std::move(NewFolder))) {
		return nullptr;
	}

	return FolderPtr;
}

bool UWorld::DestroyFolder(FGuid FolderGuid) {
	auto It = std::ranges::find_if(Folders, [FolderGuid](const std::unique_ptr<Folder>& CandidateFolder) {
		return CandidateFolder->GetID() == FolderGuid;
	});

	if (It == Folders.end()) {
		return false;
	}

	const FGuid ParentFolderGuid = (*It)->GetParentFolderGuid();
	for (const std::unique_ptr<Folder>& CandidateFolder : Folders) {
		if (CandidateFolder->GetID() != FolderGuid && CandidateFolder->GetParentFolderGuid() == FolderGuid) {
			CandidateFolder->SetParentFolderGuid(ParentFolderGuid);
		}
	}

	for (const std::unique_ptr<AActor>& Actor : Actors) {
		if (Actor->GetFolderGuid() == FolderGuid) {
			Actor->SetFolderGuid(ParentFolderGuid);
		}
	}

	Folders.erase(It);
	return true;
}

bool UWorld::SetFolderParent(FGuid FolderGuid, FGuid InParentFolderGuid) {
	Folder* FolderRecord = FindFolder(FolderGuid);
	if (FolderRecord == nullptr) {
		return false;
	}

	if (!InParentFolderGuid.IsValid()) {
		FolderRecord->ClearParentFolder();
		return true;
	}

	if (FolderGuid == InParentFolderGuid) {
		return false;
	}

	const Folder* ParentFolder = FindFolder(InParentFolderGuid);
	if (ParentFolder == nullptr) {
		return false;
	}

	TSet<FGuid> VisitedFolderGuids;
	for (const Folder* CurrentFolder = ParentFolder; CurrentFolder != nullptr; CurrentFolder = FindFolder(CurrentFolder->GetParentFolderGuid())) {
		if (CurrentFolder->GetID() == FolderGuid || !VisitedFolderGuids.insert(CurrentFolder->GetID()).second) {
			return false;
		}
	}

	FolderRecord->SetParentFolderGuid(InParentFolderGuid);
	return true;
}

Folder* UWorld::FindFolder(FGuid FolderGuid) {
	const auto It = std::ranges::find_if(Folders, [FolderGuid](const std::unique_ptr<Folder>& CandidateFolder) {
		return CandidateFolder->GetID() == FolderGuid;
	});
	return It != Folders.end() ? It->get() : nullptr;
}

const Folder* UWorld::FindFolder(FGuid FolderGuid) const {
	const auto It = std::ranges::find_if(Folders, [FolderGuid](const std::unique_ptr<Folder>& CandidateFolder) {
		return CandidateFolder->GetID() == FolderGuid;
	});
	return It != Folders.end() ? It->get() : nullptr;
}

const TArray<std::unique_ptr<Folder>>& UWorld::GetFolders() const {
	return Folders;
}

bool UWorld::SetActorFolder(AActor* Actor, FGuid FolderGuid) {
	const bool bIsWorldActor = std::ranges::any_of(Actors, [Actor](const std::unique_ptr<AActor>& Candidate) {
		return Candidate.get() == Actor;
	});

	if (!bIsWorldActor || (FolderGuid.IsValid() && FindFolder(FolderGuid) == nullptr)) {
		return false;
	}

	Actor->SetFolderGuid(FolderGuid);
	return true;
}

bool UWorld::AdoptFolder(std::unique_ptr<Folder> InFolder) {
	if (InFolder == nullptr || !InFolder->GetID().IsValid() || FindFolder(InFolder->GetID()) != nullptr) {
		return false;
	}

	Folders.push_back(std::move(InFolder));
	return true;
}

bool UWorld::ValidateFolderHierarchy() const {
	for (const std::unique_ptr<Folder>& FolderRecord : Folders) {
		TSet<FGuid> VisitedFolderGuids;
		for (const Folder* CurrentFolder = FolderRecord.get(); CurrentFolder != nullptr; CurrentFolder = FindFolder(CurrentFolder->GetParentFolderGuid())) {
			if (!VisitedFolderGuids.insert(CurrentFolder->GetID()).second) {
				return false;
			}

			if (CurrentFolder->GetParentFolderGuid().IsValid() && FindFolder(CurrentFolder->GetParentFolderGuid()) == nullptr) {
				return false;
			}
		}
	}

	return true;
}

void UWorld::InitializeSubsystems() {
	RenderSubsystem = std::make_unique<URenderSubsystem>();
	CollisionSubsystem = std::make_unique<UCollisionSubsystem>();
	PickingSubsystem = std::make_unique<UPickingSubsystem>();
	CameraSubsystem = std::make_unique<UCameraSubsystem>();

	RenderSubsystem->Initialize(this);
	CollisionSubsystem->Initialize(this);
	PickingSubsystem->Initialize(this);
	CameraSubsystem->Initialize(this);

	if (!FEditorConfigManager::Load(Settings))
	{
		FEditorConfigManager::Save(Settings);
	}
}

void UWorld::DeinitializeSubsystems() {
	if (CameraSubsystem != nullptr) {
		CameraSubsystem->Deinitialize();
	}
	if (CollisionSubsystem != nullptr) {
		CollisionSubsystem->Deinitialize();
	}
	if (PickingSubsystem != nullptr) {
		PickingSubsystem->Deinitialize();
	}
	if (RenderSubsystem != nullptr) {
		RenderSubsystem->Deinitialize();
	}
}

FRenderProbe& UWorld::BuildRenderProbe() {
	Probe.ActorProbes.clear();
    Probe.GizmoProbes.clear();
    Probe.TextProbes.clear();

	RenderSubsystem->BuildRenderProbes(AssetRegistry, Probe);

    for (const UTextRenderComponent* Component : TextRenderableComponents)
    {
        if (Component == nullptr)
        {
            continue;
        }

        FTextProbe TextProbe{};

        if (Component->MakeTextRender(TextProbe))
        {
            Probe.TextProbes.push_back(std::move(TextProbe));
        }
    }

	
    if (CameraSubsystem->GetMainCamera() != nullptr)
    {
		auto Camera = CameraSubsystem->GetMainCamera();

        Probe.MainCameraProbe.View =
            Camera->GetViewMatrix();

        Probe.MainCameraProbe.Projection =
            Camera->GetProjectionMatrix();

		Probe.MainCameraProbe.ViewProjection =
			Camera->GetViewProjectionMatrix();
	}
	return Probe;
}

void UWorld::SetEditorContext(FWorldEditorContext* InEditorContext) {
	if (InEditorContext == EditorContext) {
		return;
	}
	
	EditorContext = InEditorContext;
}

FWorldEditorContext* UWorld::GetEditorContext() const noexcept {
	return EditorContext;
}

void UWorld::Tick(float DeltaTime) {
	if (UCameraComponent* Camera = GetCameraSubsystem().GetMainCamera(); Camera != nullptr && WindowInfoReader.HasChanged()) {
		Camera->SetAspectRatio(static_cast<float>(WindowInfoReader.Read().ScreenWidth) / static_cast<float>(WindowInfoReader.Read().ScreenHeight));
	}

	if (EditorContext != nullptr && EditorContext->GetCameraState() == nullptr) {
		PublishEditorCameraState();
	}

	for (const std::unique_ptr<AActor>& Actor : Actors) {
		Actor->Tick(DeltaTime);
	}

    FlushPendingDestroyActors();
}

URenderSubsystem& UWorld::GetRenderSubsystem() {
	return *RenderSubsystem;
}

const URenderSubsystem& UWorld::GetRenderSubsystem() const {
	return *RenderSubsystem;
}

UCollisionSubsystem& UWorld::GetCollisionSubsystem() {
	return *CollisionSubsystem;
}

const UCollisionSubsystem& UWorld::GetCollisionSubsystem() const {
	return *CollisionSubsystem;
}

UPickingSubsystem& UWorld::GetPickingSubsystem() {
	return *PickingSubsystem;
}

const UPickingSubsystem& UWorld::GetPickingSubsystem() const {
	return *PickingSubsystem;
}

UCameraSubsystem& UWorld::GetCameraSubsystem() {
	return *CameraSubsystem;
}

const UCameraSubsystem& UWorld::GetCameraSubsystem() const {
	return *CameraSubsystem;
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

	ArraySize = static_cast<size_t>(Folders.size());
	ArchiveSave.BeginArrayScope("Folders", ArraySize);
	for (size_t CurrentIndex = 0, EndIndex = Folders.size(); CurrentIndex < EndIndex; ++CurrentIndex) {
		ArchiveSave.BeginObjectScope(std::to_string(CurrentIndex));
		Folders[CurrentIndex]->Serialize(ArchiveSave);
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

bool UWorld::LoadScene(const std::filesystem::path& ScenePath, ID3D11Device* Device, FAssetRegistry* AssetRegistry) {
	std::ifstream InputFileStream(ScenePath);
	if (!InputFileStream.is_open()) {
		return false;
	}

	std::stringstream Buffer;
	Buffer << InputFileStream.rdbuf();
	std::string LoadedJsonString = Buffer.str();
	InputFileStream.close();

	rapidjson::Document LoadDocument;
	LoadDocument.Parse(LoadedJsonString.c_str());

	if (LoadDocument.HasParseError() ||
		!LoadDocument.IsObject() ||
		!LoadDocument.HasMember("Assets") ||
		!LoadDocument["Assets"].IsArray() ||
		!LoadDocument.HasMember("Actors") ||
		!LoadDocument["Actors"].IsArray() ||
		(LoadDocument.HasMember("Folders") && !LoadDocument["Folders"].IsArray())) {
		return false;
	}

	if (AssetRegistry == nullptr) {
		return false;
	}

	SetAssetRegistry(AssetRegistry);
	ResetWorld(AssetRegistry, Device);

	const auto FailLoad = [this, AssetRegistry, Device]() {
		ResetWorld(AssetRegistry, Device);
		return false;
	};

	// assets
	for (const rapidjson::Value& AssetJson : LoadDocument["Assets"].GetArray()) {
		if (!AssetJson.IsObject() ||
			!AssetJson.HasMember("Guid") || !AssetJson["Guid"].IsString() ||
			!AssetJson.HasMember("TypeName") || !AssetJson["TypeName"].IsString() ||
			!AssetJson.HasMember("AssetName") || !AssetJson["AssetName"].IsString() ||
			!AssetJson.HasMember("AssetMetaDataPath") || !AssetJson["AssetMetaDataPath"].IsString()) {
			return FailLoad();
		}

		FGuid AssetGuid;
		if (!AssetGuid.Parse(AssetJson["Guid"].GetString())) {
			return FailLoad();
		}

		FString TypeName = AssetJson["TypeName"].GetString();
		const FTypeInfo* Type = TypeRegistry::Find(TypeName);
		if (Type == nullptr || Type->Creator == nullptr) {
			return FailLoad();
		}

		FString AssetName = AssetJson["AssetName"].GetString();
		FString MetadataPath = AssetJson["AssetMetaDataPath"].GetString();
		std::unique_ptr<UObject> EmptyAsset = Type->Creator();

		if (!AssetRegistry->AdoptAsset(
			Device,
			AssetGuid,
			AssetName,
			MetadataPath,
			std::move(EmptyAsset))) {
			return FailLoad();
		}
	}

	AssetRegistry->Finalize();

	if (LoadDocument.HasMember("Folders")) {
		for (rapidjson::Value& FolderJson : LoadDocument["Folders"].GetArray()) {
			if (!FolderJson.IsObject() ||
				!FolderJson.HasMember("Guid") || !FolderJson["Guid"].IsString() ||
				!FolderJson.HasMember("Name") || !FolderJson["Name"].IsString() ||
				!FolderJson.HasMember("ParentGuid") || !FolderJson["ParentGuid"].IsString()) {
				return FailLoad();
			}

			FGuid FolderGuid;
			FGuid ParentFolderGuid;
			if (!FolderGuid.Parse(FolderJson["Guid"].GetString()) || !FolderGuid.IsValid() || !ParentFolderGuid.Parse(FolderJson["ParentGuid"].GetString())) {
				return FailLoad();
			}

			std::unique_ptr<Folder> FolderPtr = std::make_unique<Folder>();
			FArchiveJson FolderArchive(FolderJson);
			FolderPtr->Serialize(FolderArchive);
			if (!AdoptFolder(std::move(FolderPtr))) {
				return FailLoad();
			}
		}

		if (!ValidateFolderHierarchy()) {
			return FailLoad();
		}
	}

	// actor and component shells
	for (rapidjson::Value& ActorJson : LoadDocument["Actors"].GetArray()) {
		if (!ActorJson.IsObject() ||
			!ActorJson.HasMember("Guid") || !ActorJson["Guid"].IsString() ||
			!ActorJson.HasMember("TypeName") || !ActorJson["TypeName"].IsString()) {
			return FailLoad();
		}

		FGuid ActorGuid;
		if (!ActorGuid.Parse(ActorJson["Guid"].GetString())) {
			return FailLoad();
		}

		if (ActorJson.HasMember("FolderGuid")) {
			if (!ActorJson["FolderGuid"].IsString()) {
				return FailLoad();
			}

			FGuid FolderGuid;
			if (!FolderGuid.Parse(ActorJson["FolderGuid"].GetString())) {
				return FailLoad();
			}
		}

		FString TypeName = ActorJson["TypeName"].GetString();
		const FTypeInfo* Type = TypeRegistry::Find(TypeName);
		if (Type == nullptr || Type->Creator == nullptr) {
			return FailLoad();
		}

		std::unique_ptr<UObject> CreatedObject = Type->Creator();
		if (CreatedObject == nullptr ||
			!CreatedObject->GetTypeInfo()->IsA(AActor::StaticTypeInfo())) {
			return FailLoad();
		}

		std::unique_ptr<AActor> ActorPtr(static_cast<AActor*>(CreatedObject.release()));
		UObjectSystem::RegisterWithGuid(ActorPtr.get(), ActorGuid);

		FArchiveJson ArchiveLoad(ActorJson);
		if (!ActorPtr->PreLoadComponents(ArchiveLoad)) {
			UObjectSystem::Unregister(ActorPtr.get(), ActorPtr->GetHandle());
			return FailLoad();
		}

		Actors.emplace_back(std::move(ActorPtr));
	}

	// serialized data
	for (size_t ActorIndex = 0; ActorIndex < Actors.size(); ++ActorIndex) {
		rapidjson::Value& ActorJson = LoadDocument["Actors"][static_cast<rapidjson::SizeType>(ActorIndex)];
		FArchiveJson ArchiveLoad(ActorJson);
		ArchiveLoad.SetAssetRegistry(AssetRegistry);
		Actors[ActorIndex]->Load(ArchiveLoad);
		if (Actors[ActorIndex]->GetFolderGuid().IsValid() && FindFolder(Actors[ActorIndex]->GetFolderGuid()) == nullptr) {
			return FailLoad();
		}
	}

	// object references
	for (const std::unique_ptr<AActor>& Actor : Actors) {
		if (!Actor->ResolveLoadedReferences()) {
			return FailLoad();
		}
	}

	// component registration
	for (const std::unique_ptr<AActor>& Actor : Actors) {
		Actor->SetWorld(this);
	}

	return true;
}

void UWorld::HandleMousePickRequest(const FMousePickRequestMessage& Message) {
	UCameraComponent* Camera = GetCameraSubsystem().GetMainCamera();

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

			UPrimitiveComponent* NearestPrimitive = nullptr;
			float NearestDistance = 0.0f;
			if (GetPickingSubsystem().Raycast(FRay{ RayOrigin.ToSimpleMath(), RayDirection.ToSimpleMath() }, NearestPrimitive, NearestDistance)) {
				Console::AddLog(Console::STDOutHandle, ELogLevel::Log, ELogCategory::Etc, "Raycast hit primitive component %f", NearestDistance);
			}

			if (NearestPrimitive != nullptr)
			{
				if (EditorContext != nullptr) {
					EditorContext->SetSelectedActor(NearestPrimitive->GetOwner());
				}
			}
			else if (EditorContext != nullptr) {
				EditorContext->ClearSelection();
			}
		}

			
		
	}

}


void UWorld::HandleMouseCameraRotateRequest(const FMouseCameraRotateRequestMessage& Message)
{
	UCameraComponent* Camera = GetCameraSubsystem().GetMainCamera();
	if (Camera == nullptr)
	{
		return;
	}

	constexpr float RotationSensitivity = 0.003f;
	constexpr float MaximumPitch = 0.99f;

	FTransform& CameraTransform = Camera->GetRelativeTransform();
	const FQuat CurrentRotation = CameraTransform.GetRotationQuaternion();
	const FMatrix CurrentWorld = Camera->GetComponentToWorld();

	FQuat YawDelta = FQuat::CreateFromAxisAngle(FVector3::UnitZ, Message.DeltaX * RotationSensitivity);
	YawDelta.Normalize();

	// Yaw 적용
	FQuat YawedRotation = FQuat::Concatenate(YawDelta, CurrentRotation);
	YawedRotation.Normalize();

	// Yaw 적용 후의 축을 행렬에서 가져옴
	FTransform YawedTransform;
	YawedTransform.SetRotation(YawedRotation);

	FMatrix YawMatrix = YawedTransform.ToMatrixWithScale();

	FVector Right = YawMatrix.Right();
	Right.Normalize();

	FVector Forward = YawMatrix.Forward();
	Forward.Normalize();

	FVector Up = FVector(0, 0, 1);
	FQuat PitchDelta;

	if (Forward.Dot(Up) > MaximumPitch && Message.DeltaY > 0.0f) {
		PitchDelta = FQuat::CreateFromAxisAngle(Right, 0 * RotationSensitivity);
	}
	else if (Forward.Dot(Up) < -MaximumPitch && Message.DeltaY < 0.0f) {
		PitchDelta = FQuat::CreateFromAxisAngle(Right, 0 * RotationSensitivity);
	}
	else {
		PitchDelta = FQuat::CreateFromAxisAngle(Right, -Message.DeltaY * RotationSensitivity);
	}

	PitchDelta.Normalize();

	FQuat FinalRotation;
	auto worldDelta = FQuat::Concatenate(PitchDelta, YawDelta);

	worldDelta.Normalize();
	CameraTransform.SetRotation(FQuat::Concatenate(worldDelta, CurrentRotation));

	// CameraTransform.SetRotation(FQuat::Concatenate(CurrentRotation, PitchDelta));

	PublishEditorCameraState();
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

void UWorld::HandleKeyboardCameraMoveRequest(
	const FKeyboardCameraMoveRequestMessage& Message)
{
	UCameraComponent* Camera = GetCameraSubsystem().GetMainCamera();
	if (Camera == nullptr || Message.DeltaTime <= 0.0f)
	{
		return;
	}

	const FMatrix CameraWorldMatrix = Camera->GetComponentToWorld();

	const FVector3 ForwardDirection = CameraWorldMatrix.Forward();
	const FVector3 RightDirection = CameraWorldMatrix.Right();


	const FVector3 Forward = ForwardDirection * Message.ForwardAxis;

	FVector3 MoveDirection = ForwardDirection * Message.ForwardAxis + RightDirection * Message.RightAxis;

	if (MoveDirection.LengthSquared() <= 0.0f)
	{
		return;
	}

	MoveDirection.Normalize();

	float CameraMoveSpeed = Settings.MoveSensitivity;


	FTransform& CameraTransform = Camera->GetRelativeTransform();

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

	if (AssetRegistry.ResolveAsset<UMesh>(MeshHandle) == nullptr)
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
			FVector3{ SpawnCenter.x + RandomX(RandomEngine),SpawnCenter.y + RandomY(RandomEngine), SpawnCenter.z + RandomZ(RandomEngine) });
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
	Folders.clear();

	AssetRegistry->Reset();
	AssetRegistry->Initialize(Device);
}

FName UWorld::MakeUniqueObjectName(std::string_view SourceName)
{
	std::string_view BaseName;
	int32 Number = 0;

	SplitNameAndNumber(SourceName, BaseName, Number);

	int32 Index = (Number > 0) ? (Number + 1) : 1;

	if ((Number == 0) && (FindActorByName(BaseName) == nullptr))
	{
		return FName(BaseName);
	}

	while (true)
	{
		FName CandidateName(BaseName, Index);

		if (FindActorByName(CandidateName) == nullptr)
		{
			return CandidateName;
		}

		Index++;
	}

	return FName();
}

AActor* UWorld::FindActorByName(FName InName) const
{
	for (const auto& Actor : Actors)
	{
		if (Actor && Actor->GetName() == InName)
		{
			return Actor.get();
		}
	}

	return nullptr;
}

void UWorld::UpdateEditorCameraState() {

}

void UWorld::SetAssetRegistry(FAssetRegistry* InAssetRegistry) {
	AssetRegistry = InAssetRegistry;
}

void UWorld::PublishEditorCameraState()
{
	UCameraComponent* Camera = GetCameraSubsystem().GetMainCamera();
	if (EditorContext == nullptr || Camera == nullptr)
	{
		return;
	}

	const FTransform& CameraTransform =
		Camera->GetRelativeTransform();

	const FCameraSnapshot CameraState{
		CameraTransform.GetPosition(),
		CameraTransform.GetRotation(),
		Camera->GetFOV()
	};

	EditorContext->PublishCameraState(CameraState);
}

void UWorld::RegisterTextRenderable(UTextRenderComponent* Component)
{
    if (Component == nullptr)
    {
        return;
    }
    if (std::ranges::find(TextRenderableComponents,Component) != TextRenderableComponents.end())
    {
        return;
    }
    TextRenderableComponents.push_back(Component);
}

void UWorld::UnregisterTextRenderable(UTextRenderComponent* Component)
{
    std::erase(TextRenderableComponents,Component);
}

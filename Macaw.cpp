// Macaw.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//
#include "PCH.h"
 
#include "framework.h"
#include "Macaw.h"

#include "Render/Renderer.h"

#include <d3d11.h>
#include <chrono>
#pragma comment(lib, "d3d11.lib")

#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"
  
#include "Core/Console/Console.h"
#include "Render/Panel/Console/ConsoleWindow.h"
#include "Core/Asset/FAssetRegistry.h"

#include "Render/Panel/Stats/StatWindow.h"

#include "Core/Base/FTransform.h"
#include "Scene/UWorld.h"
#include "Scene/AActor.h"
#include "Scene/Component/UCameraComponent.h"
#include "Scene/Component/UStaticMeshComponent.h"
#include "Scene/Component/UCollisionComponent.h"

#include "Core/Base/TypeRegistry.h"

#include "Core/Channel/FMessageChannel.h"
#include "Core/Channel/FStateChannel.h"
#include "FMouseInput.h"
#include "Render/Panel/FEditorInfo.h"
#include "Render/Panel/FEditorUIManager.h"

#include "FMousePickRequestMessage.h"
#include "FMouseCameraRotateRequestMessage.h"
#include "FWorldSelectionChangedMessage.h"
#include "FTransformEditRequestMessage.h"
#include "FKeyboardInput.h"
#include "FKeyboardCameraMoveRequestMessage.h"
#include "Render/Panel/FEditorSelection.h"

#include "Core/Base/UndoSystem/FUndoSystem.h"
#include "Core/Base/UndoSystem/FUndoMessages.h"
#include "Serialize/FArchiveMemory.h"

//test
#include "Render/Pipeline/UPipeline.h"
#include "Core/Asset/UMesh.h"
#include "Core/Asset/UColorMaterial.h"

#include "Render/EditorView/EditorViewport.h"

#include "FName.h"

#define MAX_LOADSTRING 100


#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d11.lib")

constexpr bool WINDOWED = true;
constexpr uint32 DEFAULT_WINDOW_WIDTH = 1920;
constexpr uint32 DEFAULT_WINDOW_HEIGHT = 1080;

// 전역 변수:
HINSTANCE hInst;                                // 현재 인스턴스입니다.
WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.

HWND hWnd = nullptr;

FMouseInput GMouseInput;
FKeyboardInput GKeyboardInput;

// 이 코드 모듈에 포함된 함수의 선언을 전달합니다:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
HWND gHWND;
FRenderer Renderer;

//#define LOAD 

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 여기에 코드를 입력합니다.
	TypeRegistry::Register(UObject::StaticTypeInfo());
	TypeRegistry::Register(UAsset::StaticTypeInfo());
    TypeRegistry::Register(UMesh::StaticTypeInfo());
    TypeRegistry::Register(UPipeline::StaticTypeInfo());
	TypeRegistry::Register(UColorMaterial::StaticTypeInfo());
    TypeRegistry::Register(AActor::StaticTypeInfo());

	TypeRegistry::Register(UWorld::StaticTypeInfo());
	TypeRegistry::Register(AActor::StaticTypeInfo());
	TypeRegistry::Register(UCameraComponent::StaticTypeInfo());
	TypeRegistry::Register(UStaticMeshComponent::StaticTypeInfo());
    TypeRegistry::Register(UCollisionComponent::StaticTypeInfo());
	TypeRegistry::Register(UActorComponent::StaticTypeInfo());
	TypeRegistry::Register(USceneComponent::StaticTypeInfo());
	TypeRegistry::Register(UCollisionComponent::StaticTypeInfo());
	


    auto res = TypeRegistry::Find("UMesh")->Creator();
	if (res->GetTypeInfo()->IsA(UMesh::StaticTypeInfo())) {
		Console::AddLog(Console::STDOutHandle, ELogLevel::Log, ELogCategory::Etc, "UMesh instance created successfully.");
	}
	else {
		Console::AddLog(Console::STDOutHandle, ELogLevel::Error, ELogCategory::Etc, "Failed to create UMesh instance.");
	}


    // 전역 문자열을 초기화합니다.
    //LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    //LoadStringW(hInstance, IDC_MACAW, szWindowClass, MAX_LOADSTRING);
    wcscpy_s(szTitle, MAX_LOADSTRING, L"Macaw Engine");
    wcscpy_s(szWindowClass, MAX_LOADSTRING, L"MacawEngineClass");
    MyRegisterClass(hInstance);

    // 애플리케이션 초기화를 수행합니다:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MACAW));

    MSG msg;

	Console::AddLog(Console::STDOutHandle, ELogLevel::Log, ELogCategory::Etc, "Macaw Engine Initialized.");

    // test
    UWorld World{};

    Renderer.Create(gHWND, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);


    FAssetRegistry AssetRegistry;
    AssetRegistry.Initialize(Renderer.GetDevice(), 128);
    Renderer.BindAssetRegistry(&AssetRegistry);


    FMessageChannel WorldCommandChannel{ 64 };
    FMessageChannel EditorEventChannel{ 64 };
    FEditorSelection EditorSelection;

    FStateChannel<FMessageEditorCameraState> EditorCameraStateChannel;

    FMessageChannel SpawnCommandChannel{ 64 };
    FMessageChannel SceneCommandChannel{ 64 };
    FMessageChannel GizmoCommandChannel{ 64 };

    
    
    /*
    FMessageChannel UndoCommandChannel{ 64 };
    FUndoSystem::InitializeSenderToWorldChannel(UndoCommandChannel.GetSender());
    UndoCommandChannel.TryBind<FMessageUndoObjectStateChanged>(
        [&World, &AssetRegistry](const FMessageUndoObjectStateChanged& Message)
        {
            UObject* Target = UObjectSystem::Resolve(UObjectSystem::FindHandleByGuid(Message.TargetGuid));
            if (Target)
            {
                FArchiveMemory ArchiveLoad(Message.SavedData);
                Target->Load(ArchiveLoad);

            }
        });
    UndoCommandChannel.TryBind<FMessageUndoObjectSpawned>(
        [&World, &AssetRegistry](const FMessageUndoObjectSpawned& Message)
        {
            FGuid ActorGuid;
            ActorGuid.Parse(Message.TargetGuid.ToString());
            FString TypeName = Message.TargetTypeName;

            std::unique_ptr<UObject> CreatedObject = TypeRegistry::Find(TypeName)->Creator();
            std::unique_ptr<AActor> ActorPointer(static_cast<AActor*>(CreatedObject.release()));
            UObjectSystem::RegisterWithGuid(ActorPointer.get(), ActorGuid);

            const auto SavedData = Message.SavedData;
            FArchiveMemory ArchiveLoad(SavedData);
            ArchiveLoad.SetAssetRegistry(&AssetRegistry);
            ActorPointer->Load(ArchiveLoad);
            ActorPointer->SetWorld(&World);

            World.AddActor(std::move(ActorPointer));
        });
    UndoCommandChannel.TryBind<FMessageUndoObjectDestroyed>(
        [&World](const FMessageUndoObjectDestroyed& Message)
        {
            World.DestroyActor(static_cast<AActor*>(UObjectSystem::Resolve(UObjectSystem::FindHandleByGuid(Message.TargetGuid))));
            World.FlushPendingDestroyActors();
        });
    WorldCommandChannel.TryBind<FMessageUndoApply>(
        [](const FMessageUndoApply& Message)
        {
            if (Message.bIsUndo)
                FUndoSystem::Undo();
            else
                FUndoSystem::Redo();
        });

        */


    SpawnCommandChannel.TryBind<FMessageSpawnPrimitive>(
        [&World, &AssetRegistry](const FMessageSpawnPrimitive& Message)
        {
            /*
            FString TransactionName;
            // "Spawn "(6자) + 32비트 정수 최대 길이(11자) + PrimitiveType 길이
            TransactionName.reserve(17 + Message.PrimitiveType.size());
            std::format_to(std::back_inserter(TransactionName), "Spawn {}{}(es)", Message.SpawnCount, Message.PrimitiveType);

            FUndoSystem::BeginTransaction(TransactionName);
            FUndoSystem::EndTransaction();
            */
            World.HandleSpawnPrimitive(Message, AssetRegistry);
        }
    );
    SpawnCommandChannel.TryBind<FMessageDeletePrimitive>(
        [&World](const FMessageDeletePrimitive& Message)
        {
            if (World.GetEditorSelectionStateReader().HasValue())
            {
                UCollisionComponent* SelectedActor = static_cast<UCollisionComponent*>(UObjectSystem::Resolve(World.GetEditorSelectionStateReader().Read().PickedColliderHandle));
                World.DestroyActor(SelectedActor->GetOwner());
                World.FlushPendingDestroyActors();
            }

        }
    );

    

    EditorViewport EditorView{};
    EditorView.Initialize(Renderer.GetDevice(), AssetRegistry, Renderer.GetWindowInfoReader(), World.GetEditorSelectionStateReader(), WorldCommandChannel.GetSender());

    FEditorUIManager EditorUIManager;

    EditorUIManager.Initialize(
        World,

        EditorCameraStateChannel.GetWriter(),
        EditorCameraStateChannel.GetReader(),

        gHWND,

        World.GetEditorSelectionStateReader(),
        WorldCommandChannel.GetSender(),

        SpawnCommandChannel.GetSender(),
        SceneCommandChannel.GetSender(),
        EditorView.GetGizmoMode()
    );

    World.InitializeEditorCameraState(
        EditorCameraStateChannel.GetWriter(),
        EditorCameraStateChannel.GetReader()
    );

    GMouseInput.InitializeWorldCommandSender(WorldCommandChannel.GetSender());
    GKeyboardInput.InitializeWorldCommandSender(WorldCommandChannel.GetSender());
    World.InitializeEditorEventSender(EditorEventChannel.GetSender());
	World.SetWindowInfoReader(Renderer.GetWindowInfoReader());
	World.SetAssetRegistry(&AssetRegistry);

    WorldCommandChannel.TryBind<FMousePickRequestMessage>(
        [&World](const FMousePickRequestMessage& Message)
        {
            World.HandleMousePickRequest(Message);
        });

    WorldCommandChannel.TryBind<FMouseCameraRotateRequestMessage>(
        [&World](const FMouseCameraRotateRequestMessage& Message)
        {
            World.HandleMouseCameraRotateRequest(Message);
        });

    EditorEventChannel.TryBind<FWorldSelectionChangedMessage>(
        [&EditorSelection](const FWorldSelectionChangedMessage& Message)
        {
            EditorSelection.HandleSelectionChanged(Message);
        });

    WorldCommandChannel.TryBind<
        FKeyboardCameraMoveRequestMessage>(
            [&World](
                const FKeyboardCameraMoveRequestMessage& Message)
            {
                World.HandleKeyboardCameraMoveRequest(Message);
            });

    SceneCommandChannel.TryBind<FMessageNewScene>(
        [&World](const FMessageNewScene& Message)
        {
            World.HandleNewScene(Message);
        }
    );

    SceneCommandChannel.TryBind<FMessageSaveScene>(
        [&World, &AssetRegistry](const FMessageSaveScene& Message)
        {
            World.SaveScene(
                Message.SceneName,
                &AssetRegistry
            );
        }
    );
	WorldCommandChannel.TryBind<FMousePickReleaseRequestMessage>(
		[&World](const FMousePickReleaseRequestMessage& Message)
		{
			World.HandleMousePickReleaseRequest(Message);
		});

	WorldCommandChannel.TryBind<FTransformEditRequestMessage>(
		[&World](const FTransformEditRequestMessage& Message) {
			World.HandleTransformEditRequest(Message);
		});



  
    SceneCommandChannel.TryBind<FMessageLoadScene>(
        [&World, &AssetRegistry](const FMessageLoadScene& Message)
        {
            World.LoadScene(
                std::filesystem::path(Message.FilePath.c_str()),
                Renderer.GetDevice(),
                &AssetRegistry
            );
        }
    );

    GizmoCommandChannel.TryBind<FMessageChangeGizmoMode>(
        [&World](const FMessageChangeGizmoMode& Message)
        {
            World.HandleChangeGizmoMode(Message);
        }
    );
	
#ifdef LOAD
	World.LoadScene("./scenes/NewScene1234.json", Renderer.GetDevice(), &AssetRegistry);
#else 
	AssetRegistry.EmplaceAsset<UPipeline>(Renderer.GetDevice(), "BasePipeline", "./Content/Metadata/BasePipeline.meta");
	AssetRegistry.EmplaceAsset<UPipeline>(Renderer.GetDevice(), "AlternatePipeline", "./Content/Metadata/AlternatePipeline.meta");
    // Triangle
	AssetRegistry.EmplaceAsset<UMesh>(Renderer.GetDevice(), "SphereMesh", "./Content/Metadata/SphereMesh.meta");
	AssetRegistry.EmplaceAsset<UMesh>(Renderer.GetDevice(), "CubeMesh", "./Content/Metadata/CubeMesh.meta");
	AssetRegistry.EmplaceAsset<UMesh>(Renderer.GetDevice(), "CylinderMesh", "./Content/Metadata/CylinderMesh.meta");
	AssetRegistry.EmplaceAsset<UMesh>(Renderer.GetDevice(), "PlaneMesh", "./Content/Metadata/PlaneMesh.meta");
	AssetRegistry.EmplaceAsset<UMesh>(Renderer.GetDevice(), "ConeMesh", "./Content/Metadata/ConeMesh.meta");
   	AssetRegistry.EmplaceAsset<UMesh>(Renderer.GetDevice(), "TorusMesh", "./Content/Metadata/TorusMesh.meta");
	AssetRegistry.EmplaceAsset<UMesh>(Renderer.GetDevice(), "CapsuleMesh", "./Content/Metadata/CapsuleMesh.meta");
	AssetRegistry.EmplaceAsset<UMesh>(Renderer.GetDevice(), "PyrimidMesh", "./Content/Metadata/PyramidMesh.meta");

    
    AssetRegistry.EmplaceAsset<UColorMaterial>(Renderer.GetDevice(), "GreyMaterial", "./Content/Metadata/GreyMaterial.meta");
    AssetRegistry.EmplaceAsset<UColorMaterial>(Renderer.GetDevice(), "RedMaterial", "./Content/Metadata/RedMaterial.meta");
    AssetRegistry.EmplaceAsset<UColorMaterial>(Renderer.GetDevice(), "GreenMaterial", "./Content/Metadata/GreenMaterial.meta");
    AssetRegistry.EmplaceAsset<UColorMaterial>(Renderer.GetDevice(), "BlueMaterial", "./Content/Metadata/BlueMaterial.meta");
    AssetRegistry.EmplaceAsset<UColorMaterial>(Renderer.GetDevice(), "YellowMaterial", "./Content/Metadata/YellowMaterial.meta");
    AssetRegistry.EmplaceAsset<UColorMaterial>(Renderer.GetDevice(), "AmberMaterial", "./Content/Metadata/AmberMaterial.meta");
    AssetRegistry.EmplaceAsset<UColorMaterial>(Renderer.GetDevice(), "BrownMaterial", "./Content/Metadata/BrownMaterial.meta");
    AssetRegistry.EmplaceAsset<UColorMaterial>(Renderer.GetDevice(), "CyanMaterial", "./Content/Metadata/CyanMaterial.meta");
    AssetRegistry.EmplaceAsset<UColorMaterial>(Renderer.GetDevice(), "LimeMaterial", "./Content/Metadata/LimeMaterial.meta");
    AssetRegistry.EmplaceAsset<UColorMaterial>(Renderer.GetDevice(), "MagentaMaterial", "./Content/Metadata/MagentaMaterial.meta");
    AssetRegistry.EmplaceAsset<UColorMaterial>(Renderer.GetDevice(), "NavyMaterial", "./Content/Metadata/NavyMaterial.meta");
    AssetRegistry.EmplaceAsset<UColorMaterial>(Renderer.GetDevice(), "OrangeMaterial", "./Content/Metadata/OrangeMaterial.meta");
    AssetRegistry.EmplaceAsset<UColorMaterial>(Renderer.GetDevice(), "PinkMaterial", "./Content/Metadata/PinkMaterial.meta");
    AssetRegistry.EmplaceAsset<UColorMaterial>(Renderer.GetDevice(), "PurpleMaterial", "./Content/Metadata/PurpleMaterial.meta");
    AssetRegistry.EmplaceAsset<UColorMaterial>(Renderer.GetDevice(), "TealMaterial", "./Content/Metadata/TealMaterial.meta");
    AssetRegistry.EmplaceAsset<UColorMaterial>(Renderer.GetDevice(), "WhiteMaterial", "./Content/Metadata/WhiteMaterial.meta");


    AActor* CameraActor = World.AdoptActor<AActor>();
    UCameraComponent* Camera = CameraActor->AddComponent<UCameraComponent>();

    UCollisionComponent* TestCollision = nullptr;

    CameraActor->SetRootComponent(Camera);

    /*
    {
        constexpr uint32 InstanceCount = 120;
        constexpr float MinInstanceX = -30.0f;
        constexpr float MaxInstanceX = 30.0f;
        constexpr float MinInstanceY = -12.0f;
        constexpr float MaxInstanceY = 12.0f;
        constexpr float NearInstanceDepth = 6.0f;
        constexpr float FarInstanceDepth = 70.0f;

        const FAssetHandle MeshHandle = AssetRegistry.GetAsset("SphereMesh");
        const FAssetHandle BasePipelineHandle = AssetRegistry.GetAsset("BasePipeline");
        const FAssetHandle AlternatePipelineHandle = AssetRegistry.GetAsset("AlternatePipeline");
        const FAssetHandle MaterialHandle = AssetRegistry.GetAsset("GreyMaterial");


        const auto Random01 = [](uint32 Seed) {
            Seed ^= Seed >> 16;
            Seed *= 0x7feb352dU;
            Seed ^= Seed >> 15;
            Seed *= 0x846ca68bU;
            Seed ^= Seed >> 16;

            return static_cast<float>(Seed & 0x00ffffffU) / static_cast<float>(0x00ffffffU);
            };

        UMesh* Mesh =
            AssetRegistry.ResolveAsset<UMesh>( MeshHandle);

        for (uint32 InstanceIndex = 0; InstanceIndex < InstanceCount; ++InstanceIndex) {
            const float PositionX = MinInstanceX + Random01(InstanceIndex * 7U + 1U) * (MaxInstanceX - MinInstanceX);
            const float PositionY = MinInstanceY + Random01(InstanceIndex * 7U + 2U) * (MaxInstanceY - MinInstanceY);
            const float PositionZ = NearInstanceDepth + Random01(InstanceIndex * 7U + 3U) * (FarInstanceDepth - NearInstanceDepth);
            const float ScaleFactor = 0.65f + Random01(InstanceIndex * 7U + 4U) * 0.7f;
            const float Pitch = (Random01(InstanceIndex * 7U + 5U) - 0.5f) * 0.5f;
            const float Yaw = (Random01(InstanceIndex * 7U + 6U) - 0.5f) * 0.5f;
            const float Roll = (Random01(InstanceIndex * 7U + 7U) - 0.5f) * 1.3f;

            AActor* InstanceActor = World.AdoptActor<AActor>();
            UStaticMeshComponent* InstanceComponent = InstanceActor->AddComponent<UStaticMeshComponent>();
            UCollisionComponent* CollisionComponent = InstanceActor->AddComponent<UCollisionComponent>();

            InstanceActor->SetRootComponent(InstanceComponent);
            CollisionComponent->AttachTo(InstanceComponent);

            InstanceComponent->GetTransform().SetPosition({
                PositionX,
                PositionY,
                PositionZ
                });
            InstanceComponent->GetTransform().SetRotation({ Pitch, Yaw, Roll });
            InstanceComponent->GetTransform().SetScale({ ScaleFactor, ScaleFactor, ScaleFactor });

            if (Mesh != nullptr)
            {
                CollisionComponent->SetBounds(Mesh->GetLocalBoundingBox());
            }

            InstanceComponent->SetMeshHandle(MeshHandle);





            const bool bUseAlternatePipeline = InstanceIndex % 2 == 1;
            InstanceComponent->SetPipelineHandle(bUseAlternatePipeline ? AlternatePipelineHandle : BasePipelineHandle);
            InstanceComponent->SetMaterialHandle(MaterialHandle);

            if (InstanceIndex == 0)
            {
                TestCollision = CollisionComponent;
            }
        }

    }
    World.SaveScene("test", &AssetRegistry);
    */

#endif 

    FRenderProbe Probe = World.BuildRenderProbe();

    std::string DebugText =
        "Actor Count = " + std::to_string(Probe.ActorProbes.size()) + "\n";



    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplWin32_Init((void*)hWnd);
    ImGui_ImplDX11_Init(Renderer.GetDevice(), Renderer.GetDeviceContext());

    auto LastTickTime = std::chrono::steady_clock::now();

    while (true) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                break;
            }
            if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else {
            const auto CurrentTickTime = std::chrono::steady_clock::now();
            const float DeltaTime = std::chrono::duration<float>(CurrentTickTime - LastTickTime).count();
            LastTickTime = CurrentTickTime;

            Renderer.BeginFrame();


            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            // 입력 상태는 WndProc의 ProcessWindowMessage에서 갱신한다.
			EditorView.ProcessInput(GKeyboardInput, GMouseInput, ImGui::GetIO().WantCaptureMouse);

            EditorUIManager.Tick();

            GMouseInput.DispatchPendingWorldCommands(
                DEFAULT_WINDOW_WIDTH,
                DEFAULT_WINDOW_HEIGHT,
                ImGui::GetIO().WantCaptureMouse);

            GKeyboardInput.DispatchPendingWorldCommands(
                DeltaTime,
                ImGui::GetIO().WantCaptureKeyboard);

            WorldCommandChannel.Dispatch();
            World.Tick(DeltaTime);

            EditorEventChannel.Dispatch();

            SpawnCommandChannel.Dispatch();
            SceneCommandChannel.Dispatch();
            GizmoCommandChannel.Dispatch();

            //UndoCommandChannel.Dispatch();

			FRenderProbe& Probe{ World.BuildRenderProbe() };
            
			EditorView.RenderInProbe(Probe);
            Renderer.RenderScene(Probe);
            EditorView.RenderSceneGuides(Renderer.GetDeviceContext(),Probe);
            Renderer.RenderGizmos(Probe);
            EditorView.RenderOrientationAxis(Renderer.GetDeviceContext(),Probe.MainCameraProbe);


            // FName Test ImGui
            ImGui::Begin("FName Test");

            static char InputBufferA[128] = "Player";
            static char InputBufferB[128] = "player";

            ImGui::Text("Type string to test FName");
            ImGui::Spacing();

            ImGui::InputText("String A", InputBufferA, sizeof(InputBufferA));
            ImGui::InputText("String B", InputBufferB, sizeof(InputBufferB));

            ImGui::Separator();

            FName NameA = FName(InputBufferA);
            FName NameB = FName(InputBufferB);       

            ImGui::Text("NameA String : %s", NameA.ToString().c_str());
            ImGui::Spacing();
            ImGui::Text("NameB String : %s", NameB.ToString().c_str());

            ImGui::Separator();
            ImGui::Text("Comparison Result :");
            if (NameA == NameB)
            {
                ImGui::Text("Eqaul");
            }
            else
            {
                ImGui::Text("Not Eqaul");
            }
            ImGui::End();
            // FName Test ImGui


            ImGui::Render();
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            
            Renderer.EndFrame();

            GMouseInput.EndFrame();
        }
    }
   

    // ImGui 소멸
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

	World.SaveScene("test", &AssetRegistry);

    return (int) msg.wParam;
}


//
//  함수: MyRegisterClass()
//
//  용도: 창 클래스를 등록합니다.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex{};

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MACAW));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   함수: InitInstance(HINSTANCE, int)
//
//   용도: 인스턴스 핸들을 저장하고 주 창을 만듭니다.
//
//   주석:
//
//        이 함수를 통해 인스턴스 핸들을 전역 변수에 저장하고
//        주 프로그램 창을 만든 다음 표시합니다.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.


    if (WINDOWED) {
        DWORD style = WS_OVERLAPPEDWINDOW;
        DWORD exStyle = WS_EX_OVERLAPPEDWINDOW;

        int posX = (GetSystemMetrics(SM_CXSCREEN) / 2) - (static_cast<int>(DEFAULT_WINDOW_WIDTH) / 2);
        int posY = (GetSystemMetrics(SM_CYSCREEN) / 2) - (static_cast<int>(DEFAULT_WINDOW_HEIGHT) / 2);

        RECT adjustedRect{ 0, 0, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT };
        ::AdjustWindowRectEx(std::addressof(adjustedRect), style, FALSE, exStyle);

        hWnd = CreateWindowEx(
            exStyle,                                // 확장 스타일
            szWindowClass,                          // 윈도우 클래스 이름
            szTitle,                                // 윈도우 타이틀 
            style,                                  // 윈도우 스타일
            posX, posY,                             // 위치
            adjustedRect.right - adjustedRect.left,
            adjustedRect.bottom - adjustedRect.top, // 크기
            nullptr,                                // 부모 윈도우
            nullptr,                                // 메뉴
            hInstance,                              // 인스턴스 핸들
            nullptr                                 // 추가 매개변수
        );
    }
    else {
        DWORD style = WS_POPUP;
        DWORD exStyle = NULL;

        int posX = (GetSystemMetrics(SM_CXSCREEN) / 2) - (static_cast<int>(DEFAULT_WINDOW_WIDTH) / 2);
        int posY = (GetSystemMetrics(SM_CYSCREEN) / 2) - (static_cast<int>(DEFAULT_WINDOW_HEIGHT) / 2);

        hWnd = CreateWindowEx(
            exStyle,                                // 확장 스타일
            szWindowClass,                          // 윈도우 클래스 이름
            szTitle,                                // 윈도우 타이틀 
            style,                                  // 윈도우 스타일
            posX, posY,                             // 위치 
            DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, // 크기
            nullptr,                                // 부모 윈도우
            nullptr,                                // 메뉴
            hInstance,                              // 인스턴스 핸들
            nullptr                                 // 추가 매개변수
        );
    }

    if (!hWnd) {
        // GetLastError()는 실패한 원인의 에러 코드를 반환합니다.
        DWORD errorCode = GetLastError();

        // errorCode가 1407 이면 -> "클래스를 찾을 수 없습니다" (1번 원인)
        // errorCode가 1400 이면 -> "잘못된 윈도우 핸들입니다"
        // errorCode를 구글이나 MS 공식 문서에 검색하면 원인이 바로 나옵니다.
        OutputDebugStringA(("Window Creation Failed! Error Code: " + std::to_string(errorCode) + "\n").c_str());
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

	gHWND = hWnd;

    return TRUE;
}

//
//  함수: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  용도: 주 창의 메시지를 처리합니다.
//
//  WM_COMMAND  - 애플리케이션 메뉴를 처리합니다.
//  WM_PAINT    - 주 창을 그립니다.
//  WM_DESTROY  - 종료 메시지를 게시하고 반환합니다.
//

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam);

    GMouseInput.ProcessWindowMessage(
        message,
        wParam,
        lParam);

    GKeyboardInput.ProcessWindowMessage(
        message,
        wParam,
        lParam);

    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
	case WM_SIZE:
		if (wParam != SIZE_MINIMIZED) {
			uint32 width = LOWORD(lParam);
			uint32 height = HIWORD(lParam);
			Renderer.ReSize(width, height);
		}
		break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}



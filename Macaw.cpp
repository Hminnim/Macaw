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
#include "Scene/Component/UBoxColliderComponent.h"
#include "Scene/FWorldEditorContext.h"

#include "Core/Base/TypeRegistry.h"

#include "Core/Channel/FMessageChannel.h"
#include "Core/Channel/FStateChannel.h"
#include "FMouseInput.h"
#include "Render/Panel/FEditorInfo.h"
#include "Render/Panel/FEditorUIManager.h"

#include "FMousePickRequestMessage.h"
#include "FMouseCameraRotateRequestMessage.h"
#include "FKeyboardInput.h"
#include "FKeyboardCameraMoveRequestMessage.h"

#include "Core/Base/UndoSystem/FUndoSystem.h"
#include "Core/Base/UndoSystem/FUndoMessages.h"
#include "Serialize/FArchiveMemory.h"

//test
#include "Render/Pipeline/UPipeline.h"
#include "Core/Asset/UMesh.h"
#include "Core/Asset/UColorMaterial.h"
#include "Core/Asset/UTexture.h"
#include "Core/Asset/UTexturedMaterial.h"

#include "Render/EditorView/EditorViewport.h"

#include "Core/Asset/UFont.h"
#include "UKFont.h"
#include "Scene/Component/UTextRenderComponent.h"
#include "Scene/Component/UKTextRenderComponent.h"

#include "Serialize/FEditorConfigManager.h"

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

namespace {
    constexpr bool bLoadTestScene = false;
    constexpr bool bEnableSceneSave = false;

    void ConfigureTestStaticMesh(UStaticMeshComponent* MeshComponent, const FAssetHandle& MeshHandle, const FAssetHandle& PipelineHandle, const FAssetHandle& MaterialHandle, const FVector3& Location) {
        MeshComponent->SetMeshHandle(MeshHandle);
        MeshComponent->SetPipelineHandle(PipelineHandle);
        MeshComponent->SetMaterialHandle(MaterialHandle);
        MeshComponent->SetRelativeLocation(Location);
    }

    UStaticMeshComponent* AddTestStaticMesh(AActor* Actor, const FAssetHandle& MeshHandle, const FAssetHandle& PipelineHandle, const FAssetHandle& MaterialHandle, const FVector3& Location) {
        UStaticMeshComponent* MeshComponent = Actor->AddComponent<UStaticMeshComponent>();
        if (MeshComponent != nullptr) {
            ConfigureTestStaticMesh(MeshComponent, MeshHandle, PipelineHandle, MaterialHandle, Location);
        }
        return MeshComponent;
    }

    void AddTestCollider(AActor* Actor, USceneComponent* Parent, UMeshComponent* MeshComponent) {
        UBoxColliderComponent* Collider = Actor->AddComponent<UBoxColliderComponent>();
        if (Collider == nullptr || !Collider->AttachToComponent(Parent)) {
            return;
        }
        Collider->SetMeshComponent(MeshComponent);
    }

    void CreateComponentHierarchyTest(UWorld& World, const FAssetHandle& MeshHandle, const FAssetHandle& PipelineHandle, const FAssetHandle& MaterialHandle, const UMesh* Mesh) {
        AActor* Actor = World.AdoptActor<AActor>();
        UStaticMeshComponent* Root = AddTestStaticMesh(Actor, MeshHandle, PipelineHandle, MaterialHandle, { -12.0f, 0.0f, 8.0f });
        if (Root == nullptr || !Actor->SetRootComponent(Root)) {
            return;
        }

        USceneComponent* Parent = Root;
        for (uint32 Index = 0; Index < 4; ++Index) {
            UStaticMeshComponent* Child = AddTestStaticMesh(Actor, MeshHandle, PipelineHandle, MaterialHandle, { 0.0f, 0.0f, 2.0f });
            if (Child == nullptr || !Child->AttachToComponent(Parent)) {
                return;
            }
            Parent = Child;
        }

        AddTestCollider(Actor, Root, Root);
    }

    void CreateActorHierarchyTest(UWorld& World, const FAssetHandle& MeshHandle, const FAssetHandle& PipelineHandle, const FAssetHandle& MaterialHandle, const UMesh* Mesh) {
        AActor* ParentActor = World.AdoptActor<AActor>();
        UStaticMeshComponent* ParentRoot = AddTestStaticMesh(ParentActor, MeshHandle, PipelineHandle, MaterialHandle, { 12.0f, 0.0f, 8.0f });
        if (ParentRoot == nullptr || !ParentActor->SetRootComponent(ParentRoot)) {
            return;
        }
        AddTestCollider(ParentActor, ParentRoot, ParentRoot);

        AActor* ChildActor = World.AdoptActor<AActor>();
        UStaticMeshComponent* ChildRoot = AddTestStaticMesh(ChildActor, MeshHandle, PipelineHandle, MaterialHandle, { 0.0f, 0.0f, 3.0f });
        if (ChildRoot == nullptr || !ChildActor->SetRootComponent(ChildRoot)) {
            return;
        }

        if (!ChildRoot->AttachToComponent(ParentRoot)) {
            return;
        }

        AddTestCollider(ChildActor, ChildRoot, ChildRoot);
    }

    void CreateHierarchyTests(UWorld& World, const FAssetHandle& MeshHandle, const FAssetHandle& PipelineHandle, const FAssetHandle& MaterialHandle, const UMesh* Mesh) {
        CreateComponentHierarchyTest(World, MeshHandle, PipelineHandle, MaterialHandle, Mesh);
        CreateActorHierarchyTest(World, MeshHandle, PipelineHandle, MaterialHandle, Mesh);
    }
}

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
	TypeRegistry::Register(UTexturedMaterial::StaticTypeInfo());
	TypeRegistry::Register(UTexture::StaticTypeInfo());
    TypeRegistry::Register(AActor::StaticTypeInfo());
    TypeRegistry::Register(UFont::StaticTypeInfo());
    TypeRegistry::Register(UKFont::StaticTypeInfo());


	TypeRegistry::Register(UWorld::StaticTypeInfo());
	TypeRegistry::Register(AActor::StaticTypeInfo());
	TypeRegistry::Register(UCameraComponent::StaticTypeInfo());
	TypeRegistry::Register(UStaticMeshComponent::StaticTypeInfo());
    TypeRegistry::Register(UCollisionComponent::StaticTypeInfo());
    TypeRegistry::Register(UBoxColliderComponent::StaticTypeInfo());
	TypeRegistry::Register(UActorComponent::StaticTypeInfo());
	TypeRegistry::Register(USceneComponent::StaticTypeInfo());
	TypeRegistry::Register(UCollisionComponent::StaticTypeInfo());
    TypeRegistry::Register(UTextRenderComponent::StaticTypeInfo());
    TypeRegistry::Register(UKTextRenderComponent::StaticTypeInfo());
	


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
    FWorldEditorContext EditorContext{};
    World.SetEditorContext(&EditorContext);
	EditorContext.SetWorld(&World);

    Renderer.Create(gHWND, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);


    FAssetRegistry AssetRegistry;
    AssetRegistry.Initialize(Renderer.GetDevice(), 128);
    Renderer.BindAssetRegistry(&AssetRegistry);


    FMessageChannel WorldCommandChannel{ 64 };
    EditorContext.InitializeChannels(AssetRegistry, Renderer.GetDevice());

    

    EditorViewport EditorView{};
    EditorView.Initialize(Renderer.GetDevice(), AssetRegistry, Renderer.GetWindowInfoReader(), EditorContext);

    FEditorUIManager EditorUIManager;

    EditorUIManager.Initialize(World, EditorContext, gHWND, EditorView.GetGizmoMode(), EditorView.GetGizmoCoordinateSpace());

    GMouseInput.InitializeWorldCommandSender(WorldCommandChannel.GetSender());
    GKeyboardInput.InitializeWorldCommandSender(WorldCommandChannel.GetSender());
	World.SetWindowInfoReader(Renderer.GetWindowInfoReader());
	World.SetAssetRegistry(&AssetRegistry);

    WorldCommandChannel.TryBind<FMousePickRequestMessage>(
        [&World](const FMousePickRequestMessage& Message) { World.HandleMousePickRequest(Message); });

    WorldCommandChannel.TryBind<FMouseCameraRotateRequestMessage>(
        [&World](const FMouseCameraRotateRequestMessage& Message)
        {
            World.HandleMouseCameraRotateRequest(Message);
        });

    WorldCommandChannel.TryBind<
        FKeyboardCameraMoveRequestMessage>(
            [&World](
                const FKeyboardCameraMoveRequestMessage& Message)
            {
                World.HandleKeyboardCameraMoveRequest(Message);
            });

  
	
    if constexpr (bLoadTestScene) {
		World.LoadScene("./scenes/test.json", Renderer.GetDevice(), &AssetRegistry);
    } else {
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

	AssetRegistry.EmplaceAsset<UPipeline>(Renderer.GetDevice(), "TexturedPipeline", "./Content/Metadata/TexturedTestPipeline.meta");
	AssetRegistry.EmplaceAsset<UTexture>(Renderer.GetDevice(), "PlankTexture", "./Content/Metadata/TexturedTestTexture.meta");
	AssetRegistry.EmplaceAsset<UTexturedMaterial>(Renderer.GetDevice(), "TexturedMaterial", "./Content/Metadata/TexturedTestMaterial.meta");

    FAssetHandle TextPipelineHandle = AssetRegistry.EmplaceAsset<UPipeline>(Renderer.GetDevice(),"TextPipeline", "./Content/Metadata/TextPipeline.meta");
    FAssetHandle FontTextureHandle =AssetRegistry.EmplaceAsset<UTexture>( Renderer.GetDevice(), "AsciiFontTexture","./Content/Metadata/AsciiFontTexture.meta");
    FAssetHandle FontHandle = AssetRegistry.EmplaceAsset<UFont>(Renderer.GetDevice(),"AsciiFont");
    FAssetHandle KFontHandle = AssetRegistry.EmplaceAsset<UKFont>( Renderer.GetDevice(),"KoreanFont","./Content/Metadata/NotoSansKR.meta");

    UFont* Font = AssetRegistry.ResolveAsset<UFont>(KFontHandle);
    AActor* TextActor = World.AdoptActor<AActor>();

    if (TextActor != nullptr)
    {
        UKTextRenderComponent* TextComponent = TextActor->AddComponent<UKTextRenderComponent>();
        TextActor->SetRootComponent(TextComponent);
        TextComponent->SetFontHandle(KFontHandle);
        TextComponent->SetPipelineHandle(TextPipelineHandle);
        TextComponent->SetCharacterHeight(0.5f);
        TextComponent->SetLetterSpacing(0.0f);
        TextComponent->SetLineSpacing(0.0f);
        TextComponent->SetColor( FVector4{1.0f,1.0f,1.0f, 1.0f});
        TextComponent->SetText(FString{ "크래프톤 정글3주차"});
        TextComponent->GetComponentTransform().SetPosition(FVector3{0.0f, 0.0f, 0.0f});
    }

    const FAssetHandle MeshHandle = AssetRegistry.GetAsset("CubeMesh");
    const FAssetHandle PipelineHandle = AssetRegistry.GetAsset("BasePipeline");
    const FAssetHandle MaterialHandle = AssetRegistry.GetAsset("GreyMaterial");
    CreateHierarchyTests(World, MeshHandle, PipelineHandle, MaterialHandle, AssetRegistry.ResolveAsset<UMesh>(MeshHandle));

    AActor* CameraActor = World.AdoptActor<AActor>();
    UCameraComponent* Camera = CameraActor->AddComponent<UCameraComponent>();

    Camera->SetMoveSensitivity(World.GetSettings().MoveSensitivity);
    Camera->SetRotationSensitivity(World.GetSettings().RotationSensitivity);
    CameraActor->SetRootComponent(Camera);

    }
    AssetRegistry.Finalize(); 

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplWin32_Init((void*)hWnd);
    ImGui_ImplDX11_Init(Renderer.GetDevice(), Renderer.GetDeviceContext());

    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; 

    io.Fonts->AddFontFromFileTTF("./Content/Font/NotoSansKR-Medium.ttf", 16.0f, nullptr, io.Fonts->GetGlyphRangesKorean());

    auto LastTickTime = std::chrono::steady_clock::now();

    //char BufferA[256] = "Player";
    //char BufferB[256] = "player";

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

			ImGui_ImplDX11_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();
			const ImGuiID DockSpaceId = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
			EditorUIManager.Tick();

			ImGui::SetNextWindowDockID(DockSpaceId, ImGuiCond_FirstUseEver);
			ImGui::Begin("Viewport###SceneViewport");
			const ImVec2 SceneViewportPosition = ImGui::GetCursorScreenPos();
			const ImVec2 SceneViewportSize = ImGui::GetContentRegionAvail();
			const ImVec2 MainViewportPosition = ImGui::GetMainViewport()->Pos;
			const bool bSceneViewportHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
			const bool bSceneViewportFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
			const uint32 SceneViewportWidth = static_cast<uint32>(std::max(0.0f, SceneViewportSize.x));
			const uint32 SceneViewportHeight = static_cast<uint32>(std::max(0.0f, SceneViewportSize.y));
			Renderer.ResizeSceneSurface(SceneViewportWidth, SceneViewportHeight, SceneViewportPosition.x - MainViewportPosition.x, SceneViewportPosition.y - MainViewportPosition.y);

			// 입력 상태는 WndProc의 ProcessWindowMessage에서 갱신한다.
			EditorView.ProcessInput(GKeyboardInput, GMouseInput, !bSceneViewportHovered);

			GMouseInput.DispatchPendingWorldCommands(SceneViewportWidth, SceneViewportHeight, !bSceneViewportHovered);
			GKeyboardInput.DispatchPendingWorldCommands(DeltaTime, !bSceneViewportFocused || ImGui::GetIO().WantCaptureKeyboard);

            WorldCommandChannel.Dispatch();
            World.Tick(DeltaTime);

			EditorContext.Dispatch();

			//UndoCommandChannel.Dispatch();

			FRenderProbe& Probe{ World.BuildRenderProbe() };
			Renderer.BeginSceneRender();
			EditorView.RenderInProbe(Probe);
			Renderer.RenderScene(Probe);
            EditorView.RenderSceneGuides(Renderer.GetDeviceContext(),Probe);
			Renderer.RenderGizmos(Probe);
			EditorView.RenderOrientationAxis(Renderer.GetDeviceContext(),Probe.MainCameraProbe);
			ImGui::Image(reinterpret_cast<ImTextureID>(Renderer.GetSceneShaderResourceView()), SceneViewportSize);
			ImGui::End();
            
            //ImGui::Begin("FName Test");      
            //ImGui::Separator();
            //
            //ImGui::InputText("String A", BufferA, sizeof(BufferA));
            //ImGui::InputText("String B", BufferB, sizeof(BufferB));

            //FName NameA(BufferA);
            //FName NameB(BufferB);

            //bool bIsEqual = (NameA == NameB);
            //if (bIsEqual)
            //{
            //    ImGui::Text("operator== : true");               
            //}
            //else
            //{
            //    ImGui::Text("operator== : false");            
            //}

            //ImGui::Text("=== 2. Display Result (Case Preservation) ===");
            //ImGui::Text("A.ToString() : \"%s\"", NameA.ToString().c_str());
            //ImGui::Text("B.ToString() : \"%s\"", NameB.ToString().c_str());
            //ImGui::End();

			ImGui::Render();
			Renderer.BeginUiRender();
			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            
            Renderer.EndFrame();

            GMouseInput.EndFrame();
        }
    }
    
    FEditorConfigManager::Save(World.GetSettings());

    // ImGui 소멸
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    if constexpr (bEnableSceneSave) {
		World.SaveScene("test", &AssetRegistry);
    }

    Renderer.Terminate();
    Renderer.ReportLiveObjects(); 
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
    if (const auto result = ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam)) {
        return result;
    }

    GMouseInput.ProcessWindowMessage(message, wParam, lParam);

    GKeyboardInput.ProcessWindowMessage(message, wParam, lParam);

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



#include "PCH.h"
#include "FControlPanel.h"

#include <windows.h>
#include <commdlg.h>
#include <filesystem>

void FControlPanel::DrawPanel()  
{
    // 1. 상태 채널에서 카메라 정보 읽기 (Engine -> UI)
    if (EditorContext != nullptr && EditorContext->GetCameraState() != nullptr)
    {
        CachedCamPos = EditorContext->GetCameraState()->Position;
        CachedCamRot = EditorContext->GetCameraState()->Rotation;
        CachedFOV = EditorContext->GetCameraState()->FOV;
    }

    ImGui::Begin("Control Panel");

    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Separator();

    // 2. 단방향 채널: 스폰 이벤트 전송 (UI -> Engine)
    ImGui::Text("Spawn Primitive");

    const char* PrimitiveTypes[] =
    {
        "CubeMesh",
        "SphereMesh",
        "PlaneMesh",
        "CylinderMesh",
        "CapsuleMesh",
        "ConeMesh",
        "TorusMesh",
        "PyrimidMesh"
    };

    ImGui::Combo(
        "Type",
        &SelectedPrimitiveIndex,
        PrimitiveTypes,
        IM_ARRAYSIZE(PrimitiveTypes));

    ImGui::InputInt(
        "Number of Objects to Spawn",
        &SpawnCountToRequest);

    if (SpawnCountToRequest < 1)
    {
        SpawnCountToRequest = 1;
    }

    if (ImGui::Button("Spawn Object(s)"))
    {
        EditorToWorldSender.TryEmplace<FMessageSpawnPrimitive>(
            FString(PrimitiveTypes[SelectedPrimitiveIndex]),
            static_cast<uint32>(SpawnCountToRequest));
    }

    if (ImGui::Button("Delete Object"))
    {
        EditorToWorldSender.TryEmplace<FMessageDeletePrimitive>();
    }

    ImGui::Separator();

    // =====================================================
    // Scene
    // =====================================================

    ImGui::Text("Scene Management");

    ImGui::InputText(
        "Scene Name",
        SceneNameBuffer,
        IM_ARRAYSIZE(SceneNameBuffer));

    if (ImGui::Button("Save Scene"))
    {
        EditorToWorldSender.TryEmplace<FMessageSaveScene>(
            FString(SceneNameBuffer));
    }

    ImGui::SameLine();

    if (ImGui::Button("Load Scene"))
    {
        const FString FilePath =
            OpenFileDialog();

        if (!FilePath.empty())
        {
            EditorToWorldSender.TryEmplace<FMessageLoadScene>(
                FString(FilePath));
        }
        
        size_t SlashPos = FilePath.find_last_of("\\/");

        std::string FileName;
        if (SlashPos != std::string::npos)
            FileName = FilePath.substr(SlashPos + 1);
        else
            FileName = FilePath;

        size_t DotPos = FileName.find_last_of('.');
        if (DotPos != std::string::npos)
            FileName = FileName.substr(0, DotPos);

        if (FileName.size() < sizeof(SceneNameBuffer))
            std::memcpy(SceneNameBuffer, FileName.data(), FileName.size() + 1);
    }

    ImGui::Separator();

    // =====================================================
    // Camera
    // =====================================================

    ImGui::Text("Camera");

    bool bCameraChanged = false;

    float FOVDegrees =
        CachedFOV * 180.0f / 3.1415926535f;

    if (ImGui::SliderFloat(
        "FOV",
        &FOVDegrees,
        30.0f,
        120.0f))
    {
        CachedFOV =
            FOVDegrees * 3.1415926535f / 180.0f;

        bCameraChanged = true;
    }

    bCameraChanged |= ImGui::DragFloat3(
        "Location",
        &CachedCamPos.x,
        0.1f);

    bCameraChanged |= ImGui::DragFloat3(
        "Rotation",
        &CachedCamRot.x,
        0.01f);

    if (bCameraChanged)
    {
        EditorToWorldSender.TryEmplace<FMessageSetEditorCameraRequest>(
            CachedCamPos,
            CachedCamRot,
            CachedFOV);
    }

    ImGui::Separator();

    // =====================================================
    // Grid
    // =====================================================

    ImGui::Text("Grid");

    bool bGridChanged = false;

    float gridSize = EditorContext->GetGridSizeState();

    if (ImGui::SliderFloat(
        "GridSize",
        &gridSize,
        0.1f,
        100.0f))
    {
        bGridChanged = true;
    }

    if (bGridChanged)
    {
        EditorContext->SetGridSizeState(gridSize);
    }
    
    ImGui::Separator();

    // =====================================================
    // RenderMode
    // =====================================================
    bool bRenderModeChanged = false;
    int renderIndex = EditorContext->GetRenderModeState();

    const char* renderMode[] = { "Solid", "Lit", "Unlit", "Wireframe" };

    if (ImGui::Combo("Render Mode", &renderIndex, renderMode, IM_ARRAYSIZE(renderMode))) {
        bRenderModeChanged = true;
    }

    if (bRenderModeChanged)
    {
        EditorContext->SetRenderModeState(static_cast<size_t>(renderIndex));
    }
    ImGui::End();
}



FString FControlPanel::OpenFileDialog()
{
    char FileName[MAX_PATH] = { 0 };
    OPENFILENAMEA OpenFileName = { 0 };

    OpenFileName.lStructSize = sizeof(OpenFileName);
    OpenFileName.hwndOwner = WindowHandle;

    OpenFileName.lpstrFilter = "JSON Files (*.json)\0*.json\0All Files (*.*)\0*.*\0";
    OpenFileName.lpstrFile = FileName;
    OpenFileName.nMaxFile = MAX_PATH;

    OpenFileName.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
    OpenFileName.lpstrDefExt = "json";

    std::string InitialDirectoryPath = std::filesystem::absolute("./scenes").string();

    if (!std::filesystem::exists(InitialDirectoryPath))
    {
        std::filesystem::create_directories(InitialDirectoryPath);
    }

    OpenFileName.lpstrInitialDir = InitialDirectoryPath.c_str();

    if (GetOpenFileNameA(&OpenFileName))
    {
        return FString(FileName);
    }

    return "";
}

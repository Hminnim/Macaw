#include "PCH.h"
#include "FControlPanel.h"

#include <windows.h>
#include <commdlg.h>
#include <filesystem>

#include "../../Serialize/FEditorConfigManager.h"
#include "../../Scene/UWorld.h"
void FControlPanel::DrawPanel()  
{
    // 1. 상태 채널에서 카메라 정보 읽기 (Engine -> UI)
    if (EditorContext != nullptr && EditorContext->GetCameraState() != nullptr)
    {
        CachedCamPos = EditorContext->GetCameraState()->Position;
        CachedCamRot = EditorContext->GetCameraState()->Rotation;
        CachedFOV = EditorContext->GetCameraState()->FOV;
    }

    // 전역 메뉴 바는 뷰포트의 상단에 고정되며 도킹 레이아웃의 일부가 아니다.
    if (!ImGui::BeginMainMenuBar())
    {
        return;
    }

    const char* PrimitiveTypes[] =
    {
        "CubeMesh", "SphereMesh", "PlaneMesh", "CylinderMesh",
        "CapsuleMesh", "ConeMesh", "TorusMesh", "PyrimidMesh"
    };

    // Create: 기존의 Primitive 생성/삭제 기능을 한 그룹으로 유지한다.
    if (ImGui::BeginMenu("Create"))
    {
        ImGui::TextDisabled("Spawn Primitive");
        ImGui::SetNextItemWidth(180.0f);
        ImGui::Combo("Type", &SelectedPrimitiveIndex, PrimitiveTypes, IM_ARRAYSIZE(PrimitiveTypes));
        ImGui::InputInt("Number of Objects to Spawn", &SpawnCountToRequest);

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

        ImGui::EndMenu();
    }

    // Scene: 저장과 불러오기, 씬 이름 편집을 기존과 같은 흐름으로 제공한다.
    if (ImGui::BeginMenu("Scene"))
    {
        ImGui::SetNextItemWidth(220.0f);
        ImGui::InputText("Scene Name", SceneNameBuffer, IM_ARRAYSIZE(SceneNameBuffer));

        if (ImGui::Button("Save Scene"))
        {
            EditorToWorldSender.TryEmplace<FMessageSaveScene>(FString(SceneNameBuffer));
        }

        ImGui::SameLine();
        if (ImGui::Button("Load Scene"))
        {
            const FString FilePath = OpenFileDialog();

            if (!FilePath.empty())
            {
                EditorToWorldSender.TryEmplace<FMessageLoadScene>(FString(FilePath));
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

        ImGui::EndMenu();
    }

    // Camera: 카메라 요청 메시지와 감도 설정을 한 팝업에 모은다.
    if (ImGui::BeginMenu("Camera"))
    {
        bool bCameraChanged = false;
        float FOVDegrees = CachedFOV * 180.0f / 3.1415926535f;

        if (ImGui::SliderFloat("FOV", &FOVDegrees, 30.0f, 120.0f))
        {
            CachedFOV = FOVDegrees * 3.1415926535f / 180.0f;
            bCameraChanged = true;
        }

        bCameraChanged |= ImGui::DragFloat3("Location", &CachedCamPos.x, 0.1f);
        bCameraChanged |= ImGui::DragFloat3("Rotation", &CachedCamRot.x, 0.01f);

        if (bCameraChanged)
        {
            EditorToWorldSender.TryEmplace<FMessageSetEditorCameraRequest>(
                CachedCamPos, CachedCamRot, CachedFOV);
        }

        float MoveSensitivity = EditorContext->GetWorld()->GetSettings().MoveSensitivity;
        if (ImGui::SliderFloat("MoveSensitivity", &MoveSensitivity, 0.1f, 10.0f))
        {
            EditorContext->GetWorld()->GetSettings().MoveSensitivity = MoveSensitivity;
        }

        float RotationSensitivity = EditorContext->GetWorld()->GetSettings().RotationSensitivity;
        if (ImGui::SliderFloat("RotationSensitivity", &RotationSensitivity, 0.1f, 5.0f))
        {
            EditorContext->GetWorld()->GetSettings().RotationSensitivity = RotationSensitivity;
        }

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Grid"))
    {
        float GridSize = EditorContext->GetWorld()->GetSettings().GridSize;
        if (ImGui::SliderFloat("GridSize", &GridSize, 0.1f, 100.0f))
        {
            EditorContext->GetWorld()->GetSettings().GridSize = GridSize;
        }
        ImGui::EndMenu();
    }

    ImGui::Separator();
    int RenderIndex = static_cast<int>(EditorContext->GetRenderModeState());
    const char* RenderModes[] = { "Solid", "Lit", "Unlit", "Wireframe" };
    ImGui::SetNextItemWidth(110.0f);
    if (ImGui::Combo("Render Mode", &RenderIndex, RenderModes, IM_ARRAYSIZE(RenderModes)))
    {
        EditorContext->SetRenderModeState(static_cast<size_t>(RenderIndex));
    }

    // 남은 공간의 오른쪽 끝에 성능 정보를 고정한다.
    const char* FpsText = "FPS: %.1f";
    const float FpsWidth = ImGui::CalcTextSize("FPS: 000.0").x;
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - FpsWidth - ImGui::GetStyle().WindowPadding.x);
    ImGui::Text(FpsText, ImGui::GetIO().Framerate);

    ImGui::EndMainMenuBar();
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

#pragma once

#include "PCH.h"
#include "Core/Asset/FAssetHandle.h"
#include "Core/Base/FTransform.h"

class AActor;
class FWorldEditorContext;
class UActorComponent;
class USceneComponent;
class UPrimitiveComponent;
class UMeshComponent;
class UStaticMeshComponent;
class UCameraComponent;
class UCollisionComponent;
class UBoxColliderComponent;
class UTextRenderComponent;

// Component가 ImGui 세부 사항을 직접 알 필요 없이 속성 위젯을 그릴 수 있게 하는 Editor 측 컨텍스트입니다.
class FPropertyEditorContext {
public:
    explicit FPropertyEditorContext(FWorldEditorContext& InEditorContext);

    void DrawActorComponentProperties(UActorComponent& Component);
    void DrawSceneComponentProperties(USceneComponent& Component);
    void DrawPrimitiveComponentProperties(UPrimitiveComponent& Component);
    void DrawMeshComponentProperties(UMeshComponent& Component);
    void DrawStaticMeshComponentProperties(UStaticMeshComponent& Component);
    void DrawCameraComponentProperties(UCameraComponent& Component);
    void DrawCollisionComponentProperties(UCollisionComponent& Component);
    void DrawBoxColliderComponentProperties(UBoxColliderComponent& Component);
    void DrawTextRenderComponentProperties(UTextRenderComponent& Component);

private:
    void DrawTransform(USceneComponent& Component);
    void DrawAttachment(AActor& Actor, USceneComponent& Component);
    void DrawParentPicker(AActor& Actor, USceneComponent& Component);
    void UpdateTransformFields(const FTransform& Transform);
    FTransform BuildDesiredTransform() const;

    template<typename TAsset, typename TSetter>
    void DrawAssetPicker(const char* Label, UActorComponent& Component, FAssetHandle CurrentHandle, TSetter&& SetHandle);

private:
    FWorldEditorContext* EditorContext = nullptr;
    USceneComponent* EditingTransformTarget = nullptr;
    FVector3 EditPosition{};
    FRotator EditRotation{};
    FVector3 EditScale{ 1.0f, 1.0f, 1.0f };
    bool bAbsoluteLocation = false;
    bool bAbsoluteRotation = false;
    bool bAbsoluteScale = false;
};

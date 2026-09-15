#include "PCH.h"
#include "UNameTagComponent.h"

#include "Scene/AActor.h"

#include "Core/Base/UObjectSystem.h"

void UNameTagComponent::SetTargetActor(AActor* InTargetActor)
{
    if (InTargetActor == nullptr || InTargetActor == GetOwner())
    {
        TargetActor.Reset();
        ExplicitTargetGuid = {};
    }
    else
    {
        TargetActor.Set(InTargetActor);
        ExplicitTargetGuid = InTargetActor->GetGuid();
    }
    RefreshGuidText();
}

void UNameTagComponent::OnRegister()
{
    UBillboardTextComponent::OnRegister();
    RefreshGuidText();
}

AActor* UNameTagComponent::GetTargetActor() const
{
    if (!ExplicitTargetGuid.IsValid())
    {
        return GetOwner();
    }
    return TargetActor.Get();
}

void UNameTagComponent::SetTargetLocalOffset(const FVector3& InOffset)
{
    TargetLocalOffset = InOffset;
}

const FVector3& UNameTagComponent::GetTargetLocalOffset() const
{
    return TargetLocalOffset;
}

const FVector3& UNameTagComponent::GetObjectOffset() const
{
    return TargetLocalOffset;
}

FGuid UNameTagComponent::GetObjectGuid() const
{
    if (ExplicitTargetGuid.IsValid())
    {
        return ExplicitTargetGuid;
    }

    const AActor* Owner = GetOwner();

    return Owner != nullptr ? Owner->GetGuid() : FGuid{};
}

bool UNameTagComponent::TryGetBillBoardWorld(FMatrix& OutWorld) const
{
    if (!CanRenderBillBoard())
    {
        return false;
    }

    AActor* Target = GetTargetActor();

    if (Target == nullptr || Target->GetRootComponent() == nullptr)
    {
        return false;
    }

    const FMatrix TargetWorld = Target->GetActorTransform().ToMatrixWithScale();

    // TargetLocalOffset이 Target의 로컬 공간 Offset이므로 Target의 회전과 scale까지 적용한다.    
    const FVector3 AnchorWorld = TargetWorld.TransformPosition(TargetLocalOffset);
    // NameTag 컴포넌트 자신의 scale 등은 유지하고, 렌더링 원점만 Target 위치로 교체한다.
    // 현재 Text Shader는 World에서 translation만 사용하므로 실질적으로 AnchorWorld가 Billboard 원점이 된다.
    OutWorld = GetComponentToWorld();
    OutWorld.Translation(AnchorWorld);

    return true;
}

void UNameTagComponent::Serialize(FArchive& Archive)
{
    UBillboardTextComponent::Serialize(Archive);
    Archive.Serialize("TargetActorGuid", ExplicitTargetGuid);
    Archive.Serialize("TargetLocalOffset", TargetLocalOffset);
    if (Archive.IsLoading())
    {
        TargetActor.Reset();
    }
}

bool UNameTagComponent::ResolveLoadedReferences()
{
    if (!UBillboardTextComponent::ResolveLoadedReferences())
    {
        return false;
    }
    if (!ExplicitTargetGuid.IsValid())
    {
        return GetOwner() != nullptr;
    }
    const FObjectHandle TargetHandle = UObjectSystem::FindHandleByGuid(ExplicitTargetGuid);
    UObject* object = UObjectSystem::Resolve(TargetHandle);
    if (object == nullptr || !object->GetTypeInfo()->IsA(AActor::StaticTypeInfo()))
    {
        return false;
    }
    TargetActor.Set(static_cast<AActor*>(object));
    return true;
}

void UNameTagComponent::RefreshGuidText()
{
    const FGuid TargetGuid = GetObjectGuid();

    if (TargetGuid.IsValid())
    {
        SetText(TargetGuid.ToString());
    }
    else
    {
        SetText("");
    }
}

void UBillboardTextComponent::DrawPanels(FPropertyEditorContext& Context)
{
    /*
    UBillboardComponent::DrawPanels(Context);

    if (!Context.BeginCategory("Billboard Text"))
    {
        return;
    }

 
    Context.DrawColor("Color", GetColor(), [this](const FVector4& NewColor) { SetColor(NewColor);});
    Context.DrawFloat("Character Height", GetCharacterHeight(), 0.01f, 0.001f, 1000.0f, [this](float NewHeight) {SetCharacterHeight(NewHeight);});
    Context.DrawFloat("Letter Spacing", GetLetterSpacing(), 0.01f, -100.0f, 100.0f, [this](float NewSpacing) {SetLetterSpacing(NewSpacing);});
    Context.DrawFloat("Line Spacing", GetLineSpacing(), 0.01f, -100.0f, 100.0f, [this](float NewSpacing) {SetLineSpacing(NewSpacing); });

    AActor* Owner = GetOwner();
    UWorld* World = Owner != nullptr ? Owner->GetWorld() : nullptr;
    FAssetRegistry* Registry = World != nullptr ? World->GetAssetRegistry() : nullptr;

    if (Registry == nullptr)
    {
        Context.DrawDisabledText("Font/Pipeline: Asset registry unavailable");
        return;
    }

    Context.DrawAssetPicker("Font", *Registry, *UFont::StaticTypeInfo(), GetFontHandle(), [this](FAssetHandle NewHandle) {SetFontHandle(NewHandle);});
    Context.DrawAssetPicker("Pipeline", *Registry, *UPipeline::StaticTypeInfo(), GetPipelineHandle(), [this](FAssetHandle NewHandle) {SetPipelineHandle(NewHandle); });
    */
}
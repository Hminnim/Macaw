#include "PCH.h"
#include "USubUVComponent.h"

#include "Core/Asset/FAssetRegistry.h"
#include "Core/Asset/UTexture.h"

#include "Render/Panel/FPropertyEditorContext.h"
#include "Render/Pipeline/UPipeline.h"

#include "Scene/AActor.h"
#include "Scene/UWorld.h"
#include "Scene/Subsystem/URenderSubsystem.h"

void USubUVComponent::SetSubImage(int32 InHorizontal, int32 InVertical, int32 InTotalFrame, float InFrameRate, bool bInLooping)
{
	SubImageHorizontal = InHorizontal;
	SubImageVertical = InVertical;
	TotalFrame = InTotalFrame;
	FrameRate = InFrameRate;
	bLooping = bInLooping;
}
void USubUVComponent::SetFrameRate(float InFrameRate)
{
	FrameRate = InFrameRate;
}

void USubUVComponent::SetCurrentFrame(int32 inFrame)
{
	CurrentFrameIndex = inFrame;
}

void USubUVComponent::PlaySubUV()
{
	bPlaying = true;
}

void USubUVComponent::PauseSubUV()
{
	bPlaying = false;
}

void USubUVComponent::StopSubUV()
{
	bPlaying = false;
	SetCurrentFrame(0);
}

void USubUVComponent::RestartSubUV()
{
	ElapsedTime = 0.0f;
	CurrentFrameIndex = 0;
	bPlaying = true;
}

bool USubUVComponent::IsPlaying() const
{
	return bPlaying;
}

bool USubUVComponent::IsLooping() const
{
	return bLooping;
}

void USubUVComponent::UpdateUVFromCurrentFrame()
{
	if (SubImageHorizontal <= 0 || SubImageVertical <= 0)
	{
		return;
	}

	const int32 Col = CurrentFrameIndex % SubImageHorizontal;
	const int32 Row = CurrentFrameIndex / SubImageHorizontal;

	const float UWidth = 1.0f / static_cast<float>(SubImageHorizontal);
	const float VHeight = 1.0f / static_cast<float>(SubImageVertical);

	FVector2 NewUVMin = FVector2{ Col * UWidth, Row * VHeight };
	FVector2 NewUVMax = FVector2{ (Col + 1) * UWidth, (Row + 1) * VHeight };
	UBillboardComponent::SetUV(NewUVMin, NewUVMax);
}

void USubUVComponent::Tick(float DeltaTime)
{
	UBillboardComponent::Tick(DeltaTime);

	if (!bPlaying || TotalFrame <= 1 || FrameRate <= 0.0f || DeltaTime <= 0.0f)
	{
		return;
	}

	ElapsedTime += DeltaTime;

	const int32 TargetFrame = static_cast<int32>(ElapsedTime * FrameRate);
	if (bLooping)
	{
		CurrentFrameIndex = TargetFrame % TotalFrame;
	}
	else
	{
		CurrentFrameIndex = std::min(TargetFrame, TotalFrame - 1);
		if (TargetFrame >= TotalFrame)
		{
			bPlaying = false;
		}
	}

	UpdateUVFromCurrentFrame();
}

void USubUVComponent::DrawPanels(FPropertyEditorContext& Context)
{
	UBillboardComponent::DrawPanels(Context);
	
	Context.DrawFloat("FrameRate", FrameRate, 1.0f, 0.0f, 240.0f, [this](float NewRate) { SetFrameRate(NewRate); });
	
}

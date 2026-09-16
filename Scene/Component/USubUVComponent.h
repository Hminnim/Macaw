#pragma once
#include "Scene/Component/UBillboardComponent.h"

#include "Core/Asset/FAssetHandle.h"
#include "Core/Base/FRenderProbe.h"

class FPropertyEditorContext;

class USubUVComponent : public UBillboardComponent
{
public:
	USubUVComponent() = default;
	~USubUVComponent() override = default;

	JG_DECLARE_DERIVED_TYPEINFO(USubUVComponent, UBillboardComponent);	

	// SubUV
	void SetSubImage(int32 InHorizontal, int32 InVertical, int32 InTotalFrame, float InFrameRate, bool bInLooping);
	void SetFrameRate(float InFrameRate);
	void SetCurrentFrame(int32 inFrame);

	void PlaySubUV();
	void PauseSubUV();
	void StopSubUV();
	void RestartSubUV();

	bool IsPlaying() const;
	bool IsLooping() const;

	void UpdateUVFromCurrentFrame();

	void Tick(float DeltaTime) override;

	void DrawPanels(FPropertyEditorContext& Context) override;

private:
	int32 SubImageHorizontal = 1;
	int32 SubImageVertical = 1;
	int32 TotalFrame = 1;
	float FrameRate = 30.0f;
	bool bLooping = true;
	bool bPlaying = true;

	float ElapsedTime = 0.0f;
	int32 CurrentFrameIndex = 0;
};
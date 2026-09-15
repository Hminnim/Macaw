#pragma once

#include "ILineRenderer.h"
class FBatchLineRenderer : public ILineRenderer {
private:

	struct FBatchLineInstance {
		FVector3 Position{};
		FVector4 Color{};
	};

	struct FLineFrameConstants {
		FMatrix ViewProjection{};
		FVector4 Viewport{};
	};

	static_assert(sizeof(FLineFrameConstants) == sizeof(uint32) * 20);

	struct FLineBatch {
		TArray<FBatchLineInstance> Vertices{};
		FGraphicsBuffer VertexBuffer{};
		uint32 Capacity{ 0 };
	};

public:
	FBatchLineRenderer() = default;
	~FBatchLineRenderer() = default;

	FBatchLineRenderer(const FBatchLineRenderer&) = delete;
	FBatchLineRenderer& operator=(const FBatchLineRenderer&) = delete;

	FBatchLineRenderer(FBatchLineRenderer&&) noexcept = default;
	FBatchLineRenderer& operator=(FBatchLineRenderer&&) noexcept = default;

public:
	void Initialize(ID3D11Device* InDevice, uint32 InitialLineCapacity = 1024);
	void Reset();

	void AddLine(const FVector3& Start, const FVector3& End, const FVector4& Color, float WidthPixels = 1.0f, ELineDepthMode DepthMode = ELineDepthMode::DepthTested);
	void AddRay(const FVector3& Origin, const FVector3& Direction, float Length, const FVector4& Color, float WidthPixels = 1.0f, ELineDepthMode DepthMode = ELineDepthMode::DepthTested);

	void Render(ID3D11DeviceContext* Context, const FLineViewData& ViewData);
	void Clear();

	[[nodiscard]] uint32 GetLineCount() const;
	[[nodiscard]] bool IsEmpty() const;

private:
	bool CreateVertexBuffer(ID3D11Device* InDevice, FLineBatch& Batch, uint32 Capacity);
	bool EnsureCapacity(ID3D11Device* InDevice, FLineBatch& Batch, uint32 RequiredCapacity);
	bool RenderBatch(ID3D11Device* InDevice, ID3D11DeviceContext* Context, FLineBatch& Batch, const UPipeline* Pipeline);

private:
	ID3D11Device* Device{ nullptr };

	std::unique_ptr<UPipeline> DepthTestedPipeline{};
	std::unique_ptr<UPipeline> OverlayPipeline{};

	FLineBatch DepthTestedBatch{};
	FLineBatch OverlayBatch{};

	TGraphicsRootConstants<20> FrameConstants{};
};

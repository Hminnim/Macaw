#pragma once

#include "ILineRenderer.h"

class FLineRenderer : public ILineRenderer {
private:
	struct FQuadVertex {
		FVector2D Corner{};
	};

	struct FLineInstance {
		FVector4 StartAndWidth{};
		FVector4 EndAndPadding{};
		FVector4 Color{};
	};

	struct FLineFrameConstants {
		FMatrix ViewProjection{};
		FVector4 Viewport{};
	};

	static_assert(sizeof(FLineFrameConstants) == sizeof(uint32) * 20);

	struct FLineBatch {
		TArray<FLineInstance> Instances{};
		FGraphicsBuffer InstanceBuffer{};
		uint32 Capacity{ 0 };
	};

public:
	FLineRenderer() = default;
	~FLineRenderer() = default;

	FLineRenderer(const FLineRenderer&) = delete;
	FLineRenderer& operator=(const FLineRenderer&) = delete;

	FLineRenderer(FLineRenderer&&) noexcept = default;
	FLineRenderer& operator=(FLineRenderer&&) noexcept = default;

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
	bool CreateQuadGeometry(ID3D11Device* Device);
	bool CreateInstanceBuffer(ID3D11Device* Device, FLineBatch& Batch, uint32 Capacity);
	bool EnsureCapacity(ID3D11Device* Device, FLineBatch& Batch, uint32 RequiredCapacity);
	bool RenderBatch(ID3D11Device* Device, ID3D11DeviceContext* Context, FLineBatch& Batch, const UPipeline* Pipeline);

private:
	ID3D11Device* Device{ nullptr };

	std::unique_ptr<UPipeline> DepthTestedPipeline{};
	std::unique_ptr<UPipeline> OverlayPipeline{};

	FGraphicsBuffer QuadVertexBuffer{};
	FGraphicsBuffer QuadIndexBuffer{};

	FLineBatch DepthTestedBatch{};
	FLineBatch OverlayBatch{};

	TGraphicsRootConstants<20> FrameConstants{};
};

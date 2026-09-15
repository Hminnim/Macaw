#include "PCH.h"
#include "FBatchLineRender.h"

void FBatchLineRenderer::Initialize(ID3D11Device* InDevice, uint32 InitialLineCapacity)
{
	ErrorHandler::Report(InDevice == nullptr, "[ FLineRenderer ]", "Invalid device pointer.", ErrorHandler::EErrorLevel::Critical);
	ErrorHandler::Report(InitialLineCapacity == 0, "[ FLineRenderer ]", "Initial line capacity must be greater than zero.", ErrorHandler::EErrorLevel::Critical);

	Reset();

	Device = InDevice;
	DepthTestedPipeline = std::make_unique<UPipeline>();
	OverlayPipeline = std::make_unique<UPipeline>();

	DepthTestedPipeline->Initialize(Device, "./Content/Metadata/DepthTestedBatchLine.meta");
	OverlayPipeline->Initialize(Device, "./Content/Metadata/OverlayBatchLine.meta");

	InitialLineCapacity = std::max(InitialLineCapacity * 2, 2u);

	ErrorHandler::Report(not CreateVertexBuffer(Device, DepthTestedBatch, InitialLineCapacity), "[ FLineRenderer ]", "Failed to create instance buffer for depth-tested lines.", ErrorHandler::EErrorLevel::Critical);
	ErrorHandler::Report(not CreateVertexBuffer(Device, OverlayBatch, InitialLineCapacity), "[ FLineRenderer ]", "Failed to create instance buffer for overlay lines.", ErrorHandler::EErrorLevel::Critical);
	ErrorHandler::Report(not FrameConstants.Initialize(Device), "[ FLineRenderer ]", "Failed to initialize frame constants.", ErrorHandler::EErrorLevel::Critical);


	DepthTestedBatch.Vertices.reserve(InitialLineCapacity);
	OverlayBatch.Vertices.reserve(InitialLineCapacity);
}

void FBatchLineRenderer::Reset()
{
	DepthTestedBatch.VertexBuffer.Reset();
	DepthTestedBatch.Vertices.clear();
	DepthTestedBatch.Capacity = 0;

	OverlayBatch.VertexBuffer.Reset();
	OverlayBatch.Vertices.clear();
	OverlayBatch.Capacity = 0;

	FrameConstants.Reset();

	DepthTestedPipeline = nullptr;
	OverlayPipeline = nullptr;
	Device = nullptr;
}

void FBatchLineRenderer::AddLine(const FVector3& Start, const FVector3& End, const FVector4& Color, float WidthPixels, ELineDepthMode DepthMode)
{
	if (WidthPixels <= 0.0f || (End - Start).LengthSquared() <= 0.0f) {
		return;
	}

	FLineBatch& Batch = DepthMode == ELineDepthMode::DepthTested ? DepthTestedBatch : OverlayBatch;

	Batch.Vertices.emplace_back(FBatchLineInstance{
		.Position = FVector3{ Start.x, Start.y, Start.z },
		.Color = Color
		});
	Batch.Vertices.emplace_back(FBatchLineInstance{
		.Position = FVector3{ End.x, End.y, End.z },
		.Color = Color
		});
}

void FBatchLineRenderer::AddRay(const FVector3& Origin, const FVector3& Direction, float Length, const FVector4& Color, float WidthPixels, ELineDepthMode DepthMode)
{
	if (Length <= 0.0f || Direction.LengthSquared() <= 0.0f) {
		return;
	}

	FVector3 NormalizedDirection = Direction;
	NormalizedDirection.Normalize();

	AddLine(Origin, Origin + NormalizedDirection * Length, Color, WidthPixels, DepthMode);
}

void FBatchLineRenderer::Render(ID3D11DeviceContext* Context, const FLineViewData& ViewData)
{
	ErrorHandler::Report(Device == nullptr, "[ FLineRenderer ]", "Invalid device pointer.", ErrorHandler::EErrorLevel::Critical);
	ErrorHandler::Report(Context == nullptr, "[ FLineRenderer ]", "Invalid device context pointer.", ErrorHandler::EErrorLevel::Critical);
	ErrorHandler::Report(ViewData.ViewportSize.x <= 0.0f || ViewData.ViewportSize.y <= 0.0f, "[ FLineRenderer ]", "Invalid viewport size.", ErrorHandler::EErrorLevel::Critical);
	ErrorHandler::Report(DepthTestedPipeline == nullptr, "[ FLineRenderer ]", "Depth-tested pipeline is not initialized.", ErrorHandler::EErrorLevel::Critical);

	const FLineFrameConstants Constants{
		.ViewProjection = ViewData.ViewProjection,
		.Viewport = FVector4{
			ViewData.ViewportSize.x,
			ViewData.ViewportSize.y,
			1.0f / ViewData.ViewportSize.x,
			1.0f / ViewData.ViewportSize.y
		}
	};

	ErrorHandler::Report(not FrameConstants.SetGraphicsRoot32BitConstants(Constants), "[ FLineRenderer ]", "Failed to set frame constants.", ErrorHandler::EErrorLevel::Critical);
	ErrorHandler::Report(not FrameConstants.Bind(Context, 0, EGraphicsShaderStage::Vertex), "[ FLineRenderer ]", "Failed to bind frame constants.", ErrorHandler::EErrorLevel::Critical);

	ErrorHandler::Report(not RenderBatch(Device, Context, DepthTestedBatch, DepthTestedPipeline.get()), "[ FLineRenderer ]", "Failed to render depth-tested lines.", ErrorHandler::EErrorLevel::Critical);
	ErrorHandler::Report(not RenderBatch(Device, Context, OverlayBatch, OverlayPipeline.get()), "[ FLineRenderer ]", "Failed to render overlay lines.", ErrorHandler::EErrorLevel::Critical);

	Clear();
}

void FBatchLineRenderer::Clear()
{
	DepthTestedBatch.Vertices.clear();
	OverlayBatch.Vertices.clear();
}

uint32 FBatchLineRenderer::GetLineCount() const
{
	return static_cast<uint32>(DepthTestedBatch.Vertices.size() + OverlayBatch.Vertices.size());
}

bool FBatchLineRenderer::IsEmpty() const
{
	return DepthTestedBatch.Vertices.empty() && OverlayBatch.Vertices.empty();
}

bool FBatchLineRenderer::CreateVertexBuffer(ID3D11Device* InDevice, FLineBatch& Batch, uint32 Capacity)
{
	if (InDevice == nullptr || Capacity == 0)	return false;

	FGraphicsBufferDescription Description{};
	Description.ByteSize = static_cast<uint32>(Capacity * sizeof(FBatchLineInstance));
	Description.Usage = D3D11_USAGE_DYNAMIC;
	Description.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	Description.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	FGraphicsBuffer NewBuffer{};
	if (!NewBuffer.Initialize(InDevice, Description))	return false;

	Batch.VertexBuffer = std::move(NewBuffer);
	Batch.Capacity = Capacity;
	return true;
}

bool FBatchLineRenderer::EnsureCapacity(ID3D11Device* InDevice, FLineBatch& Batch, uint32 RequiredCapacity)
{
	if (RequiredCapacity <= Batch.Capacity) {
		return true;
	}

	uint32 NewCapacity = std::max(Batch.Capacity, 1u);

	while (NewCapacity < RequiredCapacity) {
		if (NewCapacity > std::numeric_limits<uint32>::max() / 2) {
			NewCapacity = RequiredCapacity;
			break;
		}

		NewCapacity *= 2;
	}

	return CreateVertexBuffer(InDevice, Batch, NewCapacity);
}

bool FBatchLineRenderer::RenderBatch(ID3D11Device* InDevice, ID3D11DeviceContext* Context, FLineBatch& Batch, const UPipeline* Pipeline)
{
	if (Batch.Vertices.empty()) {
		return true;
	}

	if (InDevice == nullptr || Context == nullptr || Pipeline == nullptr) {
		return false;
	}

	const uint32 VertexCount = static_cast<uint32>(Batch.Vertices.size());
	const uint32 VertexByteSize = static_cast<uint32>(VertexCount * sizeof(FBatchLineInstance));

	if (!EnsureCapacity(InDevice, Batch, VertexCount) || !Batch.VertexBuffer.WriteDiscard(Context, Batch.Vertices.data(), VertexByteSize)) {
		return false;
	}

	Pipeline->Bind(Context);

	ID3D11Buffer* VertexBuffers[]{ Batch.VertexBuffer.GetBuffer() };
	const uint32 Strides[]{ static_cast<uint32>(sizeof(FBatchLineInstance)) };
	const uint32 Offsets[]{ 0 };

	Context->IASetVertexBuffers(0, 1, VertexBuffers, Strides, Offsets);

	Context->Draw(VertexCount, 0);

	return true;
}

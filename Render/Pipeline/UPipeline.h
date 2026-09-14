#pragma once

#include <d3d11.h>
#include <wrl/client.h>

#include <filesystem>
#include <string>
#include <vector>

#include "FShader.h"

#include "Defines.h"
#include "../../Core/Asset/UAsset.h"
#include "../../Core/Base/TypeInfo.h"
#include "Wrapper.h"

struct PipelineUnit {
    FShader VertexShader{};
    FShader PixelShader{};
    FShader GeometryShader{};

    Microsoft::WRL::ComPtr<ID3D11InputLayout> InputLayout;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> RasterizerState;
    Microsoft::WRL::ComPtr<ID3D11BlendState> BlendState;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> DepthStencilState;

    D3D11_PRIMITIVE_TOPOLOGY PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    bool Initialized{ false };
};

enum class ERenderMode : size_t {
    Solid,
    Lit,
    Unlit,
    Wireframe,
    Max
};

class UPipeline : public UAsset {
public:
    UPipeline() = default;
    ~UPipeline() = default;

    UPipeline(const UPipeline&) = delete;
    UPipeline& operator=(const UPipeline&) = delete;

    UPipeline(UPipeline&&) noexcept = default;
    UPipeline& operator=(UPipeline&&) noexcept = default;

public:
	JG_DECLARE_DERIVED_TYPEINFO(UPipeline, UAsset);

	virtual void Initialize(ID3D11Device* Device, const std::filesystem::path& metaData) override;

    void Bind(ID3D11DeviceContext* Context) const;
    void Reset();

    void SetRenderMode(ERenderMode mode);
    bool RenderModeSettable(ERenderMode mode);

private:
    bool LoadPipelineDescription(const std::filesystem::path& Path, FPipelineDescription& OutDescription);

	bool Make(ID3D11Device* Device, const FPipelineDescription& Description, PipelineUnit& PipelineUnit);
protected:
	virtual void Serialize(FArchive& Ar) override;

private:
	std::filesystem::path OptionFilePath{};
    
    ERenderMode Mode{ ERenderMode::Solid };

    TFixedArray<PipelineUnit, static_cast<size_t>(ERenderMode::Max)> Pipelines{};

    size_t PrimaryIndex{ 0 };
};



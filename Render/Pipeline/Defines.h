#pragma once 
#include <filesystem>

enum class EVertexFormat {
    Float2,
    Float3,
    Float4
};

enum class EInputClassification {
    PerVertex,
    PerInstance
};

enum class EPrimitiveTopology {
    PointList,
    LineList,
    LineStrip,
    TriangleList,
    TriangleStrip
};

enum class EFillMode {
    Solid,
    Wireframe
};

enum class ECullMode {
    None,
    Front,
    Back
};

enum class ECompareFunc {
    Never,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always
};

enum class EBlend {
    Zero,
    One,
    SrcAlpha,
    InvSrcAlpha,
    DestAlpha,
    InvDestAlpha,
    SrcColor,
    InvSrcColor,
    DestColor,
    InvDestColor
};

enum class EBlendOp {
    Add,
    Subtract,
    RevSubtract,
    Min,
    Max
};

struct FInputElementDescription {
    std::string SemanticName{};
    UINT SemanticIndex{ 0 };
    EVertexFormat Format{ EVertexFormat::Float3 };
    UINT InputSlot{ 0 };
    UINT AlignedByteOffset{ 0 };
    EInputClassification InputClassification{ EInputClassification::PerVertex };
    UINT InstanceDataStepRate{ 0 };
};

struct FRasterizerDescription {
    EFillMode FillMode{ EFillMode::Solid };
    ECullMode CullMode{ ECullMode::Back };
    bool FrontCounterClockwise{ false };
    bool DepthClipEnable{ true };
    bool ScissorEnable{ false };
};

struct FDepthStencilDescription {
    bool DepthEnable{ true };
    bool DepthWriteEnable{ true };
    ECompareFunc DepthFunc{ ECompareFunc::LessEqual };
};

struct FBlendDescription {
    bool BlendEnable{ false };
    EBlend SrcBlend{ EBlend::One };
    EBlend DestBlend{ EBlend::Zero };
    EBlendOp BlendOp{ EBlendOp::Add };
    EBlend SrcBlendAlpha{ EBlend::One };
    EBlend DestBlendAlpha{ EBlend::Zero };
    EBlendOp BlendOpAlpha{ EBlendOp::Add };
};

enum class EShaderStage {
    Vertex,
    Pixel,
    Geometry,
    Hull,
    Domain,
    Compute
};

struct FShaderDescription {
    std::filesystem::path Source;
    std::string EntryPoint;
    std::string Profile;
    EShaderStage Stage = EShaderStage::Vertex;
};

struct FPipelineDescription {
    FShaderDescription VertexShader{};
    FShaderDescription PixelShader{};

    FShaderDescription GeometryShader{};
    bool bHasGeometryShader = false;

    std::vector<FInputElementDescription> InputLayout{};

    EPrimitiveTopology PrimitiveTopology{ EPrimitiveTopology::TriangleList };

    FRasterizerDescription Rasterizer{};
    FDepthStencilDescription DepthStencil{};
    FBlendDescription Blend{};
};

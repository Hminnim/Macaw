#include "PCH.h"
#include "UMesh.h"

#include "../../ErrorHandler.h"
#include "FAssetMetadataParser.h"

#include "BasicGeometry/Capsule.h"
#include "BasicGeometry/Corn.h"
#include "BasicGeometry/Cube.h"
#include "BasicGeometry/Cylinder.h"
#include "BasicGeometry/Pyramid.h"
#include "BasicGeometry/Plane.h"
#include "BasicGeometry/Sphere.h"
#include "BasicGeometry/Torus.h"

void UMesh::Initialize(ID3D11Device* Device, const std::filesystem::path& metaData) {
	UAsset::Initialize(Device, metaData);

	FAssetMetadataParser MetadataParser{};

	ErrorHandler::Report(not MetadataParser.Load(AssetMetaDataPath), " [ UMesh ]", "Failed to load metadata", ErrorHandler::EErrorLevel::Critical);

	if (MetadataParser.GetOr("BasicMesh", false)) {
		const FString MeshType = MetadataParser.GetOr("MeshType", FString{});
		
		if (MeshType == "Plane") {
			UMesh::Make(Device, BasicGeometry::Plane::Indices,
				MakeVertexAttribute<EVertexAttribute::Position>(BasicGeometry::Plane::Positions),
				MakeVertexAttribute<EVertexAttribute::Normal>(BasicGeometry::Plane::Normals),
				MakeVertexAttribute<EVertexAttribute::UV>(BasicGeometry::Plane::TexCoords)
			);	
		} 
		else if (MeshType == "Cube") {
			UMesh::Make(Device, BasicGeometry::Cube::Indices,
				MakeVertexAttribute<EVertexAttribute::Position>(BasicGeometry::Cube::Positions),
				MakeVertexAttribute<EVertexAttribute::Normal>(BasicGeometry::Cube::Normals),
				MakeVertexAttribute<EVertexAttribute::UV>(BasicGeometry::Cube::TexCoords)
			);
		} 
		else if (MeshType == "Sphere") {
			UMesh::Make(Device, BasicGeometry::Sphere::Indices,
				MakeVertexAttribute<EVertexAttribute::Position>(BasicGeometry::Sphere::Positions),
				MakeVertexAttribute<EVertexAttribute::Normal>(BasicGeometry::Sphere::Normals),
				MakeVertexAttribute<EVertexAttribute::UV>(BasicGeometry::Sphere::TexCoords)
			);
		} 
		else if (MeshType == "Capsule") {
			UMesh::Make(Device, BasicGeometry::Capsule::Indices,
				MakeVertexAttribute<EVertexAttribute::Position>(BasicGeometry::Capsule::Positions),
				MakeVertexAttribute<EVertexAttribute::Normal>(BasicGeometry::Capsule::Normals),
				MakeVertexAttribute<EVertexAttribute::UV>(BasicGeometry::Capsule::TexCoords)
			);
		}
		else if (MeshType == "Cone") {
			UMesh::Make(Device, BasicGeometry::Cone::Indices,
				MakeVertexAttribute<EVertexAttribute::Position>(BasicGeometry::Cone::Positions),
				MakeVertexAttribute<EVertexAttribute::Normal>(BasicGeometry::Cone::Normals),
				MakeVertexAttribute<EVertexAttribute::UV>(BasicGeometry::Cone::TexCoords)
			);
		}
		else if (MeshType == "Cylinder") {
			UMesh::Make(Device, BasicGeometry::Cylinder::Indices,
				MakeVertexAttribute<EVertexAttribute::Position>(BasicGeometry::Cylinder::Positions),
				MakeVertexAttribute<EVertexAttribute::Normal>(BasicGeometry::Cylinder::Normals),
				MakeVertexAttribute<EVertexAttribute::UV>(BasicGeometry::Cylinder::TexCoords)
			);
		}
		else if (MeshType == "Pyramid") {
			UMesh::Make(Device, BasicGeometry::Pyramid::Indices,
				MakeVertexAttribute<EVertexAttribute::Position>(BasicGeometry::Pyramid::Positions),
				MakeVertexAttribute<EVertexAttribute::Normal>(BasicGeometry::Pyramid::Normals),
				MakeVertexAttribute<EVertexAttribute::UV>(BasicGeometry::Pyramid::TexCoords)
			);
		}
		else if (MeshType == "Torus") {
			float MajorRadius = MetadataParser.GetOr("MajorRadius",BasicGeometry::Torus::MajorRadius);
			float MinorRadius = MetadataParser.GetOr("MinorRadius",BasicGeometry::Torus::MinorRadius);

			if (MajorRadius <= 0.0f) 
			{
				MajorRadius = BasicGeometry::Torus::MajorRadius;
			}

			if (MinorRadius <= 0.0f)
			{
				MinorRadius = BasicGeometry::Torus::MinorRadius;
			}

			const BasicGeometry::Torus::FGeometry Geometry =BasicGeometry::Torus::GenerateGeometry(MajorRadius,MinorRadius);
			UMesh::Make(Device, BasicGeometry::Torus::Indices,
				MakeVertexAttribute<EVertexAttribute::Position>(Geometry.Positions),
				MakeVertexAttribute<EVertexAttribute::Normal>(Geometry.Normals),
				MakeVertexAttribute<EVertexAttribute::UV>(Geometry.TexCoords)
			);
		}
		else {
			ErrorHandler::Report(false, " [ UMesh ]", "Unsupported BasicMesh type: " + MeshType, ErrorHandler::EErrorLevel::Critical);
		}
	}
}

ID3D11Buffer* UMesh::GetVertexBuffer(EVertexAttribute Attribute) const {
	const size_t Index = GetAttributeIndex(Attribute);

	if (Index >= GetAttributeCount()) {
		return nullptr;
	}

	return VertexBuffers[Index].Get();
}

ID3D11Buffer* UMesh::GetIndexBuffer() const {
	return IndexBuffer.Get();
}

bool UMesh::HasVertexAttribute(EVertexAttribute Attribute) const {
	const size_t Index = GetAttributeIndex(Attribute);

	if (Index >= GetAttributeCount()) {
		return false;
	}

	return AttributeStorage[Index] != nullptr;
}

uint32 UMesh::GetVertexStride(EVertexAttribute Attribute) const {
	const size_t Index = GetAttributeIndex(Attribute);

	if (Index >= GetAttributeCount() || !AttributeStorage[Index]) {
		return 0;
	}

	return AttributeStorage[Index]->GetStride();
}

uint32 UMesh::GetVertexAttributeCount(EVertexAttribute Attribute) const {
	const size_t Index = GetAttributeIndex(Attribute);

	if (Index >= GetAttributeCount() || !AttributeStorage[Index]) {
		return 0;
	}

	return AttributeStorage[Index]->GetCount();
}

const void* UMesh::GetVertexData(EVertexAttribute Attribute) const {
	const size_t Index = GetAttributeIndex(Attribute);

	if (Index >= GetAttributeCount() || !AttributeStorage[Index]) {
		return nullptr;
	}

	return AttributeStorage[Index]->GetData();
}

void UMesh::Serialize(FArchive& Ar) {
	UAsset::Serialize(Ar);
}

bool UMesh::CreateIndexBuffer(ID3D11Device* Device, const std::span<const uint32>& InIndices) {
	if (Device == nullptr || InIndices.empty()) {
		return false;
	}

	const size_t ByteSize = InIndices.size_bytes(); 

	if (ByteSize > std::numeric_limits<UINT>::max()) {
		return false;
	}

	D3D11_BUFFER_DESC BufferDesc{};
	BufferDesc.ByteWidth = static_cast<UINT>(ByteSize);
	BufferDesc.Usage = D3D11_USAGE_DEFAULT;
	BufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	BufferDesc.CPUAccessFlags = 0;
	BufferDesc.MiscFlags = 0;
	BufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA InitialData{};
	InitialData.pSysMem = InIndices.data();

	Microsoft::WRL::ComPtr<ID3D11Buffer> Buffer;

	const HRESULT Result = Device->CreateBuffer(&BufferDesc, &InitialData, Buffer.GetAddressOf());

	if (FAILED(Result)) {
		return false;
	}

	IndexBuffer = std::move(Buffer);

	Indices.assign(InIndices.begin(), InIndices.end());

	return true;
}

void UMesh::Reset() {
	for (Microsoft::WRL::ComPtr<ID3D11Buffer>& Buffer : VertexBuffers) {
		Buffer.Reset();
	}

	for (std::unique_ptr<FVertexAttributeStorageBase>& Storage : AttributeStorage) {
		Storage.reset();
	}

	IndexBuffer.Reset();
	Indices.clear();

	LocalBoundingBox = DirectX::BoundingBox{};
}

void UMesh::CalculateBounds() {
	const auto Positions = GetVertexAttributeData<EVertexAttribute::Position>();

	std::vector<DirectX::XMFLOAT3> boundsPoints;
    boundsPoints.reserve(Positions.size());
    for (const FVector3& position : Positions)
        boundsPoints.emplace_back(position.x, position.y, position.z);
    LocalBoundingBox = {};
    if (!boundsPoints.empty())
        DirectX::BoundingBox::CreateFromPoints(LocalBoundingBox, boundsPoints.size(), boundsPoints.data(), sizeof(DirectX::XMFLOAT3));
}
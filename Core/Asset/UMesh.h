#pragma once 

#include "../Base/UObject.h"
#include "../../Render/Pipeline/Defines.h"
#include "../Base/FVertexAttribute.h"

#include <array>
#include <limits>
#include <memory>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

#include <d3d11.h>
#include <wrl/client.h>

#include "UAsset.h"
#include "../Base/TypeInfo.h"

class UMesh : public UAsset {
private:
	struct FVertexAttributeStorageBase {
		virtual ~FVertexAttributeStorageBase() = default;

		virtual const void* GetData() const = 0;
		virtual uint32 GetCount() const = 0;
		virtual uint32 GetStride() const = 0;
	};

	template<typename T>
	struct TVertexAttributeStorage final : FVertexAttributeStorageBase {
		explicit TVertexAttributeStorage(std::span<const T> InData)
			: Data(InData.begin(), InData.end()) {
		}

		const void* GetData() const override {
			return Data.data();
		}

		uint32 GetCount() const override {
			return static_cast<uint32>(Data.size());
		}

		uint32 GetStride() const override {
			return static_cast<uint32>(sizeof(T));
		}

		std::vector<T> Data{};
	};

public:
	UMesh() = default; 
	~UMesh() = default;

	UMesh(const UMesh&) = delete;
	UMesh& operator=(const UMesh&) = delete;

	UMesh(UMesh&&) noexcept = default;
	UMesh& operator=(UMesh&&) noexcept = default;

public:
	JG_DECLARE_DERIVED_TYPEINFO(UMesh, UAsset);
	
	virtual void Initialize(ID3D11Device* device, const std::filesystem::path& metaData) override;

	template<CVertexAttributeView... TAttributes>
	bool Make(ID3D11Device* Device, const std::span<const uint32>& InIndices, const TAttributes&... InAttributes) {
		static_assert(sizeof...(TAttributes) > 0, "UMesh requires at least one vertex attribute.");
		static_assert(AreVertexAttributesUnique<TAttributes...>(), "Duplicate vertex attributes are not allowed.");

		Reset();

		if (Device == nullptr || InIndices.empty()) {
			return false;
		}

		uint32 ExpectedVertexCount = 0;
		bool bFirstAttribute = true;
		bool bSuccess = true;

		auto ProcessAttribute = [&](const auto& InAttribute) {
			if (!bSuccess) {
				return;
			}

			if (InAttribute.Data.empty() || InAttribute.Data.size() > std::numeric_limits<uint32>::max()) {
				bSuccess = false;
				return;
			}

			const uint32 AttributeVertexCount = static_cast<uint32>(InAttribute.Data.size());

			if (bFirstAttribute) {
				ExpectedVertexCount = AttributeVertexCount;
				bFirstAttribute = false;
			}
			else if (AttributeVertexCount != ExpectedVertexCount) {
				bSuccess = false;
				return;
			}

			if (!CreateVertexBuffer(Device, InAttribute)) {
				bSuccess = false;
			}
		};

		(ProcessAttribute(InAttributes), ...);

		if (!bSuccess) {
			Reset();
			return false;
		}

		if (!CreateIndexBuffer(Device, InIndices)) {
			Reset();
			return false;
		}

		Indices.assign(InIndices.begin(), InIndices.end());

        return true;
	}

	ID3D11Buffer* GetVertexBuffer(EVertexAttribute Attribute) const;
	ID3D11Buffer* GetIndexBuffer() const;

	bool HasVertexAttribute(EVertexAttribute Attribute) const;

	uint32 GetVertexStride(EVertexAttribute Attribute) const;
	uint32 GetVertexAttributeCount(EVertexAttribute Attribute) const;

	const void* GetVertexData(EVertexAttribute Attribute) const;

	template<EVertexAttribute Attribute>
	std::span<const TVertexAttributeElementType<Attribute>> GetVertexAttributeData() const {
		using ElementType = TVertexAttributeElementType<Attribute>;
		using StorageType = TVertexAttributeStorage<ElementType>;

		constexpr size_t Index = static_cast<size_t>(Attribute);

		if (!AttributeStorage[Index]) {
			return {};
		}

		const StorageType* Storage = static_cast<const StorageType*>(AttributeStorage[Index].get());

		return std::span<const ElementType>{ Storage->Data.data(), Storage->Data.size() };
	}

	const TArray<uint32>& GetIndices() const {
		return Indices;
	}

protected:
	virtual void Serialize(FArchive& Ar) override;

private:
	template<typename... TAttributes>
	static consteval bool AreVertexAttributesUnique() {
		constexpr std::array<EVertexAttribute, sizeof...(TAttributes)> Attributes{ std::remove_cvref_t<TAttributes>::AttributeType... };

		for (size_t i = 0; i < Attributes.size(); ++i) {
			for (size_t j = i + 1; j < Attributes.size(); ++j) {
				if (Attributes[i] == Attributes[j]) {
					return false;
				}
			}
		}

		return true;
	}

	template<CVertexAttributeView TAttribute>
	bool CreateVertexBuffer(ID3D11Device* Device, const TAttribute& InAttribute) {
		using AttributeType = std::remove_cvref_t<TAttribute>;
		using ElementType = typename AttributeType::ElementType;

		constexpr EVertexAttribute Attribute = AttributeType::AttributeType;
		constexpr size_t AttributeIndex = static_cast<size_t>(Attribute);

		if (InAttribute.Data.empty() || InAttribute.Data.size_bytes() > std::numeric_limits<UINT>::max()) {
			return false;
		}

		D3D11_BUFFER_DESC BufferDesc{};
		BufferDesc.ByteWidth = static_cast<UINT>(InAttribute.Data.size_bytes());
		BufferDesc.Usage = D3D11_USAGE_DEFAULT;
		BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		BufferDesc.CPUAccessFlags = 0;
		BufferDesc.MiscFlags = 0;
		BufferDesc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA InitialData{};
		InitialData.pSysMem = InAttribute.Data.data();

		Microsoft::WRL::ComPtr<ID3D11Buffer> Buffer;

		const HRESULT Result = Device->CreateBuffer(&BufferDesc, &InitialData, Buffer.GetAddressOf());

		if (FAILED(Result)) {
			return false;
		}

		VertexBuffers[AttributeIndex] = std::move(Buffer);
		AttributeStorage[AttributeIndex] = std::make_unique<TVertexAttributeStorage<ElementType>>(InAttribute.Data);

		return true;
	}

	bool CreateIndexBuffer(ID3D11Device* Device, const std::span<const uint32>& InIndices);

	void Reset();

	static constexpr size_t GetAttributeIndex(EVertexAttribute Attribute) {
		return static_cast<size_t>(Attribute);
	}

	static constexpr size_t GetAttributeCount() {
		return static_cast<size_t>(EVertexAttribute::MAX);
	}

private:
	TFixedArray<Microsoft::WRL::ComPtr<ID3D11Buffer>, static_cast<size_t>(EVertexAttribute::MAX)> VertexBuffers{};
	TFixedArray<std::unique_ptr<FVertexAttributeStorageBase>, static_cast<size_t>(EVertexAttribute::MAX)> AttributeStorage{};

	Microsoft::WRL::ComPtr<ID3D11Buffer> IndexBuffer{ nullptr };

	TArray<uint32> Indices{};

};


#pragma once

#include "FGraphicsBuffer.h"

#include <algorithm>
#include <span>
#include <type_traits>

template<typename T, bool bAutoResize = true, bool bDynamic = false>
class TGraphicsArray {
	static_assert(std::is_trivially_copyable_v<T>, "TGraphicsArray requires trivially copyable element types.");
	static_assert(sizeof(T) % 4 == 0, "Structured buffer element size must be aligned to 4 bytes.");

public:
	TGraphicsArray() = default;
	~TGraphicsArray() = default;

	TGraphicsArray(const TGraphicsArray&) = delete;
	TGraphicsArray& operator=(const TGraphicsArray&) = delete;

	TGraphicsArray(TGraphicsArray&&) noexcept = default;
	TGraphicsArray& operator=(TGraphicsArray&&) noexcept = default;

public:
	bool Initialize(ID3D11Device* Device, ID3D11DeviceContext* Context, uint32 InCapacity, std::span<const T> InitialData = {}) {
		if (!Device || !Context || InCapacity == 0 || InitialData.size() > InCapacity) {
			return false;
		}

		Reset();

		FGraphicsBufferDescription Description{};
		Description.ByteSize = InCapacity * sizeof(T);
		Description.Stride = sizeof(T);
		Description.Usage = bDynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
		Description.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		Description.CPUAccessFlags = bDynamic ? D3D11_CPU_ACCESS_WRITE : 0;
		Description.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

		if (!Buffer.Initialize(Device, Description)) {
			return false;
		}

		if (!CreateSRV(Device, Buffer.GetBuffer(), InCapacity, SRV)) {
			Reset();
			return false;
		}

		Capacity = InCapacity;

		if (!InitialData.empty()) {
			const bool bUploaded = bDynamic ? Buffer.WriteDiscard(Context, InitialData.data(), static_cast<uint32>(InitialData.size_bytes())) : Buffer.Update(Context, InitialData.data(), static_cast<uint32>(InitialData.size_bytes()));
			if (!bUploaded) {
				Reset();
				return false;
			}

			Count = static_cast<uint32>(InitialData.size());
		}

		return true;
	}

	bool UploadDiscard(ID3D11Device* Device, ID3D11DeviceContext* Context, std::span<const T> Values) requires(bDynamic) {
		if (Values.empty()) {
			Count = 0;
			return true;
		}

		if (!EnsureCapacity(Device, Context, static_cast<uint32>(Values.size()))) {
			return false;
		}

		if (!Buffer.WriteDiscard(Context, Values.data(), static_cast<uint32>(Values.size_bytes()))) {
			return false;
		}

		Count = static_cast<uint32>(Values.size());
		return true;
	}

	bool Add(ID3D11Device* Device, ID3D11DeviceContext* Context, const T& Value) {
		if (!EnsureCapacity(Device, Context, Count + 1)) {
			return false;
		}

		if (!Buffer.Update(Context, &Value, sizeof(T), Count * sizeof(T))) {
			return false;
		}

		++Count;

		return true;
	}

	bool AddRange(ID3D11Device* Device, ID3D11DeviceContext* Context, std::span<const T> Values) {
		if (Values.empty()) {
			return true;
		}

		const uint32 AddCount = static_cast<uint32>(Values.size());

		if (AddCount > UINT32_MAX - Count) {
			return false;
		}

		const uint32 RequiredCapacity = Count + AddCount;

		if (!EnsureCapacity(Device, Context, RequiredCapacity)) {
			return false;
		}

		if (!Buffer.Update(Context, Values.data(), static_cast<uint32>(Values.size_bytes()), Count * sizeof(T))) {
			return false;
		}

		Count = RequiredCapacity;

		return true;
	}

	bool Update(ID3D11DeviceContext* Context, uint32 Index, const T& Value) {
		if (Index >= Count) {
			return false;
		}

		return Buffer.Update(Context, &Value, sizeof(T), Index * sizeof(T));
	}

	bool UpdateRange(ID3D11DeviceContext* Context, uint32 StartIndex, std::span<const T> Values) {
		if (Values.empty()) {
			return true;
		}

		if (StartIndex > Count || Values.size() > Count - StartIndex) {
			return false;
		}

		return Buffer.Update(Context, Values.data(), static_cast<uint32>(Values.size_bytes()), StartIndex * sizeof(T));
	}

	bool Reserve(ID3D11Device* Device, ID3D11DeviceContext* Context, uint32 NewCapacity) {
		if (NewCapacity <= Capacity) {
			return true;
		}

		return Resize(Device, Context, NewCapacity);
	}

	bool PopBack() {
		if (Count == 0) {
			return false;
		}

		--Count;

		return true;
	}

	void Clear() {
		Count = 0;
	}

	void Reset() {
		Buffer.Reset();
		SRV.Reset();

		Count = 0;
		Capacity = 0;
	}

public:
	[[nodiscard]] ID3D11Buffer* GetBuffer() const { return Buffer.GetBuffer(); }
	[[nodiscard]] ID3D11ShaderResourceView* const* GetSRV() const { return SRV.GetAddressOf(); }

	[[nodiscard]] uint32 GetCount() const { return Count; }
	[[nodiscard]] uint32 GetCapacity() const { return Capacity; }
	[[nodiscard]] uint32 GetStride() const { return sizeof(T); }
	[[nodiscard]] uint32 GetByteSize() const { return Capacity * sizeof(T); }

	[[nodiscard]] bool IsEmpty() const { return Count == 0; }
	[[nodiscard]] bool IsFull() const { return Count >= Capacity; }
	[[nodiscard]] bool IsValid() const { return Buffer.IsValid() && SRV != nullptr; }

private:
	bool EnsureCapacity(ID3D11Device* Device, ID3D11DeviceContext* Context, uint32 RequiredCapacity) {
		if (RequiredCapacity <= Capacity) {
			return true;
		}

		if constexpr (!bAutoResize) {
			return false;
		}

		uint32 NewCapacity = Capacity == 0 ? 1 : Capacity;

		while (NewCapacity < RequiredCapacity) {
			if (NewCapacity > UINT32_MAX / 2) {
				NewCapacity = RequiredCapacity;
				break;
			}

			NewCapacity *= 2;
		}

		return Resize(Device, Context, NewCapacity);
	}

	bool Resize(ID3D11Device* Device, ID3D11DeviceContext* Context, uint32 NewCapacity) {
		if (!Device || !Context || NewCapacity <= Capacity || NewCapacity < Count) {
			return false;
		}

		if (NewCapacity > UINT32_MAX / sizeof(T)) {
			return false;
		}

		FGraphicsBufferDescription Description{};
		Description.ByteSize = NewCapacity * sizeof(T);
		Description.Stride = sizeof(T);
		Description.Usage = bDynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
		Description.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		Description.CPUAccessFlags = bDynamic ? D3D11_CPU_ACCESS_WRITE : 0;
		Description.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

		FGraphicsBuffer NewBuffer;

		if (!NewBuffer.Initialize(Device, Description)) {
			return false;
		}

		if constexpr (!bDynamic) {
			if (Buffer.IsValid() && Count > 0) {
				if (!NewBuffer.CopyFrom(Context, 0, Buffer, 0, Count * sizeof(T))) {
					return false;
				}
			}
		}

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> NewSRV;

		if (!CreateSRV(Device, NewBuffer.GetBuffer(), NewCapacity, NewSRV)) {
			return false;
		}

		Buffer = std::move(NewBuffer);
		SRV = std::move(NewSRV);
		Capacity = NewCapacity;

		return true;
	}

	bool CreateSRV(ID3D11Device* Device, ID3D11Buffer* InBuffer, uint32 InCapacity, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& OutSRV) {
		if (!Device || !InBuffer || InCapacity == 0) {
			return false;
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC Description{};
		Description.Format = DXGI_FORMAT_UNKNOWN;
		Description.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		Description.Buffer.FirstElement = 0;
		Description.Buffer.NumElements = InCapacity;

		return SUCCEEDED(Device->CreateShaderResourceView(InBuffer, &Description, OutSRV.ReleaseAndGetAddressOf()));
	}

private:
	FGraphicsBuffer Buffer{};
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV{};

	uint32 Count{ 0 };
	uint32 Capacity{ 0 };
};

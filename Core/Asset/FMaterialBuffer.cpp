#include "PCH.h"
#include "FMaterialBuffer.h"
#include "UMaterial.h"

bool FMaterialBuffer::Initialize(ID3D11Device* Device, uint32 InMaxMaterialCount) {
    if (Device == nullptr || InMaxMaterialCount == 0) {
        return false;
    }

    MaxMaterialCount = InMaxMaterialCount;

    Slots.resize(MaxMaterialCount);
    Materials.resize(MaxMaterialCount, nullptr);

    FreeIndices.reserve(MaxMaterialCount);

    for (uint32 Index = 0; Index < MaxMaterialCount; ++Index) {
        FreeIndices.push_back(MaxMaterialCount - Index - 1);
    }

    D3D11_BUFFER_DESC BufferDesc{};
    BufferDesc.ByteWidth = MATERIAL_GPU_STRIDE * MaxMaterialCount;
    BufferDesc.Usage = D3D11_USAGE_DEFAULT;
    BufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    BufferDesc.CPUAccessFlags = 0;
    BufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    BufferDesc.StructureByteStride = MATERIAL_GPU_STRIDE;

    D3D11_SUBRESOURCE_DATA InitialData{};
    InitialData.pSysMem = Slots.data();

    HRESULT Result = Device->CreateBuffer(&BufferDesc, &InitialData, Buffer.GetAddressOf());

    if (FAILED(Result)) {
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
    SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
    SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    SRVDesc.Buffer.FirstElement = 0;
    SRVDesc.Buffer.NumElements = MaxMaterialCount;

    Result = Device->CreateShaderResourceView(Buffer.Get(), &SRVDesc, SRV.GetAddressOf());

    return SUCCEEDED(Result);
}

bool FMaterialBuffer::RegisterMaterial(UMaterial* Material) {
    if (Material == nullptr || FreeIndices.empty()) {
        return false;
    }

    const uint32 Index = AllocateSlot();

    Materials[Index] = Material;

    Material->GPUIndex = Index;

    return true;
}

void FMaterialBuffer::UnregisterMaterial(UMaterial* Material) {
    if (Material == nullptr || Material->GPUIndex == UINT32_MAX) {
        return;
    }

    const uint32 Index = Material->GPUIndex;

    Materials[Index] = nullptr;
    Slots[Index] = {};

    Material->GPUIndex = UINT32_MAX;
    Material->bGPUDataDirty = false;

    ReleaseSlot(Index);
}

void FMaterialBuffer::Flush(ID3D11DeviceContext* DeviceContext) {
    if (DeviceContext == nullptr || Buffer == nullptr) {
        return;
    }

    for (uint32 Index = 0; Index < MaxMaterialCount; ++Index) {
        UMaterial* Material = Materials[Index];

        if (Material == nullptr || !Material->bGPUDataDirty) {
            continue;
        }

        Material->BuildGPUData(Slots[Index]);

        const uint32 Offset = Index * MATERIAL_GPU_STRIDE;

        D3D11_BOX Box{};
        Box.left = Offset;
        Box.right = Offset + MATERIAL_GPU_STRIDE;
        Box.top = 0;
        Box.bottom = 1;
        Box.front = 0;
        Box.back = 1;

        DeviceContext->UpdateSubresource(Buffer.Get(), 0, &Box, Slots[Index].Data.data(), 0, 0);

        Material->bGPUDataDirty = false;
    }
}

uint32 FMaterialBuffer::AllocateSlot() {
    const uint32 Index = FreeIndices.back();
    FreeIndices.pop_back();

    return Index;
}

void FMaterialBuffer::ReleaseSlot(uint32 Index) {
    FreeIndices.push_back(Index);
}
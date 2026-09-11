#pragma once

#include "../Base/UObject.h"
#include "UAsset.h"
#include "FAssetHandle.h"
#include "FMaterialBuffer.h"
#include "FTextureArrayPool.h"
#include "UMaterial.h"
#include "UTexture.h"
#include "Common.h"

#include <d3d11.h>
#include <filesystem>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <ranges>

class FAssetRegistry : public IAssetQuery {
public:
    FAssetRegistry() = default;
    ~FAssetRegistry() = default;

    FAssetRegistry(const FAssetRegistry&) = delete;
    FAssetRegistry& operator=(const FAssetRegistry&) = delete;

    FAssetRegistry(FAssetRegistry&&) = delete;
    FAssetRegistry& operator=(FAssetRegistry&&) = delete;

public:
    bool Initialize(ID3D11Device* Device, uint32 MaxMaterialCount = 4096);

    FAssetHandle AdoptAsset(ID3D11Device* Device, const FGuid& ID, const FString& Name, const std::filesystem::path& MetadataPath, std::unique_ptr<UObject>&& Asset);

    template<typename T> requires std::is_base_of_v<UAsset, T>
    FAssetHandle EmplaceAsset(ID3D11Device* Device, const FString& Name, const std::filesystem::path& MetadataPath = {}) {
        std::unique_ptr<T> NewAsset = std::make_unique<T>();
        std::unique_ptr<UObject> Asset = std::move(NewAsset);

        return AdoptAsset(Device, FGuid::NewGuid(), Name, MetadataPath, std::move(Asset));
    }

    virtual FAssetHandle GetAsset(const FString& Name) const override;
    virtual FAssetHandle GetAsset(const FGuid& ID) const override;

    virtual UAsset* GetUAsset(const FString& Name) override;

    bool RemoveAsset(FAssetHandle Handle);

    template<typename T> requires std::is_base_of_v<UAsset, T>
    T* ResolveAsset(FAssetHandle Handle) {
        if (Handle.ID >= Assets.size()) {
            return nullptr;
        }

        auto& Entry = Assets[Handle.ID];

        if (Entry.first != Handle || Entry.second == nullptr) {
            return nullptr;
        }

        if (!Entry.second->GetTypeInfo()->IsA(T::StaticTypeInfo())) {
            return nullptr;
        }

        return static_cast<T*>(Entry.second.get());
    }

    template<typename T> requires std::is_base_of_v<UAsset, T>
    const T* ResolveAsset(FAssetHandle Handle) const {
        if (Handle.ID >= Assets.size()) {
            return nullptr;
        }

        const auto& Entry = Assets[Handle.ID];

        if (Entry.first != Handle || Entry.second == nullptr) {
            return nullptr;
        }

        if (!Entry.second->GetTypeInfo().IsA(T::StaticTypeInfo())) {
            return nullptr;
        }

        return static_cast<const T*>(Entry.second.get());
    }

    template<typename T, typename Func> requires std::is_base_of_v<UAsset, T>
    void ModifyAsset(FAssetHandle Handle, Func&& Modifier) {
        T* Asset = ResolveAsset<T>(Handle);

        if (Asset == nullptr) {
            return;
        }

        std::invoke(std::forward<Func>(Modifier), *Asset);
    }

    FMaterialBuffer& GetMaterialBuffer() {
        return MaterialBuffer;
    }

    const FMaterialBuffer& GetMaterialBuffer() const {
        return MaterialBuffer;
    }

    auto GetAssetList() {
        return Assets | std::ranges::views::transform([](auto& Pair) -> UObject* {
            return Pair.second.get();
            });
    }

    void Reset()
    {
        Assets.clear();
        FreeHandles.clear();
        AssetNameToHandle.clear();
        AssetIDToHandle.clear();
        MaterialBuffer.Reset();
        TexturePools.clear();
    }

    void Finalize();

private:
    FAssetHandle AllocateHandle();
    void RemoveHandleMappings(FAssetHandle Handle);

private:
    TArray<TPair<FAssetHandle, std::unique_ptr<UObject>>> Assets{};
    TArray<FAssetHandle> FreeHandles{};

    TMap<FString, FAssetHandle> AssetNameToHandle{};
    TMap<FGuid, FAssetHandle> AssetIDToHandle{};

    FMaterialBuffer MaterialBuffer{};
	
	TMap<FTextureProfile, FTextureArrayPool> TexturePools{};
};
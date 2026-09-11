#pragma once
#include <d3d11.h>
#include "UAsset.h"
#include "FTextureProfile.h"
#include <wrl/client.h>

#include <DirectXTex.h>
/*
================================================================================
 D3D11 Texture2DArray Profile Pooling / Material Chunking 설계
================================================================================

 [목표]
 - Texture2DArray는 범용 GPU 저장소다. 어떤 slice가 BaseColor, Normal, ORM,
   UI인지 같은 semantic은 알지 못한다.
 - Texture2DArray를 만들거나 선택하는 유일한 기준은 Texture Profile이다.
 - 각 shader texture field가 같은 Texture2DArray를 가리키는 material들은
   서로 다른 이미지(slice)와 상수 parameter를 사용해도 하나의 instanced draw에
   포함될 수 있다.

 [1. Texture Profile 하나는 정확히 Texture2DArray 하나다]

     struct FTextureProfile
     {
         uint32 Width;
         uint32 Height;
         uint32 MipLevels;       // 정규화된 full mip chain 정책
         DXGI_FORMAT Format;
         uint32 SampleCount;     // Texture2DArray.Sample 경로에서는 1
         uint32 SampleQuality;
     };

 - AssetRegistry는 ProfileId -> Texture2DArray/SRV/free-slice allocator를
   소유한다.
 - profile마다 physical Texture2DArray는 반드시 하나만 만든다. Profile이
   가득 찼다고 두 번째 "page"를 만들지 않는다. 이 경우는 asset 예산/로딩
   실패로 명시적으로 보고해야 한다.
 - MAX_TEXTURE_PROFILE_COUNT는 texture semantic의 개수가 아니라 profile의
   개수다. 예를 들어 Textures[4]는 profile #4를 만족하는 Color, Normal,
   ORM, UI 이미지를 모두 slice로 보관할 수 있다.
 - sampled pool은 D3D11 제한(ArraySize <= 2048)을 만족해야 한다. 아래의
   16-bit slice encoding은 이 하드웨어 제한보다 의도적으로 넓다.

 [2. Texture location encoding]

     // CPU의 authoritative 표현. Renderer는 GPU byte를 역해석하지 않는다.
     struct FTextureLocation
     {
         uint16 ProfileId;       // Textures[ProfileId] 선택
         uint16 SliceId;         // 해당 Texture2DArray 내부 이미지 선택
     };

     using FPackedTextureLocation = uint32;

     Pack(Location)   = (uint32(Location.ProfileId) << 16) | Location.SliceId;
     Profile(Packed)  = Packed >> 16;
     Slice(Packed)    = Packed & 0xFFFFu;

 - UTexture는 독립 Texture2D/SRV 대신 FTextureLocation(또는 packed 값과 typed
   CPU cache)을 보관한다.
 - Registry는 O(1) material lookup을 위해 TextureName -> FTextureLocation
   역색인도 소유한다.
 - packed uint32는 material GPU ABI다. bit operation은 shader에 전달할 값을
   직렬화할 때만 필요하며 CPU chunk 생성은 typed field를 사용한다.

 [3. Import / upload 계약]

     UTexture::Initialize(Device, Registry, Metadata)
       -> source image decode
       -> FTextureProfile 산출/정규화
       -> Registry.FindProfile(Profile)
       -> profile Texture2DArray의 빈 slice 할당
       -> source upload 후 FTextureLocation 반환

 - authored mip이 정규화된 full chain과 일치하면 모든 mip subresource를
   업로드한다.
 - mip이 없거나 부족하면 최종 array의 해당 slice Mip0만 업로드하고,
   그 slice만 보이는 SRV로 GenerateMips를 호출한다. 이 경로에는
   D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET 및
   D3D11_RESOURCE_MISC_GENERATE_MIPS가 필요하다.
 - Texture asset은 material보다 먼저 AdoptAsset한다. Material 초기화는 새
   texture asset을 만들지 않고 Registry에서 metadata texture 이름을 조회한다.

 [4. Material은 두 표현을 가진다]

     GPU: opaque FMaterialGPUSlot byte buffer
          - shader만 해석한다.
          - FPackedTextureLocation, scalar, flag 등을 포함한다.

     CPU: texture-aware concrete material 내부 binding buffer
          - metadata texture 이름으로 조회한 FTextureLocation을 cache한다.
          - 상수 전용 material은 texture binding buffer를 갖지 않는다.

 - Renderer는 profile을 얻기 위해 FMaterialGPUSlot byte를 역해석하면 안 된다.
 - texture-aware concrete UMaterial의 Initialize는 metadata texture 이름으로
   Registry에서 UTexture를 조회하고 UTexture::GetLocation()을 자기 내부 binding
   buffer에 저장한다. UColorMaterial 같은 상수 전용 material은 이 경로를 쓰지
   않는다.
 - concrete UMaterial은 virtual 함수로 자기 material chunk 기여분을 만든다.
   이 함수는 자기 내부에 cache한 texture-location field를 shader가 해석하는
   순서와 같은 순서로 builder에 추가한다.

       // Shader material struct: BaseColor, Normal, ORM 순서
       Builder.AddTexture(BaseColorLocation);
       Builder.AddTexture(NormalLocation);
       Builder.AddTexture(ORMLocation);

 [5. Concrete material과 shader struct의 순서 계약]

     Shader material struct의 texture field: [ A, B, C, D ]
     Chunk signature의 ProfileId field:    [ Pool(A), Pool(B), Pool(C), Pool(D) ]

 - Shader와 concrete material은 같은 texture-location field 순서를 공유해야
   한다. Builder는 각 location에서 ProfileId만 취하고 SliceId는 버린다.
 - texture field 구성이 다르면 optional slot이나 fallback texture로 처리하지
   않는다. [A, C, D]나 [A, D, F]처럼 다른 field sequence는 별도의 concrete
   material과 shader/material 구현 계약으로 분리한다.
 - Component/asset 설정이 맞는 Pipeline과 Material을 조합해야 하지만,
   UPipeline과 UMaterial은 서로를 참조하거나 런타임에 호환성을 검증하지 않는다.
   PipelineHandle은 오직 FRenderChunkKey의 독립적인 key 구성요소다.

     struct FMaterialChunkSignature
     {
         uint8 TextureFieldCount;
         uint16 ProfileIds[MAX_MATERIAL_TEXTURE_FIELDS];
     };

 - SliceId, BaseColor, Roughness, UV transform, flag 및 기타 모든 scalar 값은
   의도적으로 signature에서 제외한다.

 [6. Chunking 및 shader 접근]

     FRenderChunkKey = MeshHandle + PipelineHandle + MaterialChunkSignature

 - opaque pass는 FRenderChunkKey -> instances hash map을 직접 만들 수 있다.
   전체 actor sorting은 필수 조건이 아니라 draw 순서 최적화 수단이다.
 - 하나의 chunk에서는 texture field i마다 ProfileIds[i]가 draw-wide uniform임을
   보장한다. Instance별 SliceId는 서로 달라도 되며, 그래야 서로 다른 이미지를
   하나의 instanced draw에 넣을 수 있다.

     Texture2DArray Textures[MAX_TEXTURE_PROFILE_COUNT] : register(t2);

     uint PoolId  = PackedLocation >> 16;
     uint SliceId = PackedLocation & 0xFFFFu;
     float4 Value = Textures[PoolId].Sample(LinearWrap, float3(UV, SliceId));

 - Profile SRV는 t2부터 연속된 resource array로 bind한다. Shader source,
   compile target, MAX_TEXTURE_PROFILE_COUNT는 목표 D3D11 feature level에서
   하나의 단위로 검증해야 한다.

   MAX_MATERIAL_TEXTURE_FIELDS == 8 로 진행한다. 하나의 material shader가
   texture field를 8개보다 많이 해석해야 하는 경우에만 이 상한을 확장한다.

 [7. 현재 프로젝트에서 구현/수정할 파일]

 [새로 만들어야 하는 파일]

 - Core/Asset/FTextureProfile.h -> 완료 
   FTextureProfile, FTextureLocation, FPackedTextureLocation, 16+16 bit
   pack/unpack helper, MAX_TEXTURE_PROFILE_COUNT를 선언한다. CPU의 location은
   typed field로 유지하며, packed uint32는 GPU material ABI에만 사용한다.

 - Core/Asset/FTextureArrayPool.h / .cpp -> 완료 
   profile 하나에 대응하는 physical Texture2DArray와 SRV를 소유한다. 빈 slice
   할당/반납, source mip upload, single-slice SRV 기반 GenerateMips를 구현한다.
   profile이 가득 찬 경우 두 번째 page를 만들지 않고 실패를 반환한다.

 - Core/Asset/FMaterialChunkSignature.h -> 완료 
   MAX_MATERIAL_TEXTURE_FIELDS(현재 8)개의 ProfileId를 하나의 uint64에 담는
   FMaterialChunkSignature와 builder를 구현한다. Builder.AddTexture(Location)는
   SliceId를 버리고 ProfileId만 canonical texture field 순서대로 압축한다.

 - Core/Asset/UPBRMaterial.h / .cpp
   최초의 texture-aware concrete material을 구현한다. Initialize에서 metadata
   texture 이름으로 Registry의 UTexture를 조회하고 location을 자기 binding buffer와
   GPU byte layout에 cache한다. BuildChunkSignature에서는 shader struct 순서대로
   location을 builder에 전달한다.

 [수정해야 하는 파일]

 - Core/Asset/FAssetRegistry.h / .cpp
   Profile -> FTextureArrayPool, TextureName -> FTextureLocation 역색인,
   texture 등록/업로드 API, profile SRV array getter, profile 용량 초과 처리를
   추가한다.

 - Core/Asset/UTexture.h / .cpp
   독립 Texture2D/SRV 멤버를 제거한다. Source import 후 Registry에 slice 할당을
   요청하고, 반환받은 FTextureLocation만 보관하도록 바꾼다.

 - Core/Asset/UMaterial.h / .cpp
   AssetRegistry를 사용할 수 있도록 texture-aware subclass Initialize 경로를
   지원한다. Base UMaterial에는 virtual BuildChunkSignature와 cached
   FMaterialChunkSignature를 추가한다. UColorMaterial 같은 상수 전용 subclass는
   texture lookup/location cache를 추가하지 않는다.

 - Core/Asset/FMaterialGPUData.h
   UPBRMaterial이 사용할 packed texture-location field를 포함하는 128-byte GPU
   layout을 정의한다. C++ layout과 HLSL struct의 byte offset을 일치시킨다.

 - Content/Shader/Base.hlsl, Content/Shader/Alternate.hlsl
   Texture2DArray Textures[MAX_TEXTURE_PROFILE_COUNT] : register(t2), packed
   location unpack helper, UPBRMaterial과 일치하는 material struct 및 texture
   sample 코드를 추가한다.

 - Render/Renderer.cpp
   Mesh + Pipeline + MaterialChunkSignature를 key로 FRenderChunkKey hash group을
   만든다. Group별 ModelContext를 연속 upload하고 DrawIndexedInstanced를 한 번
   호출한다. Profile SRV array를 pixel shader t2부터 bind한다.

 - Macaw.cpp, Scene/UWorld.cpp
   bootstrap과 scene load 모두에서 texture asset을 texture-aware material보다
   먼저 AdoptAsset하도록 asset load 순서를 보장한다.
================================================================================
*/

class UTexture : public UAsset {
public:
    UTexture() {};
    ~UTexture() {};

	UTexture(const UTexture&) = delete;
	UTexture& operator=(const UTexture&) = delete;

	UTexture(UTexture&&) noexcept = default;
	UTexture& operator=(UTexture&&) noexcept = default;

public:
	JG_DECLARE_DERIVED_TYPEINFO(UTexture, UAsset);

    virtual void Initialize(ID3D11Device* device, const std::filesystem::path& metaData) override;

	FTextureProfile GetProfile() const { return Profile; }
	FTextureLocation GetLocation() const { return Location; }

    const byte* GetSourceImageData() const;
    const size_t GetSourceImageDataSize() const;
    const uint32 GetSourceImageRowPitch() const;

	void SetLocation(FTextureLocation InLocation) { Location = InLocation; }
protected:
	virtual void Serialize(FArchive& Ar) override;
private:
	FTextureProfile Profile{};
	FTextureLocation Location{};
    
	DirectX::ScratchImage SourceImage{};
	DirectX::TexMetadata SourceImageMetaData{};
};

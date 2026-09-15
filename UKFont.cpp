#include "PCH.h"
#include "UKFont.h"

#include "Core/Asset/FAssetMetadataParser.h"
#include "ErrorHandler.h"

UKFont::~UKFont()
{
	Reset();
}
void UKFont::Reset()
{
	// SRV가 AtlasTexture를 참조하므로 View를 먼저 해제하고 Texture를 해제한다.
	AtlasSRV.Reset();
	AtlasTexture.Reset();

	if (Face != nullptr)
	{
		FT_Done_Face(Face);
		Face = nullptr;
	}

	if (Library != nullptr)
	{
		FT_Done_FreeType(Library);
		Library = nullptr;
	}

	CodePointToGlyphIndex.clear();
	GlyphCache.clear();

	AtlasPixels.clear();

	FontMetrics = {};

	AtlasWidth = 0;
	AtlasHeight = 0;

	NextAtlasX = 0;
	NextAtlasY = 0;
	CurrentRowHeight = 0;

	bAtlasDirty = false;
	bInitialized = false;
}
void UKFont::Initialize(ID3D11Device* Device, const std::filesystem::path& MetaDataPath)
{
	//UAsset Initialize
	UFont::Initialize(Device, MetaDataPath);
	bInitialized = false;

	//이전 상태 초기화
	Reset();
	if (Device == nullptr)
	{
		return;
	}
	// MetaData 읽기
	FAssetMetadataParser Parser;
	if (!Parser.Load(MetaDataPath))
	{
		ErrorHandler::Report("[UKFont]", "Falied to load font metadata",ErrorHandler::EErrorLevel::Error);
		return;
	}
	// MetaData에서 폰트 파일과 설정 가져오기
	const std::filesystem::path FontPath = Parser.ResolvePath("FilePath");
	const uint32_t BakePixelHeight = Parser.GetOr("BakePixelHeight", uint32_t{32}); // 없으면 32
	AtlasWidth = Parser.GetOr("AtlasWidth", uint32_t{ 4096 });
	AtlasHeight = Parser.GetOr("AtlasHeight", uint32_t{ 4096 });
	// 검증
	if (Device == nullptr || FontPath.empty() || BakePixelHeight == 0 || AtlasHeight == 0 || AtlasWidth == 0)
	{
		return;
	}
	FT_Error Error = FT_Init_FreeType(&Library); // FreeType 라이브러리 시작
	if (Error != FT_Err_Ok)
	{
		Reset();
		return;
	}
	Error = FT_New_Face(Library, FontPath.string().c_str(), 0, &Face); // TTF 파일에서 Face 생성
	if (Error != FT_Err_Ok)
	{
		Reset();
		return;
	}
	Error = FT_Select_Charmap(Face, FT_ENCODING_UNICODE); // Unicode CodePoint를 사용할 charmap 선택
	if (Error != FT_Err_Ok)
	{
		Reset();
		return;
	}
	Error = FT_Set_Pixel_Sizes(Face, 0, BakePixelHeight); // Glyph를 생성할 픽셀 크기 설정
	// 폰트 전체 공통 Metric 저장
	FontMetrics.BakePixelHeight = static_cast<float>(BakePixelHeight);
	FontMetrics.Ascender = static_cast<float>(Face->size->metrics.ascender) / 64.0f;
	FontMetrics.Descender = static_cast<float>(Face->size->metrics.descender) / 64.0f;
	FontMetrics.LineHeight = static_cast<float>(Face->size->metrics.height) / 64.0f;
	// 빈 CPU Atlas 생성
	const size_t AtlasPixelCount = static_cast<size_t>(AtlasWidth) * static_cast<size_t>(AtlasHeight);
	AtlasPixels.assign(AtlasPixelCount, uint8_t{ 0 });
	if (!CreateAtlasTexture(Device))
	{
		Reset();
		return;
	}
	bAtlasDirty = false;
	bInitialized = true;
}
bool UKFont::CreateAtlasTexture(ID3D11Device* Device)
{
	if (Device == nullptr || AtlasPixels.empty())
	{
		return false;
	}

	AtlasSRV.Reset();
	AtlasTexture.Reset();

	D3D11_TEXTURE2D_DESC TextureDesc = {};
	TextureDesc.Width = AtlasWidth;
	TextureDesc.Height = AtlasHeight;
	TextureDesc.MipLevels = 1;
	TextureDesc.ArraySize = 1;
	TextureDesc.Format = DXGI_FORMAT_R8_UNORM;
	TextureDesc.SampleDesc.Count = 1;
	TextureDesc.Usage = D3D11_USAGE_DEFAULT;
	TextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	TextureDesc.CPUAccessFlags = 0;
	TextureDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA InitialData = {};
	InitialData.pSysMem = AtlasPixels.data();
	InitialData.SysMemPitch = AtlasWidth * sizeof(uint8_t);
	InitialData.SysMemSlicePitch = 0;

	HRESULT Result = Device->CreateTexture2D(&TextureDesc,&InitialData,AtlasTexture.GetAddressOf());

	if (FAILED(Result))
	{
		return false;
	}

	Result = Device->CreateShaderResourceView(AtlasTexture.Get(),nullptr,AtlasSRV.GetAddressOf());

	if (FAILED(Result))
	{
		AtlasTexture.Reset();
		return false;
	}

	return true;
}
void UKFont::FlushAtlas(ID3D11DeviceContext* Context)
{
	if (!bInitialized || !bAtlasDirty || Context == nullptr || AtlasTexture == nullptr || AtlasPixels.empty())
	{
		return;
	}

	Context->UpdateSubresource(
		AtlasTexture.Get(),  // 갱신할 GPU Texture
		0,					// 첫번째 mip level
		nullptr,			// Texture 전체 갱신
		AtlasPixels.data(),	// CPU 픽셀 시작 주소
		AtlasWidth * sizeof(uint8_t), // CPU 한 행의 바이트 수
		0);                 // 2D Texture 이므로 사용 X

	bAtlasDirty = false;
}
const FKGlyph* UKFont::FindGlyph(char32_t CodePoint) const
{
	auto CodePointIt = CodePointToGlyphIndex.find(CodePoint);
	if (CodePointIt == CodePointToGlyphIndex.end())
	{
		return nullptr;
	}
	auto GlyphCacheIt = GlyphCache.find(CodePointIt->second);
	if (GlyphCacheIt == GlyphCache.end())
	{
		return nullptr;
	}
	return &GlyphCacheIt->second;
}
const FKGlyph* UKFont::GetOrCreateGlyph(char32_t CodePoint)
{
	if (!bInitialized || Face == nullptr)
	{
		return nullptr;
	}
	// FindGlyph 결과 Glyph가 존재한다면 해당 Glyph 반환
	if (const FKGlyph* CachedGlyph = FindGlyph(CodePoint))
	{
		return CachedGlyph;
	}
	// 없다면 우선 첫번째로 GlyphIndex 매핑은 됐으나 GlyphCache에 등록되지 않은 경우를 찾아본다.
	// 즉, 다른 CodePoint로 같은 GlyphIndex가 나온 것이다. FT_Face에서 같은 모양을 가리킨다고 판단한것이다.
	uint32_t GlyphIndex = 0;
	const auto CodePointIt = CodePointToGlyphIndex.find(CodePoint);
	if (CodePointIt != CodePointToGlyphIndex.end())
	{
		// 해당 CodePoint와 GlyphIndex를 매핑한다.
		GlyphIndex = CodePointIt->second;
	}
	else
	{
		// 두번째원인은, 처음으로 캐싱하는 Glyph인 경우이다. 해당 폰트에서 GlyphIndex를 받는다.
		GlyphIndex = static_cast<uint32_t>(FT_Get_Char_Index(Face, static_cast<FT_ULong>(CodePoint)));
		// GlyphIndex가 0인 경우, 해당 글씨가 폰트에 존재하지 않는것이다.
		if (GlyphIndex == 0)
		{
			return nullptr;
		}
	}
	// 다른 CodePoint로 같은 GlyphIndex가 나온경우 해당 CodePoint에 매핑되는 Glyph가 존재하므로
	const auto GlyphCacheIt = GlyphCache.find(GlyphIndex);
	if (GlyphCacheIt != GlyphCache.end())
	{
		// 해당 관계(CodePoint->GlyphIndex)를 등록하고
		CodePointToGlyphIndex.insert_or_assign(CodePoint, GlyphIndex);
		// 캐시의 Glyph를 사용한다.
		return &GlyphCacheIt->second;
	}
	// 캐시에 glyph가 없다면 Create 한다.
	// Glyph outline과 metric 로드
	// TTF 폰트는 글자의 윤곽선을 점과 곡선으로 저장한다. -> outline -> glyph를 구성하는 정보가 들어있다.
	// Metric은 glyph를 어디에 놓고 다음 glyph로 얼마나 이동할지를 알려주는 숫자이다.
	FT_Error Error = FT_Load_Glyph(Face, static_cast<FT_UInt>(GlyphIndex), FT_LOAD_DEFAULT | FT_LOAD_NO_BITMAP);
	if (Error != FT_Err_Ok)
	{
		return nullptr;
	}
	// Glyph를 outline에서 Bitmap으로 전환
	Error = FT_Render_Glyph(Face->glyph, FT_RENDER_MODE_NORMAL);
	// FT_Err_Ok -> 성공인 경우 0
	if (Error != FT_Err_Ok)
	{
		return nullptr;
	}

	FT_GlyphSlot Slot = Face->glyph;
	FT_Bitmap& Bitmap = Slot->bitmap;

	// FKGlyph 구조체에 배치 정보 복사
	FKGlyph NewGlyph{};
	NewGlyph.GlyphIndex = GlyphIndex;
	NewGlyph.BitmapWidth = static_cast<uint32_t>(Bitmap.width);
	NewGlyph.BitmapHeight = static_cast<uint32_t>(Bitmap.rows);
	NewGlyph.BearingX = static_cast<int32_t>(Slot->bitmap_left);
	NewGlyph.BearingY = static_cast<int32_t>(Slot->bitmap_top);
	// FreeType의 Advance는 26.6(정수 26bit / 소숫점 6bit) 고정소수점을 사용하므로 실제값을 얻으려면 64로 나눠야함.
	// FreeType은 소숫점이 포함된 픽셀 정밀도를 유지하기 위해 이를 사용한다.
	NewGlyph.AdvanceX = static_cast<float>(Slot->advance.x) / 64.0f;
	NewGlyph.AdvanceY = static_cast<float>(Slot->advance.y) / 64.0f;

	// Atlas 공간 할당
	if (Bitmap.width > 0 && Bitmap.rows > 0)
	{
		uint32_t AtlasX = 0;
		uint32_t AtlasY = 0;
		// AtlasX, Y를 업데이트하고 Bitmap을 AtlasPixels에 복사한다.
		if (!AllocateAtlasRect(static_cast<uint32_t>(Bitmap.width), static_cast<uint32_t>(Bitmap.rows), AtlasX, AtlasY))
		{
			return nullptr;
		}
		if (!CopyBitmapToAtlas(Bitmap, AtlasX, AtlasY))
		{
			return nullptr;
		}
		// 확정된 AtlasX, Y를 바탕으로 나머지 정보들을 업데이트한다.
		NewGlyph.AtlasX = AtlasX;
		NewGlyph.AtlasY = AtlasY;
		NewGlyph.UVMin = FVector2(static_cast<float>(AtlasX) / static_cast<float>(AtlasWidth), 
			static_cast<float>(AtlasY) / static_cast<float>(AtlasHeight));
		NewGlyph.UVMax = FVector2(static_cast<float>(AtlasX + Bitmap.width) / static_cast<float>(AtlasWidth),
			static_cast<float>(AtlasY + Bitmap.rows) / static_cast<float>(AtlasHeight));
		bAtlasDirty = true;
	}
	NewGlyph.bValid = true;
	// 캐시에 저장
	// emplace는 (Iterator, bool) 로 반환함, move -> Glyph를 복사하기보단 이동
	auto [InsertedIt, bInserted] = GlyphCache.emplace(GlyphIndex, std::move(NewGlyph));
	if (!bInserted)
	{
		return nullptr;
	}
	// CodePoint 매핑에도 저장
	CodePointToGlyphIndex.insert_or_assign(CodePoint, GlyphIndex);
	// 캐시 내부의 glyph 주소 반환
	return &InsertedIt->second;
}
const FKFontMetrics& UKFont::GetFontMetrics() const
{
	return FontMetrics;
}
// 현재 NextAtlasX,Y에서 해당 BitmapWidth/Height 를 할당할 수 있는지 확인하고 비트맵에 배치될 좌상단 좌표를 OutAtlasX,Y에 업데이트.
// 이후 NextAtlasX,Y를 업데이트
bool UKFont::AllocateAtlasRect(uint32_t BitmapWidth, uint32_t BitmapHeight, uint32_t& OutAtlasX, uint32_t& OutAtlasY)
{
	if (BitmapWidth <= 0 || BitmapHeight <= 0)
	{
		return false;
	}
	uint32_t ReqWidth = BitmapWidth + AtlasPadding * 2;
	uint32_t ReqHeight = BitmapHeight + AtlasPadding * 2;
	uint32_t TempX = NextAtlasX;
	uint32_t TempY = NextAtlasY;
	if (TempX + ReqWidth > AtlasWidth)
	{
		TempX = 0;
		TempY += CurrentRowHeight;
		CurrentRowHeight = ReqHeight;
	}
	if (TempY > AtlasHeight || TempY + ReqHeight > AtlasHeight || TempY + CurrentRowHeight > AtlasHeight)
	{
		return false;
	}
	OutAtlasX = TempX + AtlasPadding;
	OutAtlasY = TempY + AtlasPadding;
	CurrentRowHeight = std::max(CurrentRowHeight, ReqHeight);
	NextAtlasX = TempX + ReqWidth;
	NextAtlasY = TempY;
	return true;
}
bool UKFont::CopyBitmapToAtlas(FT_Bitmap& Bitmap, uint32_t AtlasX, uint32_t AtlasY)
{
	if (Bitmap.width == 0 || Bitmap.rows == 0)
	{
		// 공백의 bitmap
		return true;
	}
	if (Bitmap.buffer == nullptr)
	{
		return false;
	}
	if (Bitmap.width + AtlasX > AtlasWidth || Bitmap.rows + AtlasY > AtlasHeight)
	{
		return false;
	}
	// Pitch = 다음 행까지의 거리 | 실제 픽셀의 길이를 나타내는 width와 다르다. 
	// width는 bitmap에서 최대 가로 길이 이기도 하므로 Pitch는 Padding에 대비함이다.
	const uint32_t SourcePitch = static_cast<uint32_t>(Bitmap.pitch >= 0 ? Bitmap.pitch : -Bitmap.pitch);
	if (SourcePitch < Bitmap.width)
	{
		return false;
	}
	for (uint32_t row = 0; row < Bitmap.rows; row++)
	{
		const uint8_t* SourceRow = nullptr;
		/* 일반적인 경우 */
		// buffer의 첫번째 행은 bitmap의 첫번째 행
		if (Bitmap.pitch >= 0)
		{
			SourceRow = Bitmap.buffer + static_cast<size_t>(row) * SourcePitch;
		}
		/* 방향이 반대인 경우 */
		// buffer의 첫번째 행은 bitmap의 아래쪽 행
		else
		{
			SourceRow = Bitmap.buffer + static_cast<size_t>(Bitmap.rows - 1 - row) * SourcePitch;
		}
		// AtlasPixels[0] + (Atlas시작 Y픽셀 위치 + 탐색중인 Bitmap row) * Atlas너비 + Atlas시작 X픽셀 위치 = AtlasPixels 기록 시작 위치
		uint8_t* DestinationRow = AtlasPixels.data() + static_cast<size_t>(AtlasY + row) * AtlasWidth + AtlasX;
		// SourceRow 부터 Bitmap.width 만큼의 정보를 DestinationRow에 복사
		std::memcpy(DestinationRow, SourceRow, Bitmap.width);
	}
	// Atlas 정보 업데이트됨
	bAtlasDirty = true;
	return true;
}
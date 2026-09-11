#include "PCH.h"
#include "FAssetMetadataParser.h"

#include <rapidjson/error/en.h>
#include <rapidjson/istreamwrapper.h>

#include <fstream>
#include <format>

namespace {
    bool TryGetNumberMember(const rapidjson::Value& Object, const char* UpperName, const char* LowerName, float& OutValue) {
        if (!Object.IsObject()) {
            return false;
        }

        auto It = Object.FindMember(UpperName);

        if (It == Object.MemberEnd()) {
            It = Object.FindMember(LowerName);
        }

        if (It == Object.MemberEnd() || !It->value.IsNumber()) {
            return false;
        }

        OutValue = It->value.GetFloat();
        return true;
    }

    bool TryReadVector2(const rapidjson::Value& Value, float& X, float& Y) {
        if (Value.IsArray()) {
            if (Value.Size() != 2 || !Value[0].IsNumber() || !Value[1].IsNumber()) {
                return false;
            }

            X = Value[0].GetFloat();
            Y = Value[1].GetFloat();

            return true;
        }

        if (Value.IsObject()) {
            return TryGetNumberMember(Value, "X", "x", X) && TryGetNumberMember(Value, "Y", "y", Y);
        }

        return false;
    }

    bool TryReadVector3(const rapidjson::Value& Value, float& X, float& Y, float& Z) {
        if (Value.IsArray()) {
            if (Value.Size() != 3 || !Value[0].IsNumber() || !Value[1].IsNumber() || !Value[2].IsNumber()) {
                return false;
            }

            X = Value[0].GetFloat();
            Y = Value[1].GetFloat();
            Z = Value[2].GetFloat();

            return true;
        }

        if (Value.IsObject()) {
            return TryGetNumberMember(Value, "X", "x", X) && TryGetNumberMember(Value, "Y", "y", Y) && TryGetNumberMember(Value, "Z", "z", Z);
        }

        return false;
    }

    bool TryReadVector4(const rapidjson::Value& Value, float& X, float& Y, float& Z, float& W) {
        if (Value.IsArray()) {
            if (Value.Size() != 4 || !Value[0].IsNumber() || !Value[1].IsNumber() || !Value[2].IsNumber() || !Value[3].IsNumber()) {
                return false;
            }

            X = Value[0].GetFloat();
            Y = Value[1].GetFloat();
            Z = Value[2].GetFloat();
            W = Value[3].GetFloat();

            return true;
        }

        if (Value.IsObject()) {
            return TryGetNumberMember(Value, "X", "x", X) && TryGetNumberMember(Value, "Y", "y", Y) && TryGetNumberMember(Value, "Z", "z", Z) && TryGetNumberMember(Value, "W", "w", W);
        }

        return false;
    }

    bool TryReadColor(const rapidjson::Value& Value, float& R, float& G, float& B, float& A) {
        if (Value.IsArray()) {
            if (Value.Size() != 4 || !Value[0].IsNumber() || !Value[1].IsNumber() || !Value[2].IsNumber() || !Value[3].IsNumber()) {
                return false;
            }

            R = Value[0].GetFloat();
            G = Value[1].GetFloat();
            B = Value[2].GetFloat();
            A = Value[3].GetFloat();

            return true;
        }

        if (Value.IsObject()) {
            return TryGetNumberMember(Value, "R", "r", R) && TryGetNumberMember(Value, "G", "g", G) && TryGetNumberMember(Value, "B", "b", B) && TryGetNumberMember(Value, "A", "a", A);
        }

        return false;
    }
}

bool FAssetMetadataParser::Load(const std::filesystem::path& Path) {
    Clear();

    if (Path.empty()) {
        LastError = "Metadata path is empty.";
        return false;
    }

    MetadataPath = Path.lexically_normal();

    std::ifstream File(MetadataPath, std::ios::binary);

    if (!File.is_open()) {
        LastError = std::format("Failed to open metadata file: {}", MetadataPath.string());
        return false;
    }

    rapidjson::IStreamWrapper Stream(File);

    Document.ParseStream<
        rapidjson::kParseCommentsFlag |
        rapidjson::kParseTrailingCommasFlag
    >(Stream);

    if (Document.HasParseError()) {
        LastError = std::format(
            "Failed to parse metadata file: {} - {} at offset {}",
            MetadataPath.string(),
            rapidjson::GetParseError_En(Document.GetParseError()),
            Document.GetErrorOffset()
        );

        return false;
    }

    if (!Document.IsObject()) {
        LastError = std::format("Metadata root must be an object: {}", MetadataPath.string());
        return false;
    }

    bValid = true;
    return true;
}

void FAssetMetadataParser::Clear() {
    Document.SetObject();

    MetadataPath.clear();
    LastError.clear();

    bValid = false;
}

const rapidjson::Value* FAssetMetadataParser::FindValue(std::string_view Path) const {
    if (!bValid || Path.empty()) {
        return nullptr;
    }

    const rapidjson::Value* Current = &Document;
    size_t Begin = 0;

    while (Begin < Path.size()) {
        const size_t End = std::min(Path.find('.', Begin), Path.size());
        const size_t Length = End - Begin;

        if (Length == 0 || !Current->IsObject()) {
            return nullptr;
        }

        const auto It = Current->FindMember(rapidjson::StringRef(Path.data() + Begin, static_cast<rapidjson::SizeType>(Length)));

        if (It == Current->MemberEnd()) {
            return nullptr;
        }

        Current = &It->value;

        if (End == std::string_view::npos) {
            break;
        }

        Begin = End + 1;
    }

    return Current;
}

bool FAssetMetadataParser::TryResolvePath(std::string_view Path, std::filesystem::path& OutPath) const {
    std::filesystem::path ParsedPath{};

    if (!TryGet(Path, ParsedPath)) {
        return false;
    }

    if (ParsedPath.empty()) {
        return false;
    }

    if (ParsedPath.is_absolute()) {
        OutPath = ParsedPath.lexically_normal();
        return true;
    }

    OutPath = (MetadataPath.parent_path().parent_path() / ParsedPath).lexically_normal();

    return true;
}

bool FAssetMetadataParser::ConvertValue(const rapidjson::Value& Value, FVector2D& OutValue) {
    float X;
    float Y;

    if (!TryReadVector2(Value, X, Y)) {
        return false;
    }

    OutValue = FVector2D{ X, Y };
    return true;
}

bool FAssetMetadataParser::ConvertValue(const rapidjson::Value& Value, FVector3& OutValue) {
    float X;
    float Y;
    float Z;

    if (!TryReadVector3(Value, X, Y, Z)) {
        return false;
    }

    OutValue = FVector3{ X, Y, Z };
    return true;
}

bool FAssetMetadataParser::ConvertValue(const rapidjson::Value& Value, FVector4& OutValue) {
    float X;
    float Y;
    float Z;
    float W;

    if (!TryReadVector4(Value, X, Y, Z, W)) {
        return false;
    }

    OutValue = FVector4{ X, Y, Z, W };
    return true;
}
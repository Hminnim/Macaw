#pragma once

/*
================================================================================
 FAssetMetadataParser
================================================================================

 Asset metadata JSON parsing helper.

 ------------------------------------------------------------------------------
 Example Directory Structure
 ------------------------------------------------------------------------------

 Content/
 |
 +-- Meshes/
 |   |
 |   +-- Cube.obj
 |
 +-- Textures/
 |   |
 |   +-- Cube_D.png
 |   +-- Cube_N.png
 |
 +-- Metadata/
     |
     +-- Cube.json       <-- Metadata file


 ------------------------------------------------------------------------------
 Example Metadata
 ------------------------------------------------------------------------------

 Cube.json

 {
     "Source": "../Meshes/Cube.obj",

     "Import": {
         "Scale": [1.0, 1.0, 1.0],
         "GenerateNormals": true,
         "GenerateTangents": true
     },

     "Textures": {
         "BaseColor": "../Textures/Cube_D.png",
         "Normal": "../Textures/Cube_N.png"
     },

     "Material": {
         "BaseColor": [1.0, 0.5, 0.25, 1.0],
         "Roughness": 0.5,
         "Metallic": 0.0
     },

     "Tags": [
         "StaticMesh",
         "Environment",
         "Opaque"
     ]
 }


 ------------------------------------------------------------------------------
 Property Path Resolution
 ------------------------------------------------------------------------------

 JSON:

     "Import": {
         "Scale": [1.0, 1.0, 1.0]
     }

 Query:

     "Import.Scale"

 Resolution:

     Document
        |
        +-- Import
              |
              +-- Scale
                    |
                    +-- [1.0, 1.0, 1.0]


 Usage:

     FVector3 Scale =
         Metadata.GetOr("Import.Scale", FVector3{ 1.0f, 1.0f, 1.0f });


 ------------------------------------------------------------------------------
 File Path Resolution
 ------------------------------------------------------------------------------

 Relative paths are resolved from the directory containing the metadata file.

 Metadata:

     "Source": "../Meshes/Cube.obj"

 Metadata File:

     Content/Metadata/Cube.json

 Resolution:

     Content/Metadata/
             +
     ../Meshes/Cube.obj
             |
             v
     Content/Metadata/../Meshes/Cube.obj
             |
             | lexically_normal()
             v
     Content/Meshes/Cube.obj


 Code:

     std::filesystem::path Source =
         Metadata.ResolvePath("Source");


 Absolute paths are preserved:

     "Source": "D:/Assets/Cube.obj"

                         |
                         v

                  D:/Assets/Cube.obj


 ------------------------------------------------------------------------------
 Supported Value Formats
 ------------------------------------------------------------------------------

 Scalar:

     "Enabled": true
     "Count": 32
     "Scale": 1.0
     "Name": "Cube"


 FVector2D:

     "UVScale": [1.0, 1.0]

 or

     "UVScale": {
         "X": 1.0,
         "Y": 1.0
     }


 FVector3:

     "Scale": [1.0, 2.0, 3.0]

              |
              v

     FVector3{ 1.0f, 2.0f, 3.0f }


 or

     "Scale": {
         "X": 1.0,
         "Y": 2.0,
         "Z": 3.0
     }


 FVector4:

     "Value": [1.0, 2.0, 3.0, 4.0]


 FLinearColor:

     "BaseColor": [1.0, 0.5, 0.25, 1.0]

                      R     G     B     A
                      |     |     |     |
                      v     v     v     v

     FLinearColor{ 1.0f, 0.5f, 0.25f, 1.0f }


 or

     "BaseColor": {
         "R": 1.0,
         "G": 0.5,
         "B": 0.25,
         "A": 1.0
     }


 Arrays:

     "Tags": [
         "StaticMesh",
         "Environment",
         "Opaque"
     ]

     TArray<FString> Tags;
     Metadata.TryGetArray("Tags", Tags);


 ------------------------------------------------------------------------------
 Typical Usage
 ------------------------------------------------------------------------------

 FAssetMetadataParser Metadata;

 if (!Metadata.Load(MetadataPath)) {
     return;
 }

 const std::filesystem::path SourcePath = Metadata.ResolvePath("Source");

 const FVector3 Scale = Metadata.GetOr("Import.Scale", FVector3{ 1.0f, 1.0f, 1.0f });

 const bool bGenerateNormals = Metadata.GetOr("Import.GenerateNormals", true);

 const FVector4 BaseColor = Metadata.GetOr( "Material.BaseColor", FVector4{ 1.0f, 1.0f, 1.0f, 1.0f });


 ------------------------------------------------------------------------------
 Notes
 ------------------------------------------------------------------------------

    - JSON 주석을 사용할 수 있습니다.
    - 마지막 항목 뒤에 쉼표를 사용할 수 있습니다.
    - 상대 경로는 메타데이터 파일이 위치한 디렉터리를 기준으로 해석합니다.
    - 경로 정규화에는 lexically_normal()을 사용합니다.
    - 경로를 해석할 때 대상 파일이 실제로 존재할 필요는 없습니다.
    - 값이 없거나 타입이 일치하지 않으면 TryGet()은 false를 반환합니다.
    - 파싱에 실패하면 GetOr()는 전달받은 기본값을 반환합니다.

================================================================================
*/

#include <rapidjson/document.h>

#include <filesystem>
#include <functional>
#include <string_view>
#include <type_traits>
#include <utility>

template<typename T>
concept PushBackContainer = requires(T & Values, typename T::value_type Value) {
    Values.clear();
    Values.push_back(std::move(Value));
};


class FAssetMetadataParser {
public:
    FAssetMetadataParser() = default;
    ~FAssetMetadataParser() = default;

    FAssetMetadataParser(const FAssetMetadataParser&) = delete;
    FAssetMetadataParser& operator=(const FAssetMetadataParser&) = delete;

    FAssetMetadataParser(FAssetMetadataParser&&) noexcept = default;
    FAssetMetadataParser& operator=(FAssetMetadataParser&&) noexcept = default;

public:
    bool Load(const std::filesystem::path& Path);
    void Clear();

    bool IsValid() const {
        return bValid;
    }

    bool Contains(std::string_view Path) const {
        return FindValue(Path) != nullptr;
    }

    const std::filesystem::path& GetPath() const {
        return MetadataPath;
    }

    const FString& GetLastError() const {
        return LastError;
    }

    const rapidjson::Document& GetDocument() const {
        return Document;
    }

    const rapidjson::Value* FindValue(std::string_view Path) const;

    bool TryResolvePath(std::string_view Path, std::filesystem::path& OutPath) const;

    std::filesystem::path ResolvePath(std::string_view Path) const {
        std::filesystem::path Result{};

        if (!TryResolvePath(Path, Result)) {
            return {};
        }

        return Result;
    }

    template<typename T>
    bool TryGet(std::string_view Path, T& OutValue) const {
        const rapidjson::Value* Value = FindValue(Path);

        if (Value == nullptr) {
            return false;
        }

        return ConvertValue(*Value, OutValue);
    }

    template<typename T>
    T GetOr(std::string_view Path, T DefaultValue) const {
        T Value{};

        if (!TryGet(Path, Value)) {
            return DefaultValue;
        }

        return Value;
    }

    template<typename Container>
    bool TryGetArray(std::string_view Path, Container& OutValues) const 
        requires requires(Container& Values, typename Container::value_type Value) { Values.clear(); Values.push_back(std::move(Value)); } {
        const rapidjson::Value* Value = FindValue(Path);

        if (Value == nullptr || !Value->IsArray()) {
            return false;
        }

        Container Result{};

        if constexpr (requires(Container & Values, size_t Count) { Values.reserve(Count); }) {
            Result.reserve(Value->Size());
        }

        for (const rapidjson::Value& Element : Value->GetArray()) {
            typename Container::value_type Converted{};

            if (!ConvertValue(Element, Converted)) {
                return false;
            }

            Result.push_back(std::move(Converted));
        }

        OutValues = std::move(Result);
        return true;
    }

    template<typename Func>
    bool ForEachArray(std::string_view Path, Func&& Function) const {
        const rapidjson::Value* Value = FindValue(Path);

        if (Value == nullptr || !Value->IsArray()) {
            return false;
        }

        for (const rapidjson::Value& Element : Value->GetArray()) {
            std::invoke(std::forward<Func>(Function), Element);
        }

        return true;
    }

private:
    static bool ConvertValue(const rapidjson::Value& Value, FVector2D& OutValue);
    static bool ConvertValue(const rapidjson::Value& Value, FVector3& OutValue);
    static bool ConvertValue(const rapidjson::Value& Value, FVector4& OutValue);

    template<typename T>
    static bool ConvertValue(const rapidjson::Value& Value, T& OutValue) {
        if constexpr (std::is_same_v<T, bool>) {
            if (!Value.IsBool()) {
                return false;
            }

            OutValue = Value.GetBool();
            return true;
        }
        else if constexpr (std::is_same_v<T, FString>) {
            if (!Value.IsString()) {
                return false;
            }

            OutValue = FString(Value.GetString(), Value.GetStringLength());
            return true;
        }
        else if constexpr (std::is_same_v<T, std::filesystem::path>) {
            if (!Value.IsString()) {
                return false;
            }

            const std::u8string UTF8Path(
                reinterpret_cast<const char8_t*>(Value.GetString()),
                Value.GetStringLength()
            );

            OutValue = std::filesystem::path(UTF8Path);
            return true;
        }
        else if constexpr (std::is_floating_point_v<T>) {
            if (!Value.IsNumber()) {
                return false;
            }

            OutValue = static_cast<T>(Value.GetDouble());
            return true;
        }
        else if constexpr (std::is_integral_v<T> && std::is_signed_v<T>) {
            if (!Value.IsInt64()) {
                return false;
            }

            const int64 RawValue = Value.GetInt64();

            if (!std::in_range<T>(RawValue)) {
                return false;
            }

            OutValue = static_cast<T>(RawValue);
            return true;
        }
        else if constexpr (std::is_integral_v<T> && std::is_unsigned_v<T>) {
            if (!Value.IsUint64()) {
                return false;
            }

            const uint64 RawValue = Value.GetUint64();

            if (!std::in_range<T>(RawValue)) {
                return false;
            }

            OutValue = static_cast<T>(RawValue);
            return true;
        }
        else if constexpr (std::is_enum_v<T>) {
            using UnderlyingType = std::underlying_type_t<T>;

            UnderlyingType RawValue{};

            if (!ConvertValue(Value, RawValue)) {
                return false;
            }

            OutValue = static_cast<T>(RawValue);
            return true;
        }
        else {
            static_assert(TDependentFalse<T>, "Unsupported metadata value type.");
        }
    }

    template<typename>
    inline static constexpr bool TDependentFalse = false;

private:
    rapidjson::Document Document{};

    std::filesystem::path MetadataPath{};
    FString LastError{};

    bool bValid{ false };
};
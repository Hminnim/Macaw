#pragma once
#include "../Core/Base/FGuid.h"

enum class EArchiveMode : uint8
{
    Loading,
    Saving,
    Hashing,
    Counting 
};

class FAssetRegistry;

class FArchive
{
public:
    explicit FArchive(EArchiveMode InMode);

    virtual ~FArchive() = default;

    inline bool IsLoading() const { return Mode == EArchiveMode::Loading; }
    inline bool IsSaving() const { return Mode == EArchiveMode::Saving; }
    inline bool IsHashing() const { return Mode == EArchiveMode::Hashing; }
    inline bool IsCounting() const { return Mode == EArchiveMode::Counting; }

    // ---------------------------------------------------
    // 1. 기본 원시 타입 (Primitives)
    // ---------------------------------------------------
    virtual void Serialize(std::string_view Name, bool& Value) = 0;
    virtual void Serialize(std::string_view Name, uint8& Value) = 0; 
    virtual void Serialize(std::string_view Name, int32& Value) = 0;
    virtual void Serialize(std::string_view Name, uint32& Value) = 0;
    virtual void Serialize(std::string_view Name, int64& Value) = 0;
    virtual void Serialize(std::string_view Name, uint64& Value) = 0;
    virtual void Serialize(std::string_view Name, float32& Value) = 0;
    virtual void Serialize(std::string_view Name, float64& Value) = 0;

    // ---------------------------------------------------
    // 2. 수학 코어 타입 (DirectX SimpleMath)
    // ---------------------------------------------------
    virtual void Serialize(std::string_view Name, FVector2& Value) = 0;
    virtual void Serialize(std::string_view Name, FVector& Value) = 0;
    virtual void Serialize(std::string_view Name, FVector4& Value) = 0;
    virtual void Serialize(std::string_view Name, FQuat& Value) = 0;
    virtual void Serialize(std::string_view Name, FMatrix& Value) = 0;

    // ---------------------------------------------------
    // 3. 엔진 코어 타입
    // ---------------------------------------------------
    virtual void Serialize(std::string_view Name, FString& Value) = 0;
    virtual void Serialize(std::string_view Name, FGuid& Value) = 0;
    template<typename T>
    void Serialize(std::string_view Name, TArray<T>& ArrayValue)
    {
        size_t Size = ArrayValue.size();

        BeginArrayScope(Name, Size);

        if (IsLoading())
        {
            ArrayValue.resize(Size);
        }

        for (size_t i = 0; i < Size; ++i)
        {
            // 인덱스를 Name으로 변환하여 순회
            Serialize(std::to_string(i), ArrayValue[i]);
        }

        EndArrayScope();
    }

    // ---------------------------------------------------
    // 4. 구조체 / 계층 처리 (Scope)
    // ---------------------------------------------------
    virtual void BeginObjectScope(std::string_view Name) = 0;
    virtual void EndObjectScope() = 0;

    // ---------------------------------------------------
    // 5. 배열 처리 (Scope)
    // ---------------------------------------------------
    virtual void BeginArrayScope(std::string_view Name, size_t& ArraySize) = 0;
    virtual void EndArrayScope() = 0;

    // ---------------------------------------------------
    // 6. 구조체 직렬화 헬퍼 (UObject 자식들이나 커스텀 구조체)
    // ---------------------------------------------------
    template<typename T>
    void SerializeStruct(std::string_view Name, T& StructValue)
    {
        BeginObjectScope(Name);
        StructValue.Serialize(*this);
        EndObjectScope();
    }

    // ---------------------------------------------------
    // 7. Asset Registry 처리
    // ---------------------------------------------------
    void SetAssetRegistry(FAssetRegistry* InputAssetRegistry);


    FAssetRegistry* GetAssetRegistry();


protected:
    const EArchiveMode Mode;

private:
    FAssetRegistry* AssetRegistry;
};
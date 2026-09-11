#pragma once
#include "FArchive.h"
#include <rapidjson/document.h>

class FArchiveJson : public FArchive
{
private:
    rapidjson::Document::AllocatorType* Allocator;
    TArray<rapidjson::Value*> NodeStack;

    // called when saving in progress
    inline void AddChildNode(rapidjson::Value* Parent, std::string_view Name, rapidjson::Value& Child)
    {
        if (Parent->IsObject())
        {
            rapidjson::Value JsonKey(Name.data(), static_cast<uint32>(Name.size()), *Allocator);
            Parent->AddMember(JsonKey, Child, *Allocator);
        }
        else if (Parent->IsArray())
        {
            Parent->PushBack(Child, *Allocator);
        }
    }
    // called when loading in progress
    inline rapidjson::Value* GetChildNode(rapidjson::Value* Parent, std::string_view Name)
    {
        if (Parent->IsObject())
        {
            std::string KeyStr(Name);
            if (Parent->HasMember(KeyStr.c_str()))
            {
                return &(*Parent)[KeyStr.c_str()];
            }
        }
        else if (Parent->IsArray())
        {
            size_t Index = std::stoull(std::string(Name));
            if (Index < Parent->Size())
            {
                return &(*Parent)[static_cast<rapidjson::SizeType>(Index)];
            }
        }
        return nullptr;
    }

public:
    FArchiveJson(rapidjson::Value& RootNode, rapidjson::Document::AllocatorType& InAllocator);
    FArchiveJson(rapidjson::Value& RootNode);

    // Primitives
    virtual void Serialize(std::string_view Name, bool& Value) override;
    virtual void Serialize(std::string_view Name, uint8& Value) override;
    virtual void Serialize(std::string_view Name, int32& Value) override;
    virtual void Serialize(std::string_view Name, uint32& Value) override;
    virtual void Serialize(std::string_view Name, int64& Value) override;
    virtual void Serialize(std::string_view Name, uint64& Value) override;
    virtual void Serialize(std::string_view Name, float32& Value) override;
    virtual void Serialize(std::string_view Name, float64& Value) override;

    // Engine Core
    virtual void Serialize(std::string_view Name, FString& Value) override;
    virtual void Serialize(std::string_view Name, FGuid& Value) override;

    // Math Types
    virtual void Serialize(std::string_view Name, FVector2D& Value) override;
    virtual void Serialize(std::string_view Name, FVector3& Value) override;
    virtual void Serialize(std::string_view Name, FVector4& Value) override;
    virtual void Serialize(std::string_view Name, FQuat& Value) override;
    virtual void Serialize(std::string_view Name, FMatrix& Value) override;

    // Scope
    virtual void BeginObjectScope(std::string_view Name) override;
    virtual void EndObjectScope() override
    {
        if (!NodeStack.empty()) 
            NodeStack.pop_back();
    }

    // Array
    virtual void BeginArrayScope(std::string_view Name, size_t& ArraySize) override;
    virtual void EndArrayScope() override
    {
        if (!NodeStack.empty()) 
            NodeStack.pop_back();
    }
};
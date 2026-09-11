#pragma once

#include <memory>
#include <string_view>

using FObjectCreator = std::unique_ptr<class UObject>(*)(); 

struct FTypeInfo {
    std::string_view TypeName;
    const FTypeInfo* Parent{ nullptr };
	FObjectCreator Creator{ nullptr };

    [[nodiscard]] bool IsA(const FTypeInfo* Type) const noexcept {
        for (const FTypeInfo* Current = this; Current != nullptr; Current = Current->Parent) {
            if (Current == Type) {
                return true;
            }
        }

        return false;
    }

    template<typename T>
    [[nodiscard]] bool IsA() const noexcept{
		const auto TypeInfo = T::StaticTypeInfo();
		return IsA(TypeInfo);
    }

    [[nodiscard]] bool isExactlyA(const FTypeInfo* Type) const noexcept {
        return this == Type;
	}
};

#define JG_DECLARE_ROOT_TYPEINFO(Type) \
    using TypeInfoOwner = Type; \
    inline static const FTypeInfo TypeInfo{ #Type, nullptr, +[]() -> std::unique_ptr<UObject> { return std::make_unique<Type>(); } }; \
    static const FTypeInfo* StaticTypeInfo() noexcept { return &TypeInfo; } \
    virtual const FTypeInfo* GetTypeInfo() const noexcept { return &TypeInfo; }

#define JG_DECLARE_DERIVED_TYPEINFO(Type, ParentType) \
    using TypeInfoOwner = Type; \
    inline static const FTypeInfo TypeInfo{ #Type, ParentType::StaticTypeInfo(), +[]() -> std::unique_ptr<UObject> { return std::make_unique<Type>(); } }; \
    static const FTypeInfo* StaticTypeInfo() noexcept { return &TypeInfo; } \
    virtual const FTypeInfo* GetTypeInfo() const noexcept override { return &TypeInfo; }

#define JG_DECLARE_ABSTRACT_DERIVED_TYPEINFO(Type, ParentType) \
    using TypeInfoOwner = Type; \
    inline static const FTypeInfo TypeInfo{ #Type, ParentType::StaticTypeInfo(), nullptr }; \
    static const FTypeInfo* StaticTypeInfo() noexcept { return &TypeInfo; } \
    virtual const FTypeInfo* GetTypeInfo() const noexcept override { return &TypeInfo; }

#define JG_DECLARE_CHANNEL_MESSAGE(MessageType) \
    inline static const FTypeInfo TypeInfo{ #MessageType, nullptr, nullptr }; \
    static const FTypeInfo& StaticTypeInfo() noexcept { return TypeInfo; } \
    MessageType() = default; \
    ~MessageType() = default; \
    MessageType(const MessageType&) = default; \
    MessageType& operator=(const MessageType&) = default; \
    MessageType(MessageType&&) noexcept = default; \
    MessageType& operator=(MessageType&&) noexcept = default
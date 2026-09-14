#pragma once

#include <cstddef>
#include <type_traits>

#include "FObjectHandle.h"
#include "UObject.h"
#include "UObjectSystem.h"

template<typename T>
class TObjectRef {
public:
    TObjectRef() = default;


    TObjectRef(std::nullptr_t) noexcept {
    }

    TObjectRef(T* Object) noexcept {
        Set(Object);
    }

    explicit TObjectRef(FObjectHandle InHandle) noexcept
        : Handle(InHandle) {
    }

    T* Get() const noexcept {
        ValidateType();

        UObject* Object = UObjectSystem::Resolve(Handle);

        if (Object == nullptr || !Object->GetTypeInfo()->IsA(T::StaticTypeInfo())) {
            return nullptr;
        }

        return static_cast<T*>(Object);
    }

    T* operator->() const noexcept {
        return Get();
    }

    T& operator*() const noexcept {
        return *Get();
    }

    explicit operator bool() const noexcept {
        return IsValid();
    }

    bool IsValid() const noexcept {
        return Get() != nullptr;
    }

    void Set(T* Object) noexcept {
        ValidateType();

        Handle = Object != nullptr ? Object->GetHandle() : FObjectHandle{};
    }

    void SetHandle(FObjectHandle InHandle) noexcept {
        Handle = InHandle;
    }

    void Reset() noexcept {
        Handle = {};
    }

    FObjectHandle GetHandle() const noexcept {
        return Handle;
    }

private:
    static constexpr void ValidateType() noexcept {
        static_assert(std::is_base_of_v<UObject, T>, "TObjectRef<T> requires T to derive from UObject.");
        static_assert(std::is_same_v<typename T::TypeInfoOwner, T>, "TObjectRef<T> requires T to declare its own type information.");
    }

    FObjectHandle Handle;
};

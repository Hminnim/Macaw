#include "PCH.h"
#include "doctest.h"

#include "../Core/Base/TObjectRef.h"

namespace
{
    class UObjectRefBase : public UObject
    {
    public:
        JG_DECLARE_DERIVED_TYPEINFO(UObjectRefBase, UObject)

        int32 GetValue() const
        {
            return 42;
        }
    };

    class UObjectRefDerived : public UObjectRefBase
    {
    public:
        JG_DECLARE_DERIVED_TYPEINFO(UObjectRefDerived, UObjectRefBase)
    };

    class UObjectRefOther : public UObject
    {
    public:
        JG_DECLARE_DERIVED_TYPEINFO(UObjectRefOther, UObject)
    };
}

TEST_SUITE("TObjectRef Tests")
{
    TEST_CASE("Resolves a registered object through a public base type")
    {
        auto Object = std::make_unique<UObjectRefDerived>();
        UObjectSystem::Register(Object.get());

        TObjectRef<UObjectRefBase> Ref(Object.get());

        REQUIRE(Ref.Get() != nullptr);
        CHECK(Ref.Get() == Object.get());
        CHECK(Ref->GetValue() == 42);

        UObjectSystem::Unregister(Object.get(), Object->GetHandle());
    }

    TEST_CASE("Rejects an object that is not compatible with the requested type")
    {
        auto Object = std::make_unique<UObjectRefDerived>();
        const FObjectHandle Handle = UObjectSystem::Register(Object.get());

        TObjectRef<UObjectRefOther> Ref(Handle);

        CHECK(Ref.Get() == nullptr);
        CHECK_FALSE(Ref.IsValid());

        UObjectSystem::Unregister(Object.get(), Handle);
    }

    TEST_CASE("Becomes invalid after the target is unregistered")
    {
        auto Object = std::make_unique<UObjectRefDerived>();
        const FObjectHandle Handle = UObjectSystem::Register(Object.get());
        TObjectRef<UObjectRefBase> Ref(Object.get());

        UObjectSystem::Unregister(Object.get(), Handle);

        CHECK(Ref.Get() == nullptr);
        CHECK_FALSE(Ref);
    }
}

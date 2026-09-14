#include "PCH.h"
#include "doctest.h"

#include <fstream>

#include <rapidjson/document.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>

#include "../Core/Asset/FAssetRegistry.h"
#include "../Core/Base/TypeRegistry.h"
#include "../Core/Base/UObjectSystem.h"
#include "../Scene/AActor.h"
#include "../Scene/Component/USceneComponent.h"
#include "../Scene/UWorld.h"
#include "../Serialize/FArchiveJson.h"

namespace
{
    struct FSceneLoadProbe
    {
        int RegisterCalls = 0;
        int32 ValueSeenOnRegister = 0;
        bool RootWasResolvedOnRegister = false;
    };

    FSceneLoadProbe GSceneLoadProbe;

    class USceneLoadProbeComponent final : public UActorComponent
    {
    public:
        JG_DECLARE_DERIVED_TYPEINFO(USceneLoadProbeComponent, UActorComponent)

        int32 SerializedValue = 0;

        void OnRegister() override
        {
            ++GSceneLoadProbe.RegisterCalls;
            GSceneLoadProbe.ValueSeenOnRegister = SerializedValue;

            AActor* Owner = GetOwner();
            GSceneLoadProbe.RootWasResolvedOnRegister =
                Owner != nullptr && Owner->GetRootComponent() != nullptr;
        }

    protected:
        void Serialize(FArchive& Archive) override
        {
            UActorComponent::Serialize(Archive);
            Archive.Serialize("SerializedValue", SerializedValue);
        }
    };

    void RegisterSceneLoadTypes()
    {
        TypeRegistry::Register(AActor::StaticTypeInfo());
        TypeRegistry::Register(USceneComponent::StaticTypeInfo());
        TypeRegistry::Register(USceneLoadProbeComponent::StaticTypeInfo());
    }
}

TEST_SUITE("CH2 Scene Load Pipeline")
{
    TEST_CASE("LoadScene resolves serialized data and references before component registration")
    {
        RegisterSceneLoadTypes();
        GSceneLoadProbe = {};
        const uint32 ObjectCountBefore = UObjectSystem::GetObjectCount();

        rapidjson::Document Document;
        Document.SetObject();

        {
            UWorld SourceWorld;
            AActor* SourceActor = SourceWorld.AdoptActor<AActor>();
            REQUIRE(SourceActor != nullptr);

            USceneComponent* SourceRoot =
                SourceActor->AddComponent<USceneComponent>();
            USceneLoadProbeComponent* SourceProbe =
                SourceActor->AddComponent<USceneLoadProbeComponent>();
            SourceProbe->SerializedValue = 73;
            SourceActor->SetRootComponent(SourceRoot);

            FArchiveJson ArchiveSave(Document, Document.GetAllocator());
            size_t AssetCount = 0;
            ArchiveSave.BeginArrayScope("Assets", AssetCount);
            ArchiveSave.EndArrayScope();

            size_t ActorCount = 1;
            ArchiveSave.BeginArrayScope("Actors", ActorCount);
            ArchiveSave.BeginObjectScope("0");
            SourceActor->Save(ArchiveSave);
            ArchiveSave.EndObjectScope();
            ArchiveSave.EndArrayScope();
        }

        GSceneLoadProbe = {};

        const std::filesystem::path ScenePath =
            std::filesystem::temp_directory_path() / "MacawSceneLoadPipelineTests.json";
        {
            std::ofstream OutputFile(ScenePath);
            REQUIRE(OutputFile.is_open());

            rapidjson::OStreamWrapper StreamWrapper(OutputFile);
            rapidjson::Writer<rapidjson::OStreamWrapper> Writer(StreamWrapper);
            Document.Accept(Writer);
        }

        {
            FAssetRegistry AssetRegistry;
            UWorld LoadedWorld;

            REQUIRE(LoadedWorld.LoadScene(ScenePath, nullptr, &AssetRegistry));
            REQUIRE_EQ(LoadedWorld.GetActors().size(), 1);

            AActor* LoadedActor = LoadedWorld.GetActors()[0].get();
            REQUIRE(LoadedActor != nullptr);
            CHECK(LoadedActor->GetRootComponent() != nullptr);

            USceneLoadProbeComponent* LoadedProbe =
                LoadedActor->GetComponent<USceneLoadProbeComponent>();
            REQUIRE(LoadedProbe != nullptr);
            CHECK_EQ(LoadedProbe->SerializedValue, 73);
            CHECK(LoadedProbe->IsRegistered());
            CHECK_EQ(LoadedProbe->GetBelongingWorld(), &LoadedWorld);
            CHECK_EQ(GSceneLoadProbe.RegisterCalls, 1);
            CHECK_EQ(GSceneLoadProbe.ValueSeenOnRegister, 73);
            CHECK(GSceneLoadProbe.RootWasResolvedOnRegister);
        }

        std::filesystem::remove(ScenePath);
        CHECK_EQ(UObjectSystem::GetObjectCount(), ObjectCountBefore);
    }
}

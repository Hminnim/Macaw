#include "PCH.h"
#include "doctest.h"

#include <memory>
#include "../Core/Base/UObject.h"
#include "../Core/Base/UObjectSystem.h"
#include "../Serialize/FArchiveMemory.h"
#include "../Serialize/FArchiveJson.h"

#include <fstream>
#include <sstream>
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include <filesystem>

TEST_SUITE("Serialize Tests") {
    // =================================================================
    // 테스트용 종합 객체 정의 (Stress Test Object)
    // =================================================================
    class UObjectStressTest : public UObject
    {
    public:
        int32 Health = 100;
        FString PlayerName = "JungleEngine";
        FVector3 Location = { 10.5f, -20.0f, 3.14f };
        std::vector<int32> Inventory = { 101, 202, 303, 404 };

        const FTypeInfo* GetTypeInfo() const noexcept override
        {
            static FTypeInfo Info = []() {
                FTypeInfo InitInfo;
                InitInfo.TypeName = "UObjectStressTest";
                InitInfo.Parent = nullptr;
                InitInfo.Creator = []() -> std::unique_ptr<UObject> { return std::make_unique<UObjectStressTest>(); };
                return InitInfo;
                }();
            return &Info;
        }

    protected: 
        void Serialize(FArchive& Archive) override
        {
            UObject::Serialize(Archive); 

            Archive.Serialize("Health", Health);
            Archive.Serialize("PlayerName", PlayerName);
            Archive.Serialize("Location", Location);

            // 동적 배열 직렬화
            size_t ArraySize = Inventory.size();
            Archive.BeginArrayScope("Inventory", ArraySize);

            if (Archive.IsLoading())
            {
                Inventory.resize(ArraySize);
            }

            for (size_t i = 0; i < ArraySize; ++i)
            {
                Archive.Serialize(std::to_string(i), Inventory[i]);
            }

            Archive.EndArrayScope();
        }
    };

    // =================================================================
    // 바이너리 아카이브 (FArchiveMemory) 테스트
    // =================================================================
    TEST_CASE("FArchiveMemory Stress Test")
    {
        auto TestObject = std::make_unique<UObjectStressTest>();
        UObjectSystem::Register(TestObject.get());
        FGuid BeforeGuid = TestObject->GetGuid();

        // --- SAVE ---
        TArray<uint8> SavedData;
        FArchiveMemory ArchiveSave(SavedData);
        TestObject->Save(ArchiveSave);

        // --- MODIFY ---
        TestObject->Health = 999;
        TestObject->PlayerName = "Corrupted";
        TestObject->Location = { 0.0f, 0.0f, 0.0f };
        TestObject->Inventory.clear();
        TestObject->Inventory.push_back(999);
        FGuid AfterGuid = FGuid::NewGuid();
        //TestObject->RestoreGuid(AfterGuid);     // 이건 테스트용으로 public으로 빼두었지만 작동을 확인했으니 이제 사용하지 않음

        // 오염 확인
        CHECK_EQ(TestObject->Health, 999);
        CHECK_EQ(TestObject->Inventory.size(), 1);
        //CHECK_EQ(TestObject->GetGuid(), AfterGuid);

        // --- LOAD ---
        const TArray<uint8> CopyData = SavedData;
        FArchiveMemory ArchiveLoad(CopyData);
        TestObject->Load(ArchiveLoad);

        // --- VERIFY ---
        CHECK_EQ(TestObject->Health, 100);
        CHECK_EQ(TestObject->PlayerName, FString("JungleEngine"));
        CHECK_EQ(TestObject->Location.x, 10.5f);
        CHECK_EQ(TestObject->Location.y, -20.0f);
        CHECK_EQ(TestObject->Location.z, 3.14f);

        CHECK_EQ(TestObject->Inventory.size(), 4);
        CHECK_EQ(TestObject->Inventory[0], 101);
        CHECK_EQ(TestObject->Inventory[3], 404);
        CHECK_EQ(TestObject->GetGuid(), BeforeGuid);
    }

    // =================================================================
    // JSON 아카이브 (FArchiveJson) 파일 입출력 테스트
    // =================================================================
    TEST_CASE("FArchiveJson File I/O Stress Test")
    {
        auto TestObject = std::make_unique<UObjectStressTest>();
        UObjectSystem::Register(TestObject.get());
        FGuid BeforeGuid = TestObject->GetGuid();

        const std::filesystem::path FilePath =
            std::filesystem::temp_directory_path() /
            "MacawSerializeTests.json";

        // --- SAVE ---
        {
            rapidjson::Document SaveDoc;
            SaveDoc.SetObject();

            FArchiveJson ArchiveSave(SaveDoc, SaveDoc.GetAllocator());
            TestObject->Save(ArchiveSave);

            rapidjson::StringBuffer Buffer;
            rapidjson::Writer<rapidjson::StringBuffer> Writer(Buffer);
            SaveDoc.Accept(Writer);

            std::string JsonString = Buffer.GetString();

            std::ofstream OutFile(FilePath);
            REQUIRE(OutFile.is_open());
            OutFile << JsonString;
            OutFile.close();
        }

        // --- MODIFY  ---
        TestObject->Health = 999;
        TestObject->PlayerName = "Corrupted";
        TestObject->Location = { 0.0f, 0.0f, 0.0f };
        TestObject->Inventory.clear();
        TestObject->Inventory.push_back(999);
        //FGuid AfterGuid = FGuid::NewGuid();
        //TestObject->RestoreGuid(AfterGuid);

        // 오염 확인
        CHECK_EQ(TestObject->Health, 999);
        CHECK_EQ(TestObject->Inventory.size(), 1);
        //CHECK_EQ(TestObject->GetGuid(), AfterGuid);

        // --- LOAD ---
        {
            std::ifstream InFile(FilePath);
            REQUIRE(InFile.is_open());

            std::stringstream Buffer;
            Buffer << InFile.rdbuf();
            std::string LoadedJsonString = Buffer.str();
            InFile.close();

            rapidjson::Document LoadDoc;
            LoadDoc.Parse(LoadedJsonString.c_str());
            REQUIRE(LoadDoc.HasParseError() == false); 

            FArchiveJson ArchiveLoad(LoadDoc);
            TestObject->Load(ArchiveLoad);
        }

        // --- VERIFY  ---
        CHECK_EQ(TestObject->Health, 100);
        CHECK_EQ(TestObject->PlayerName, FString("JungleEngine"));
        CHECK_EQ(TestObject->Location.x, 10.5f);
        CHECK_EQ(TestObject->Location.y, -20.0f);
        CHECK_EQ(TestObject->Location.z, 3.14f);

        CHECK_EQ(TestObject->Inventory.size(), 4);
        CHECK_EQ(TestObject->Inventory[0], 101);
        CHECK_EQ(TestObject->Inventory[3], 404);
        CHECK_EQ(TestObject->GetGuid(), BeforeGuid);

        std::filesystem::remove(FilePath);
    }
}

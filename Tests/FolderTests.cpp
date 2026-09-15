#include "PCH.h"
#include "doctest.h"

#include "../Scene/AActor.h"
#include "../Scene/Folder.h"
#include "../Scene/UWorld.h"
#include "../Serialize/FArchiveMemory.h"

TEST_SUITE("Scene Folder") {
    TEST_CASE("A new folder is an independent root record") {
        Folder NewFolder;

        CHECK(NewFolder.GetID().IsValid());
        CHECK(NewFolder.IsRootFolder());
        CHECK(NewFolder.GetParentFolderGuid() == FGuid{});
        CHECK(NewFolder.GetName().empty());
    }

    TEST_CASE("A folder stores its parent as a GUID reference") {
        Folder ParentFolder("Environment");
        Folder ChildFolder("Props");

        ChildFolder.SetParentFolderGuid(ParentFolder.GetID());

        CHECK_FALSE(ChildFolder.IsRootFolder());
        CHECK(ChildFolder.GetParentFolderGuid() == ParentFolder.GetID());

        ChildFolder.ClearParentFolder();

        CHECK(ChildFolder.IsRootFolder());
        CHECK(ChildFolder.GetParentFolderGuid() == FGuid{});
    }

    TEST_CASE("A folder can be reparented without moving its record") {
        Folder EnvironmentFolder("Environment");
        Folder GameplayFolder("Gameplay");
        Folder PropsFolder("Props");

        const FGuid PropsID = PropsFolder.GetID();
        PropsFolder.SetParentFolderGuid(EnvironmentFolder.GetID());
        PropsFolder.SetParentFolderGuid(GameplayFolder.GetID());

        CHECK(PropsFolder.GetID() == PropsID);
        CHECK(PropsFolder.GetParentFolderGuid() == GameplayFolder.GetID());
    }

    TEST_CASE("World owns folder records and actor memberships") {
        UWorld World;
        Folder* EnvironmentFolder = World.CreateFolder("Environment");
        REQUIRE(EnvironmentFolder != nullptr);
        Folder* PropsFolder = World.CreateFolder("Props", EnvironmentFolder->GetID());
        REQUIRE(PropsFolder != nullptr);
        AActor* Actor = World.AdoptActor<AActor>();
        REQUIRE(Actor != nullptr);

        CHECK_EQ(World.GetFolders().size(), 2);
        CHECK(World.SetActorFolder(Actor, PropsFolder->GetID()));
        CHECK(Actor->GetFolderGuid() == PropsFolder->GetID());
        CHECK_FALSE(World.SetFolderParent(EnvironmentFolder->GetID(), PropsFolder->GetID()));
        CHECK_FALSE(World.SetActorFolder(Actor, FGuid::NewGuid()));

        const FGuid EnvironmentGuid = EnvironmentFolder->GetID();
        const FGuid PropsGuid = PropsFolder->GetID();
        CHECK(World.DestroyFolder(EnvironmentGuid));
        CHECK(PropsFolder->IsRootFolder());
        CHECK(Actor->GetFolderGuid() == PropsGuid);
        CHECK(World.DestroyFolder(PropsGuid));
        CHECK(Actor->GetFolderGuid() == FGuid{});
    }

    TEST_CASE("Folder serialization preserves durable record fields") {
        Folder SavedFolder("Props");
        const FGuid ParentGuid = FGuid::NewGuid();
        SavedFolder.SetParentFolderGuid(ParentGuid);

        TArray<uint8> Bytes;
        FArchiveMemory SaveArchive(Bytes);
        SavedFolder.Serialize(SaveArchive);

        const TArray<uint8>& ReadBytes = Bytes;
        Folder LoadedFolder;
        FArchiveMemory LoadArchive(ReadBytes);
        LoadedFolder.Serialize(LoadArchive);

        CHECK(LoadedFolder.GetID() == SavedFolder.GetID());
        CHECK(std::strcmp(LoadedFolder.GetName().c_str(), "Props") == 0);
        CHECK(LoadedFolder.GetParentFolderGuid() == ParentGuid);
    }
}

#include "PCH.h"

#include "Folder.h"

#include "../Serialize/FArchive.h"

Folder::Folder()
    : ID(FGuid::NewGuid()) {
}

Folder::Folder(FString FolderName)
    : ID(FGuid::NewGuid())
    , Name(std::move(FolderName)) {
}

Folder::Folder(FGuid InID, FString InName, FGuid ParentFolder)
    : ID(InID.IsValid() ? InID : FGuid::NewGuid())
    , Name(std::move(InName))
    , ParentFolderGuid(ParentFolder) {
}

void Folder::SetName(FString FolderName) {
    Name = std::move(Name);
}

void Folder::SetParentFolderGuid(FGuid ParentFolder) {
    ParentFolderGuid = ParentFolder;
}

void Folder::ClearParentFolder() {
    ParentFolderGuid = {};
}

void Folder::Serialize(FArchive& Archive) {
    Archive.Serialize("Guid", ID);
    Archive.Serialize("Name", Name);
    Archive.Serialize("ParentGuid", ParentFolderGuid);
}

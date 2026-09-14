#pragma once

#include "../Common.h"
#include "../Core/Base/FGuid.h"

class FArchive;

class Folder {
public:
    Folder();
    explicit Folder(FString FolderName);
    Folder(FGuid ID, FString FolderName, FGuid ParentFolder = {});

    FGuid GetID() const { return ID; }
    const FString& GetName() const { return Name; }
    const FGuid& GetParentFolderGuid() const { return ParentFolderGuid; }
    bool IsRootFolder() const { return !ParentFolderGuid.IsValid(); }

    void SetName(FString FolderName);
    void SetParentFolderGuid(FGuid ParentFolder);
    void ClearParentFolder();
    void Serialize(FArchive& Archive);

private:
    FGuid ID;
    FString Name;
    FGuid ParentFolderGuid;
};

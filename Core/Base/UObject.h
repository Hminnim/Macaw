#pragma once

#include "FGuid.h"
#include "FObjectHandle.h"
#include "TypeInfo.h"

#include <cstddef>
#include <new>

#include "../../Serialize/FArchive.h"
#include "TypeInfo.h"
#include "../../ErrorHandler.h"

#include "../../FName.h"

class UObject;
namespace UObjectSystem
{
	FObjectHandle Register(UObject* Object);
	FObjectHandle RegisterWithGuid(UObject* Object, const FGuid& InGuid);
}

class UObject
{
public:
	UObject();
	virtual ~UObject() = default;

	UObject(const UObject&) = delete;
	UObject& operator=(const UObject&) = delete;

	const FGuid& GetGuid() const;
	FObjectHandle GetHandle() const;

	FName GetName() const { return Name; }
	void SetName(FName InName) { Name = InName; }

	void Save(FArchive& Archive) 
	{
		ErrorHandler::Report(Archive.IsSaving() == false, "Save Error", "Given archive is not set as saving mode", ErrorHandler::EErrorLevel::Error);
		Serialize(Archive);
	}
	void Load(FArchive& Archive) 
	{
		ErrorHandler::Report(Archive.IsLoading() == false, "Load Error", "Given archive is not set as loading mode", ErrorHandler::EErrorLevel::Error);
		Serialize(Archive);
	}

	static void* operator new(std::size_t Size);
	static void operator delete(void* Ptr) noexcept;

	static void* operator new(std::size_t Size, std::align_val_t Alignment);
	static void operator delete(void* Ptr, std::align_val_t Alignment) noexcept;

	// RTTI
	JG_DECLARE_ROOT_TYPEINFO(UObject)
protected:
	virtual void Serialize(FArchive& Archive)
	{
		Archive.Serialize("Guid", Guid);
		FString TypeNameStr(GetTypeInfo()->TypeName);
		Archive.Serialize("TypeName", TypeNameStr);
		Archive.Serialize("Name", Name);
	}

private:
	friend FObjectHandle UObjectSystem::Register(UObject* Object);
	friend FObjectHandle UObjectSystem::RegisterWithGuid(UObject* Object, const FGuid& InGuid);

	void SetHandle(FObjectHandle InHandle);
	void RestoreGuid(const FGuid& InGuid); 

private:
	FGuid Guid;
	FObjectHandle Handle;
	FName Name;
};
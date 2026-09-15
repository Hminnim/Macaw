#pragma once
#include <filesystem>
#include "FEditorSettings.h"

class FAssetRegistry;

class FEditorConfigManager {
public:
	static bool Save(FEditorSettings& Settings, FAssetRegistry* AssetRegistry = nullptr, const std::filesystem::path& ConfigPath = "Editor.ini");
	static bool Load(FEditorSettings& OutSettings, FAssetRegistry* AssetRegistry = nullptr, const std::filesystem::path& ConfigPath = "Editor.ini");

private:
	FEditorConfigManager() = default;
	~FEditorConfigManager() = default;
};
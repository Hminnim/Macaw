#include "PCH.h"
#include "Render/Panel/Stats/StatWindow.h"

#include "Core/Memory/Memory.h"
#include "Core/Base/UObjectSystem.h"
#include "Scene/UWorld.h"
#include "ImGui/imgui.h"
#include "Scene/AActor.h"

void DrawStatWindow(const UWorld& World)
{
	if (!ImGui::Begin("Stats"))
	{
		ImGui::End();
		return;
	}

	const float FPS = ImGui::GetIO().Framerate;
	const float FrameTimeMs = FPS > 0.0f ? 1000.0f / FPS : 0.0f;

	ImGui::Text("FPS: %.1f (%.2f ms)", FPS, FrameTimeMs);

	ImGui::Spacing();

	const uint32 ObjectCount = UObjectSystem::GetObjectCount();

	ImGui::TextUnformatted("Object System");
	ImGui::Separator();

	ImGui::Text("Registered UObjects: %u", ObjectCount);
	ImGui::Text("Actors : %llu", static_cast<unsigned long long>(World.GetActors().size()));

	ImGui::Spacing();

	const Memory::FMemoryStats Stats = Memory::GetStats();

	const double AllocatedKiB = static_cast<double>(Stats.AllocatedBytes) / 1024.0;
	const double PeakKiB = static_cast<double>(Stats.PeakAllocatedBytes) / 1024.0;

	ImGui::TextUnformatted("Heap Memory");
	ImGui::Separator();

	ImGui::Text("Allocated : %llu bytes (%.2f KiB)", static_cast<unsigned long long>(Stats.AllocatedBytes), AllocatedKiB);
	ImGui::Text("Peak Allocated: %llu bytes (%.2f KiB)", static_cast<unsigned long long>(Stats.PeakAllocatedBytes), PeakKiB);
	ImGui::Text("Active allocations: %llu", static_cast<unsigned long long>(Stats.ActiveAllocationCount));
	ImGui::Text("Total allocation calls: %llu", static_cast<unsigned long long>(Stats.TotalAllocationCount));
	ImGui::Text("Total deallocation calls: %llu", static_cast<unsigned long long>(Stats.TotalDeallocationCount));

	float UsageRatio = 0.0f;
	if (Stats.PeakAllocatedBytes > 0)
	{
		UsageRatio = static_cast<float>(Stats.AllocatedBytes) / static_cast<float>(Stats.PeakAllocatedBytes);
	}
	if (UsageRatio > 1.0f) UsageRatio = 1.0f;

	ImGui::ProgressBar(UsageRatio, ImVec2(-1.0f, 0.0f), "Current / Peak");

	ImGui::Spacing();
	ImGui::TextUnformatted("Memory Usage by Tag");
	ImGui::Separator();

	if (ImGui::BeginTable("TagMemoryStatsTable", 4, ImGuiTableFlags_BordersOuter | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit))
	{
		ImGui::TableSetupColumn("Tag");
		ImGui::TableSetupColumn("Allocated");
		ImGui::TableSetupColumn("Active Count");
		ImGui::TableSetupColumn("Share");
		ImGui::TableHeadersRow();

		for (std::size_t i = 0; i < static_cast<std::size_t>(Memory::EMemoryTag::Count); ++i)
		{
			const Memory::EMemoryTag Tag = static_cast<Memory::EMemoryTag>(i);
			const auto& TagStat = Stats.TagStats[i];
			const double TagKiB = static_cast<double>(TagStat.AllocatedBytes) / 1024.0;

			const float SharePercent = (Stats.AllocatedBytes > 0)
				? (static_cast<float>(TagStat.AllocatedBytes) / static_cast<float>(Stats.AllocatedBytes)) * 100.0f
				: 0.0f;

			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted(Memory::GetMemoryTagName(Tag));

			ImGui::TableSetColumnIndex(1);
			ImGui::Text("%.2f KiB (%llu B)", TagKiB, static_cast<unsigned long long>(TagStat.AllocatedBytes));

			ImGui::TableSetColumnIndex(2);
			ImGui::Text("%llu", static_cast<unsigned long long>(TagStat.ActiveAllocationCount));

			ImGui::TableSetColumnIndex(3);
			ImGui::Text("%.1f%%", SharePercent);
		}

		ImGui::EndTable();
	}

	ImGui::Separator();
	ImGui::TextDisabled("Tracked allocations made through Memory::Allocate()");

	ImGui::End();
}

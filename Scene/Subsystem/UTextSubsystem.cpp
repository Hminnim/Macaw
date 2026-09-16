#include "PCH.h"

#include "UTextSubsystem.h"
#include "Scene/Component/UBillboardTextComponent.h"

void UTextSubsystem::RegisterComponent(UBillboardTextComponent* Component)
{
    if (Component == nullptr || ContainsComponent(Component))
    {
        return;
    }

    Components.emplace_back(Component);
}

void UTextSubsystem::UnregisterComponent(UBillboardTextComponent* Component)
{
    std::erase(Components, Component);
}

void UTextSubsystem::BuildTextProbes(FRenderProbe& Probe) const
{
    Probe.TextProbes.clear();

    for (UBillboardTextComponent* Component : Components)
    {
        if (Component == nullptr)
        {
            continue;
        }

        FTextProbe TextProbe{};

        if (Component->MakeTextRender(TextProbe))
        {
            Probe.TextProbes.push_back(std::move(TextProbe));
        }
    }
}

bool UTextSubsystem::ContainsComponent(const UBillboardTextComponent* Component) const
{
    return std::ranges::find(Components, Component) != Components.end();
}

const TArray<UBillboardTextComponent*>&UTextSubsystem::GetRegisteredComponents() const
{
    return Components;
}

void UTextSubsystem::OnDeinitialize()
{
    Components.clear();
}
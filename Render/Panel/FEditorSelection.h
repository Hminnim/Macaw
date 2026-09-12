#pragma once

#include "Core/Base/TObjectRef.h"
#include "FWorldSelectionChangedMessage.h"
#include "Scene/Component/USceneComponent.h"

class FEditorSelection
{
public:
	void HandleSelectionChanged(const FWorldSelectionChangedMessage& Message);

	USceneComponent* GetSelectedComponent() const;

private:
	TObjectRef<USceneComponent> SelectedComponent;
};

// MIT License
// Copyright (c) 2024 Buvi Games

#pragma once

#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"

#include "Figma2UMGListEntryWidget.generated.h"

UCLASS(BlueprintType)
class FIGMA2UMG_API UFigma2UMGListEntryWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()
};

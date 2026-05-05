// MIT License
// Copyright (c) 2024 Buvi Games

#pragma once

#include "CoreMinimal.h"
#include "FigmaEnums.h"

#include "FigmaLayoutConstraint.generated.h"

USTRUCT()
struct FIGMA2UMG_API FFigmaLayoutConstraint
{
public:
	GENERATED_BODY()

	UPROPERTY()
	EFigmaLayoutConstraintVertical Vertical = EFigmaLayoutConstraintVertical::TOP;

	UPROPERTY()
	EFigmaLayoutConstraintHorizontal Horizontal = EFigmaLayoutConstraintHorizontal::LEFT;
};

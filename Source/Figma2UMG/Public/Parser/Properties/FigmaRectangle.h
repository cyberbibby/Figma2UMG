// MIT License
// Copyright (c) 2024 Buvi Games

#pragma once

#include "CoreMinimal.h"

#include "FigmaRectangle.generated.h"

USTRUCT()
struct FIGMA2UMG_API FFigmaRectangle
{
public:
	GENERATED_BODY()

	FVector2D GetPosition(const float Rotation) const;
	FVector2D GetSize(float Rotation) const;
	FVector2D GetCenter() const;

	UPROPERTY()
	float X = 0.0f;

	UPROPERTY()
	float Y = 0.0f;

	UPROPERTY()
	float Width = 0.0f;

	UPROPERTY()
	float Height = 0.0f;
};

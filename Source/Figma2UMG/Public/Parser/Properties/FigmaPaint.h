// MIT License
// Copyright (c) 2024 Buvi Games

#pragma once

#include "CoreMinimal.h"
#include "FigmaBlendMode.h"
#include "FigmaColor.h"
#include "FigmaColorStop.h"
#include "FigmaEnums.h"
#include "FigmaImageFilters.h"
#include "FigmaTransform.h"
#include "FigmaVariableAlias.h"
#include "FigmaVector.h"

#include "FigmaPaint.generated.h"


class UFigmaNode;
class IAssetBuilder;

USTRUCT()
struct FIGMA2UMG_API FFigmaPaint
{
public:
	GENERATED_BODY()

	void PostSerialize(const TSharedPtr<FJsonObject> JsonObj);

	FLinearColor GetLinearColor() const
	{
		return FLinearColor(Color.R, Color.G, Color.B, Opacity);
	}

	FLinearColor GetLinearColorFromSRGB() const
	{
		const auto ConvertSRGBChannelToLinear = [](const float Channel)
			{
				const float SRGBChannel = FMath::Clamp(Channel, 0.0f, 1.0f);
				return SRGBChannel <= 0.04045f
					? SRGBChannel / 12.92f
					: FMath::Pow((SRGBChannel + 0.055f) / 1.055f, 2.4f);
			};

		return FLinearColor(
			ConvertSRGBChannelToLinear(Color.R),
			ConvertSRGBChannelToLinear(Color.G),
			ConvertSRGBChannelToLinear(Color.B),
			FMath::Clamp(Color.A * Opacity, 0.0f, 1.0f));
	}

	void CreateAssetBuilder(const FString& InFileKey, const UFigmaNode* OwnerNode, TArray<TScriptInterface<IAssetBuilder>>& AssetBuilders, bool IsStroke = false);
	TObjectPtr<UTexture2D> GetTexture() const;
	TObjectPtr<UMaterialInterface> GetMaterial() const;

	UPROPERTY()
	EPaintTypes Type = EPaintTypes::SOLID;

	UPROPERTY()
	bool Visible = true;

	UPROPERTY()
	float Opacity = 1.0f;

	UPROPERTY()
	FFigmaColor Color;

	UPROPERTY()
	EFigmaBlendMode BlendMode = EFigmaBlendMode::NORMAL;

	UPROPERTY()
	TArray<FFigmaVector> GradientHandlePositions;

	UPROPERTY()
	TArray<FFigmaColorStop> GradientStops;

	UPROPERTY()
	EScaleMode ScaleMode = EScaleMode::FILL;

	FFigmaTransform ImageTransform;

	UPROPERTY()
	float ScalingFactor = 1.0f;

	UPROPERTY()
	float Rotation = 0.0f;

	UPROPERTY()
	FString ImageRef;

	UPROPERTY()
	FFigmaImageFilters Filters;

	UPROPERTY()
	FString GifRef;

	UPROPERTY()
	TMap<FString, FFigmaVariableAlias> BoundVariables;

protected:
	TScriptInterface<IAssetBuilder> AssetBuilder = nullptr;
};

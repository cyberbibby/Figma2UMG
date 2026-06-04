// MIT License
// Copyright (c) 2024 Buvi Games

#pragma once

#include "CoreMinimal.h"
#include "WidgetBuilder.h"
#include "ComboBoxStringWidgetBuilder.generated.h"

class UComboBoxString;
class UTexture2D;
class UTexture2DBuilder;
struct FFigmaTypeStyle;

UCLASS()
class FIGMA2UMG_API UComboBoxStringWidgetBuilder : public UObject, public IWidgetBuilder
{
	GENERATED_BODY()

public:
	void SetBackgroundTexture2DBuilder(const TObjectPtr<UTexture2DBuilder>& InTexture2DBuilder);
	void SetBackgroundTexture(const TObjectPtr<UTexture2D>& InTexture);
	void SetBackgroundBrushSize(const FVector2D& InBrushSize);
	void SetArrowTexture2DBuilder(const TObjectPtr<UTexture2DBuilder>& InTexture2DBuilder);
	void SetArrowTexture(const TObjectPtr<UTexture2D>& InTexture);
	void SetArrowBrushSize(const FVector2D& InBrushSize);
	void SetArrowPadding(const FMargin& InPadding);
	void SetSelectedOption(const FString& InSelectedOption);
	void SetSelectedOptionStyle(const FFigmaTypeStyle& InStyle);
	void SetSelectedOptionColor(const FLinearColor& InColor);

	virtual void PatchAndInsertWidget(TObjectPtr<UWidgetBlueprint> WidgetBlueprint, const TObjectPtr<UWidget>& WidgetToPatch) override;
	virtual bool TryInsertOrReplace(const TObjectPtr<UWidget>& PrePatchWidget, const TObjectPtr<UWidget>& PostPatchWidget) override;
	virtual void SetWidget(const TObjectPtr<UWidget>& InWidget) override;
	virtual TObjectPtr<UWidget> GetWidget() const override;
	virtual void ResetWidget() override;

private:
	void Setup() const;

	UPROPERTY()
	TObjectPtr<UComboBoxString> Widget = nullptr;

	UPROPERTY()
	TObjectPtr<UTexture2DBuilder> BackgroundTexture2DBuilder = nullptr;

	UPROPERTY()
	TObjectPtr<UTexture2D> BackgroundTexture = nullptr;

	FVector2D BackgroundBrushSize = FVector2D::ZeroVector;

	UPROPERTY()
	TObjectPtr<UTexture2DBuilder> ArrowTexture2DBuilder = nullptr;

	UPROPERTY()
	TObjectPtr<UTexture2D> ArrowTexture = nullptr;

	FVector2D ArrowBrushSize = FVector2D::ZeroVector;
	TOptional<FMargin> ArrowPadding;

	UPROPERTY()
	FString SelectedOption;

	TOptional<FSlateFontInfo> SelectedOptionFont;
	TOptional<FLinearColor> SelectedOptionColor;
};

// MIT License
// Copyright (c) 2024 Buvi Games

#pragma once

#include "CoreMinimal.h"
#include "PanelWidgetBuilder.h"
#include "SingleChildBuilder.h"
#include "Settings/ClassOverrides.h"
#include "WidgetBuilder.h"

#include "GenericWidgetBuilder.generated.h"

class UContentWidget;
class UWidgetBlueprintBuilder;
class UTexture2DBuilder;
class UTexture2D;

UCLASS()
class FIGMA2UMG_API UGenericPanelWidgetBuilder : public UPanelWidgetBuilder
{
	GENERATED_BODY()
public:
	void SetWidgetType(EFigmaUMGWidgetType InWidgetType);

	virtual void PatchAndInsertWidget(TObjectPtr<UWidgetBlueprint> WidgetBlueprint, const TObjectPtr<UWidget>& WidgetToPatch) override;

protected:
	virtual void Setup() const override;

	UPROPERTY()
	EFigmaUMGWidgetType WidgetType = EFigmaUMGWidgetType::None;
};

UCLASS()
class FIGMA2UMG_API UGenericContentWidgetBuilder : public USingleChildBuilder
{
	GENERATED_BODY()
public:
	void SetWidgetType(EFigmaUMGWidgetType InWidgetType);

	virtual void PatchAndInsertWidget(TObjectPtr<UWidgetBlueprint> WidgetBlueprint, const TObjectPtr<UWidget>& WidgetToPatch) override;
	virtual void SetWidget(const TObjectPtr<UWidget>& InWidget) override;
	virtual void ResetWidget() override;

protected:
	virtual TObjectPtr<UContentWidget> GetContentWidget() const override;

	UPROPERTY()
	EFigmaUMGWidgetType WidgetType = EFigmaUMGWidgetType::None;

	UPROPERTY()
	TObjectPtr<UContentWidget> Widget = nullptr;
};

UCLASS()
class FIGMA2UMG_API UGenericLeafWidgetBuilder : public UObject, public IWidgetBuilder
{
	GENERATED_BODY()
public:
	void SetWidgetType(EFigmaUMGWidgetType InWidgetType);
	void SetListEntryWidgetBlueprintBuilder(const TObjectPtr<UWidgetBlueprintBuilder>& InWidgetBlueprintBuilder);
	void SetDesignerPreviewEntryCount(int32 InDesignerPreviewEntryCount);
	void SetVisualTexture2DBuilder(const TObjectPtr<UTexture2DBuilder>& InTexture2DBuilder);
	void SetVisualTexture(const TObjectPtr<UTexture2D>& InTexture);
	void SetCheckedVisualTexture2DBuilder(const TObjectPtr<UTexture2DBuilder>& InTexture2DBuilder);
	void SetCheckedVisualTexture(const TObjectPtr<UTexture2D>& InTexture);
	void SetUncheckedVisualTexture2DBuilder(const TObjectPtr<UTexture2DBuilder>& InTexture2DBuilder);
	void SetUncheckedVisualTexture(const TObjectPtr<UTexture2D>& InTexture);

	virtual void PatchAndInsertWidget(TObjectPtr<UWidgetBlueprint> WidgetBlueprint, const TObjectPtr<UWidget>& WidgetToPatch) override;
	virtual bool TryInsertOrReplace(const TObjectPtr<UWidget>& PrePatchWidget, const TObjectPtr<UWidget>& PostPatchWidget) override;
	virtual void PatchWidgetProperties() override;
	virtual void SetWidget(const TObjectPtr<UWidget>& InWidget) override;
	virtual TObjectPtr<UWidget> GetWidget() const override;
	virtual void ResetWidget() override;

private:
	void Setup() const;

	UPROPERTY()
	EFigmaUMGWidgetType WidgetType = EFigmaUMGWidgetType::None;

	UPROPERTY()
	TObjectPtr<UWidget> Widget = nullptr;

	UPROPERTY()
	TObjectPtr<UWidgetBlueprintBuilder> ListEntryWidgetBlueprintBuilder = nullptr;

	UPROPERTY()
	int32 DesignerPreviewEntryCount = INDEX_NONE;

	UPROPERTY()
	TObjectPtr<UTexture2DBuilder> VisualTexture2DBuilder = nullptr;

	UPROPERTY()
	TObjectPtr<UTexture2D> VisualTexture = nullptr;

	UPROPERTY()
	TObjectPtr<UTexture2DBuilder> CheckedVisualTexture2DBuilder = nullptr;

	UPROPERTY()
	TObjectPtr<UTexture2D> CheckedVisualTexture = nullptr;

	UPROPERTY()
	TObjectPtr<UTexture2DBuilder> UncheckedVisualTexture2DBuilder = nullptr;

	UPROPERTY()
	TObjectPtr<UTexture2D> UncheckedVisualTexture = nullptr;
};

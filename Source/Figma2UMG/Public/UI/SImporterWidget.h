// MIT License
// Copyright (c) 2024 Buvi Games

#pragma once

#include "CoreMinimal.h"
#include "REST/FigmaImporter.h"

class IDetailsView;
class SGridPanel;

class FIGMA2UMG_API SImporterWidget : public SCompoundWidget, public FNotifyHook
{
public:
	SLATE_BEGIN_ARGS(SImporterWidget)
		{
		}

	SLATE_END_ARGS()

	SImporterWidget();
	virtual ~SImporterWidget() override;

	void Construct(const FArguments& InArgs);

private:
	void AddPropertyView(TSharedRef<SGridPanel> Content);
	void CacheDefaultInputValues();
	void LoadSavedInputOverrides();
	void SaveInputOverrides();
	bool HasValidProperties() const;
	bool LoadPropertyOverride(const FName& PropertyName, bool& bLoadedDeprecatedOverride);
	bool SavePropertyOverride(const FName& PropertyName);
	bool ExportPropertyValue(const FName& PropertyName, FString& OutValue) const;
	bool ImportPropertyValue(const FName& PropertyName, const FString& Value) const;
	void HandleFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent);
	bool IsSavedInputProperty(const FPropertyChangedEvent& PropertyChangedEvent, const FProperty* PropertyThatChanged) const;
	const TArray<FName>& GetSavedInputPropertyNames() const;

	FReply DoImport();
	void OnRequestFinished(eRequestStatus Status, FString InMessage);

	virtual void NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged) override;

	TObjectPtr<URequestParams> Properties;

	TSharedPtr<IDetailsView> DetailViewWidget;
	TSharedPtr<SButton> ImportButton;
	FText ImportButtonName;
	FText ImportButtonTooltip;

	TMap<FName, FString> DefaultInputValues;
	TMap<FName, FString> SavedInputOverrides;

	int RowCount = 0;
};

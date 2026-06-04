// MIT License
// Copyright (c) 2024 Buvi Games


#include "UI/SImporterWidget.h"

#include "FigmaImportSubsystem.h"
#include "REST/RequestParams.h"
#include "TimerManager.h"
#include "Figma2UMGModule.h"

#include "Misc/ConfigCacheIni.h"
#include "Widgets/Layout/SGridPanel.h"

#define LOCTEXT_NAMESPACE "Figma2UMG"

namespace
{
	const TCHAR* ImporterLocalSettingsSection = TEXT("Figma2UMGImporter");
	const TCHAR* DeprecatedImporterLocalSettingsSection = TEXT("/Script/Figma2UMG.ImporterWidgetLocalSettings");
}

SImporterWidget::SImporterWidget()
{
	ImportButtonName = LOCTEXT("ImportButtonName", "Import");
	ImportButtonTooltip  = LOCTEXT("ImportButtonTooltip", "This will import the file from Figma according to the value above.");
}

SImporterWidget::~SImporterWidget()
{
	DetailViewWidget.Reset();

	if (HasValidProperties() && Properties->IsRooted())
	{
		Properties->RemoveFromRoot();
	}
	Properties = nullptr;
}

void SImporterWidget::Construct(const FArguments& InArgs)
{
	if (Properties == nullptr)
	{
		Properties = NewObject<URequestParams>();
		Properties->AddToRoot();
	}

	CacheDefaultInputValues();
	LoadSavedInputOverrides();

	TSharedRef<SGridPanel> Content = SNew(SGridPanel).FillColumn(1, 1.0f);
	TSharedRef<SBorder> MainContent = SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		.Padding(20.f, 5.f, 10.f, 5.f)
		[
			Content
		];

	ChildSlot[MainContent];

	AddPropertyView(Content);

	Content->AddSlot(0, RowCount)
		.ColumnSpan(2)
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		[
			SAssignNew(ImportButton, SButton)
				.Text(ImportButtonName)
				.ToolTipText(ImportButtonTooltip)
				.OnClicked(this, &SImporterWidget::DoImport)
		];
	RowCount++;

}

void SImporterWidget::AddPropertyView(TSharedRef<SGridPanel> Content)
{
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.bUpdatesFromSelection = false;
	DetailsViewArgs.bLockable = false;
	DetailsViewArgs.bShowPropertyMatrixButton = false;
	DetailsViewArgs.NotifyHook = this;

	DetailsViewArgs.ViewIdentifier = FName("Figma2UMG Importer");
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.bAllowFavoriteSystem = false;
	DetailsViewArgs.bShowOptions = false;
	DetailsViewArgs.bShowObjectLabel = false;
	DetailsViewArgs.bShowModifiedPropertiesOption = true;
	DetailsViewArgs.bShowKeyablePropertiesOption = false;
	DetailsViewArgs.bShowAnimatedPropertiesOption = false;
	DetailsViewArgs.bShowScrollBar = true;
	DetailsViewArgs.bForceHiddenPropertyVisibility = false;
	//DetailsViewArgs.ColumnWidth = ColumnWidth;
	DetailsViewArgs.bShowCustomFilterOption = false;

	DetailViewWidget = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	DetailViewWidget->OnFinishedChangingProperties().AddRaw(this, &SImporterWidget::HandleFinishedChangingProperties);
	DetailViewWidget->SetObject(Properties);
	if (DetailViewWidget.IsValid())
	{
		Content->AddSlot(0, 0)
			.ColumnSpan(2)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Fill)
			[
				DetailViewWidget.ToSharedRef()
			];

		RowCount++;
	}
}

void SImporterWidget::CacheDefaultInputValues()
{
	DefaultInputValues.Empty();

	for (const FName& PropertyName : GetSavedInputPropertyNames())
	{
		FString DefaultValue;
		if (ExportPropertyValue(PropertyName, DefaultValue))
		{
			DefaultInputValues.Add(PropertyName, DefaultValue);
		}
	}
}

void SImporterWidget::LoadSavedInputOverrides()
{
	if (!GConfig || !HasValidProperties())
	{
		return;
	}

	bool bDirty = false;
	for (const FName& PropertyName : GetSavedInputPropertyNames())
	{
		bool bLoadedDeprecatedOverride = false;
		if (LoadPropertyOverride(PropertyName, bLoadedDeprecatedOverride) && bLoadedDeprecatedOverride)
		{
			const FString* SavedValue = SavedInputOverrides.Find(PropertyName);
			if (SavedValue)
			{
				GConfig->SetString(ImporterLocalSettingsSection, *PropertyName.ToString(), **SavedValue, GEditorPerProjectIni);
				GConfig->RemoveKey(DeprecatedImporterLocalSettingsSection, *PropertyName.ToString(), GEditorPerProjectIni);
				bDirty = true;
			}
		}
	}

	if (bDirty)
	{
		GConfig->Flush(false, GEditorPerProjectIni);
	}
}

bool SImporterWidget::LoadPropertyOverride(const FName& PropertyName, bool& bLoadedDeprecatedOverride)
{
	bLoadedDeprecatedOverride = false;

	if (!GConfig)
	{
		return false;
	}

	const FString Key = PropertyName.ToString();
	FString SavedValue;
	bool bHasSavedValue = GConfig->GetString(ImporterLocalSettingsSection, *Key, SavedValue, GEditorPerProjectIni);
	if (!bHasSavedValue)
	{
		bHasSavedValue = GConfig->GetString(DeprecatedImporterLocalSettingsSection, *Key, SavedValue, GEditorPerProjectIni);
		bLoadedDeprecatedOverride = bHasSavedValue;
	}

	if (!bHasSavedValue || !ImportPropertyValue(PropertyName, SavedValue))
	{
		return false;
	}

	SavedInputOverrides.Add(PropertyName, SavedValue);
	return true;
}

void SImporterWidget::SaveInputOverrides()
{
	if (!GConfig || !HasValidProperties())
	{
		return;
	}

	bool bDirty = false;
	for (const FName& PropertyName : GetSavedInputPropertyNames())
	{
		bDirty |= SavePropertyOverride(PropertyName);
	}

	if (bDirty)
	{
		GConfig->Flush(false, GEditorPerProjectIni);
	}
}

bool SImporterWidget::HasValidProperties() const
{
	const UObject* PropertiesObject = Properties.Get();
	return PropertiesObject && PropertiesObject->IsValidLowLevelFast(false) && IsValid(PropertiesObject);
}

bool SImporterWidget::SavePropertyOverride(const FName& PropertyName)
{
	if (!GConfig)
	{
		return false;
	}

	const FString* DefaultValue = DefaultInputValues.Find(PropertyName);
	if (!DefaultValue)
	{
		return false;
	}

	FString CurrentValue;
	if (!ExportPropertyValue(PropertyName, CurrentValue))
	{
		return false;
	}

	const FString Key = PropertyName.ToString();
	if (CurrentValue == *DefaultValue)
	{
		if (SavedInputOverrides.Remove(PropertyName) > 0)
		{
			GConfig->RemoveKey(ImporterLocalSettingsSection, *Key, GEditorPerProjectIni);
			return true;
		}

		return false;
	}

	if (const FString* SavedValue = SavedInputOverrides.Find(PropertyName))
	{
		if (*SavedValue == CurrentValue)
		{
			return false;
		}
	}

	GConfig->SetString(ImporterLocalSettingsSection, *Key, *CurrentValue, GEditorPerProjectIni);
	SavedInputOverrides.Add(PropertyName, CurrentValue);
	return true;
}

bool SImporterWidget::ExportPropertyValue(const FName& PropertyName, FString& OutValue) const
{
	if (!HasValidProperties())
	{
		return false;
	}

	const FProperty* Property = URequestParams::StaticClass()->FindPropertyByName(PropertyName);
	if (!Property)
	{
		return false;
	}

	const void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Properties);
	if (const FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property))
	{
		OutValue = BoolProperty->GetPropertyValue(ValuePtr) ? TEXT("True") : TEXT("False");
		return true;
	}

	Property->ExportText_Direct(OutValue, ValuePtr, nullptr, Properties, PPF_None);
	return true;
}

bool SImporterWidget::ImportPropertyValue(const FName& PropertyName, const FString& Value) const
{
	if (!HasValidProperties())
	{
		return false;
	}

	FProperty* Property = URequestParams::StaticClass()->FindPropertyByName(PropertyName);
	if (!Property)
	{
		return false;
	}

	void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Properties);
	if (const FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property))
	{
		BoolProperty->SetPropertyValue(ValuePtr, Value.ToBool());
		return true;
	}

	const TCHAR* Result = Property->ImportText_Direct(*Value, ValuePtr, Properties, PPF_None);
	return Result != nullptr;
}

void SImporterWidget::HandleFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent)
{
	if (IsSavedInputProperty(PropertyChangedEvent, PropertyChangedEvent.Property))
	{
		SaveInputOverrides();
	}
}

bool SImporterWidget::IsSavedInputProperty(const FPropertyChangedEvent& PropertyChangedEvent, const FProperty* PropertyThatChanged) const
{
	const FName MemberPropertyName = PropertyChangedEvent.GetMemberPropertyName();
	if (GetSavedInputPropertyNames().Contains(MemberPropertyName))
	{
		return true;
	}

	if (PropertyThatChanged)
	{
		const FName PropertyName = PropertyThatChanged->GetFName();
		if (GetSavedInputPropertyNames().Contains(PropertyName))
		{
			return true;
		}
	}

	const FName EventPropertyName = PropertyChangedEvent.GetPropertyName();
	return GetSavedInputPropertyNames().Contains(EventPropertyName);
}

const TArray<FName>& SImporterWidget::GetSavedInputPropertyNames() const
{
	static const TArray<FName> SavedInputPropertyNames =
	{
		GET_MEMBER_NAME_CHECKED(URequestParams, AccessToken),
		GET_MEMBER_NAME_CHECKED(URequestParams, ContentRootFolder),
		GET_MEMBER_NAME_CHECKED(URequestParams, DownloadFontsFromGoogle),
		GET_MEMBER_NAME_CHECKED(URequestParams, GFontsAPIKey),
		GET_MEMBER_NAME_CHECKED(URequestParams, DebugNodeName),
		GET_MEMBER_NAME_CHECKED(URequestParams, FrameToButton),
		GET_MEMBER_NAME_CHECKED(URequestParams, WidgetPrefixMappings),
		GET_MEMBER_NAME_CHECKED(URequestParams, WidgetOverrides),
	};

	return SavedInputPropertyNames;
}

FReply SImporterWidget::DoImport()
{
	if (!HasValidProperties())
	{
		return FReply::Handled();
	}

	SaveInputOverrides();

	UFigmaImportSubsystem* Importer = GEditor->GetEditorSubsystem<UFigmaImportSubsystem>();
	if (Importer)
	{
		UE_LOG_Figma2UMG(Display, TEXT("Connecting with Figma"));
		ImportButton->SetEnabled(false);
		Importer->Request(Properties, FOnFigmaImportUpdateStatusCB::CreateRaw(this, &SImporterWidget::OnRequestFinished));
	}

	return FReply::Handled();
}

void SImporterWidget::OnRequestFinished(eRequestStatus Status, FString InMessage)
{
	bool IsError = Status == eRequestStatus::Failed;
	if (Status == eRequestStatus::Succeeded || Status == eRequestStatus::Failed)
	{
		ImportButton->SetEnabled(true);
		if (IsError)
		{
			UE_LOG_Figma2UMG(Error, TEXT("%s"), *InMessage);
		}
		else
		{
			UE_LOG_Figma2UMG(Display, TEXT("-----------------------------------------------------"));
			UE_LOG_Figma2UMG(Display, TEXT("%s"), *InMessage);
			UE_LOG_Figma2UMG(Display, TEXT("-----------------------------------------------------"));
		}
	}
	else
	{
		UE_LOG_Figma2UMG(Display, TEXT("%s"), *InMessage);
	}
}

void SImporterWidget::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged)
{
	if (IsSavedInputProperty(PropertyChangedEvent, PropertyThatChanged))
	{
		SaveInputOverrides();
	}
}

#undef LOCTEXT_NAMESPACE

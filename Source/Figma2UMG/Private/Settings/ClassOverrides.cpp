// MIT License
// Copyright (c) 2024 Buvi Games

#include "Settings/ClassOverrides.h"

#include "Components/BackgroundBlur.h"
#include "Components/CheckBox.h"
#include "Components/CircularThrobber.h"
#include "Components/ComboBoxString.h"
#include "Components/ContentWidget.h"
#include "Components/EditableText.h"
#include "Components/EditableTextBox.h"
#include "Components/GridPanel.h"
#include "Components/InputKeySelector.h"
#include "Components/InvalidationBox.h"
#include "Components/ListView.h"
#include "Components/MenuAnchor.h"
#include "Components/MultiLineEditableText.h"
#include "Components/MultiLineEditableTextBox.h"
#include "Components/NamedSlot.h"
#include "Components/Overlay.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/RetainerBox.h"
#include "Components/RichTextBlock.h"
#include "Components/SafeZone.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBar.h"
#include "Components/ScrollBox.h"
#include "Components/Slider.h"
#include "Components/SpinBox.h"
#include "Components/Throbber.h"
#include "Components/TileView.h"
#include "Components/UniformGridPanel.h"
#include "Components/WindowTitleBarArea.h"

FWidgetPrefixMapping::FWidgetPrefixMapping(EFigmaUMGWidgetType InWidgetType, const FString& InPrefix)
	: WidgetType(InWidgetType)
	, Prefix(InPrefix)
{
}

bool FWidgetPrefixMapping::Match(const FString& NodeName) const
{
	return bEnabled
		&& WidgetType != EFigmaUMGWidgetType::None
		&& !Prefix.IsEmpty()
		&& NodeName.StartsWith(Prefix, ESearchCase::IgnoreCase);
}

bool FWidgetOverride::Match(const FString& NodeName) const
{
	if (!HasCondition)
	{
		return true;
	}
	else if (StringCheckType == EOverrideConditionCheck::StartsWith)
	{
		return NodeName.StartsWith(NameComparison);		
	}
	else if (StringCheckType == EOverrideConditionCheck::Contains)
	{
		return NodeName.Contains(NameComparison);
	}
	else if (StringCheckType == EOverrideConditionCheck::WildCard)
	{
		return NodeName.MatchesWildcard(NameComparison);
	}

	return false;
}

const TArray<FWidgetPrefixMapping>& GetDefaultUMGWidgetPrefixMappings()
{
	static const TArray<FWidgetPrefixMapping> DefaultMappings =
	{
		FWidgetPrefixMapping(EFigmaUMGWidgetType::CanvasPanel, TEXT("PNL_")),
		FWidgetPrefixMapping(EFigmaUMGWidgetType::VerticalBox, TEXT("VBX_")),
		FWidgetPrefixMapping(EFigmaUMGWidgetType::HorizontalBox, TEXT("HBX_")),
		FWidgetPrefixMapping(EFigmaUMGWidgetType::Overlay, TEXT("OVR_")),
		FWidgetPrefixMapping(EFigmaUMGWidgetType::WrapBox, TEXT("WPB_")),
		FWidgetPrefixMapping(EFigmaUMGWidgetType::UniformGridPanel, TEXT("UGP_")),
		FWidgetPrefixMapping(EFigmaUMGWidgetType::GridPanel, TEXT("GDP_")),
		FWidgetPrefixMapping(EFigmaUMGWidgetType::WidgetSwitcher, TEXT("WSW_")),
		FWidgetPrefixMapping(EFigmaUMGWidgetType::Border, TEXT("BDR_")),
		FWidgetPrefixMapping(EFigmaUMGWidgetType::SizeBox, TEXT("SIZ_")),
		FWidgetPrefixMapping(EFigmaUMGWidgetType::ScaleBox, TEXT("SCL_")),
		FWidgetPrefixMapping(EFigmaUMGWidgetType::SafeZone, TEXT("SFZ_")),
		FWidgetPrefixMapping(EFigmaUMGWidgetType::MenuAnchor, TEXT("MNA_")),
		FWidgetPrefixMapping(EFigmaUMGWidgetType::NamedSlot, TEXT("NSL_")),
		FWidgetPrefixMapping(EFigmaUMGWidgetType::BackgroundBlur, TEXT("BLR_")),
		FWidgetPrefixMapping(EFigmaUMGWidgetType::InvalidationBox, TEXT("INB_")),
		FWidgetPrefixMapping(EFigmaUMGWidgetType::RetainerBox, TEXT("RTB_")),
		FWidgetPrefixMapping(EFigmaUMGWidgetType::WindowTitleBarArea, TEXT("TBA_")),
		FWidgetPrefixMapping(EFigmaUMGWidgetType::ScrollBox, TEXT("SCR_")),
		FWidgetPrefixMapping(EFigmaUMGWidgetType::ScrollBar, TEXT("SBR_")),
		FWidgetPrefixMapping(EFigmaUMGWidgetType::TextBlock, TEXT("TXT_")),
		FWidgetPrefixMapping(EFigmaUMGWidgetType::RichTextBlock, TEXT("ETXT_")),
		FWidgetPrefixMapping(EFigmaUMGWidgetType::EditableText, TEXT("EDT_")),
		FWidgetPrefixMapping(EFigmaUMGWidgetType::EditableTextBox, TEXT("EDB_")),
		FWidgetPrefixMapping(EFigmaUMGWidgetType::MultiLineEditableText, TEXT("MLT_")),
		FWidgetPrefixMapping(EFigmaUMGWidgetType::MultiLineEditableTextBox, TEXT("MLB_")),
		FWidgetPrefixMapping(EFigmaUMGWidgetType::Image, TEXT("IMG_")),
		FWidgetPrefixMapping(EFigmaUMGWidgetType::Button, TEXT("BTN_")),
		FWidgetPrefixMapping(EFigmaUMGWidgetType::CheckBox, TEXT("CHK_")),
		FWidgetPrefixMapping(EFigmaUMGWidgetType::ComboBoxString, TEXT("CMB_")),
		FWidgetPrefixMapping(EFigmaUMGWidgetType::ProgressBar, TEXT("PBR_")),
		FWidgetPrefixMapping(EFigmaUMGWidgetType::Slider, TEXT("SLD_")),
		FWidgetPrefixMapping(EFigmaUMGWidgetType::SpinBox, TEXT("SPN_")),
		FWidgetPrefixMapping(EFigmaUMGWidgetType::InputKeySelector, TEXT("KEY_")),
		FWidgetPrefixMapping(EFigmaUMGWidgetType::Throbber, TEXT("THB_")),
		FWidgetPrefixMapping(EFigmaUMGWidgetType::CircularThrobber, TEXT("CTH_")),
		FWidgetPrefixMapping(EFigmaUMGWidgetType::Spacer, TEXT("SPC_")),
		FWidgetPrefixMapping(EFigmaUMGWidgetType::ListView, TEXT("LST_")),
		FWidgetPrefixMapping(EFigmaUMGWidgetType::TileView, TEXT("TLV_")),
	};

	return DefaultMappings;
}

void ResetUMGWidgetPrefixMappingsToDefault(TArray<FWidgetPrefixMapping>& OutMappings)
{
	OutMappings = GetDefaultUMGWidgetPrefixMappings();
}

const TCHAR* LexToString(EFigmaUMGWidgetType WidgetType)
{
	switch (WidgetType)
	{
	case EFigmaUMGWidgetType::CanvasPanel:
		return TEXT("CanvasPanel");
	case EFigmaUMGWidgetType::VerticalBox:
		return TEXT("VerticalBox");
	case EFigmaUMGWidgetType::HorizontalBox:
		return TEXT("HorizontalBox");
	case EFigmaUMGWidgetType::Overlay:
		return TEXT("Overlay");
	case EFigmaUMGWidgetType::WrapBox:
		return TEXT("WrapBox");
	case EFigmaUMGWidgetType::UniformGridPanel:
		return TEXT("UniformGridPanel");
	case EFigmaUMGWidgetType::GridPanel:
		return TEXT("GridPanel");
	case EFigmaUMGWidgetType::WidgetSwitcher:
		return TEXT("WidgetSwitcher");
	case EFigmaUMGWidgetType::Border:
		return TEXT("Border");
	case EFigmaUMGWidgetType::SizeBox:
		return TEXT("SizeBox");
	case EFigmaUMGWidgetType::ScaleBox:
		return TEXT("ScaleBox");
	case EFigmaUMGWidgetType::SafeZone:
		return TEXT("SafeZone");
	case EFigmaUMGWidgetType::MenuAnchor:
		return TEXT("MenuAnchor");
	case EFigmaUMGWidgetType::NamedSlot:
		return TEXT("NamedSlot");
	case EFigmaUMGWidgetType::BackgroundBlur:
		return TEXT("BackgroundBlur");
	case EFigmaUMGWidgetType::InvalidationBox:
		return TEXT("InvalidationBox");
	case EFigmaUMGWidgetType::RetainerBox:
		return TEXT("RetainerBox");
	case EFigmaUMGWidgetType::WindowTitleBarArea:
		return TEXT("WindowTitleBarArea");
	case EFigmaUMGWidgetType::ScrollBox:
		return TEXT("ScrollBox");
	case EFigmaUMGWidgetType::ScrollBar:
		return TEXT("ScrollBar");
	case EFigmaUMGWidgetType::TextBlock:
		return TEXT("TextBlock");
	case EFigmaUMGWidgetType::RichTextBlock:
		return TEXT("RichTextBlock");
	case EFigmaUMGWidgetType::EditableText:
		return TEXT("EditableText");
	case EFigmaUMGWidgetType::EditableTextBox:
		return TEXT("EditableTextBox");
	case EFigmaUMGWidgetType::MultiLineEditableText:
		return TEXT("MultiLineEditableText");
	case EFigmaUMGWidgetType::MultiLineEditableTextBox:
		return TEXT("MultiLineEditableTextBox");
	case EFigmaUMGWidgetType::Image:
		return TEXT("Image");
	case EFigmaUMGWidgetType::Button:
		return TEXT("Button");
	case EFigmaUMGWidgetType::CheckBox:
		return TEXT("CheckBox");
	case EFigmaUMGWidgetType::ComboBoxString:
		return TEXT("ComboBoxString");
	case EFigmaUMGWidgetType::ProgressBar:
		return TEXT("ProgressBar");
	case EFigmaUMGWidgetType::Slider:
		return TEXT("Slider");
	case EFigmaUMGWidgetType::SpinBox:
		return TEXT("SpinBox");
	case EFigmaUMGWidgetType::InputKeySelector:
		return TEXT("InputKeySelector");
	case EFigmaUMGWidgetType::Throbber:
		return TEXT("Throbber");
	case EFigmaUMGWidgetType::CircularThrobber:
		return TEXT("CircularThrobber");
	case EFigmaUMGWidgetType::Spacer:
		return TEXT("Spacer");
	case EFigmaUMGWidgetType::ListView:
		return TEXT("ListView");
	case EFigmaUMGWidgetType::TileView:
		return TEXT("TileView");
	case EFigmaUMGWidgetType::None:
	default:
		return TEXT("None");
	}
}

UClass* GetUMGWidgetClass(EFigmaUMGWidgetType WidgetType)
{
	switch (WidgetType)
	{
	case EFigmaUMGWidgetType::CanvasPanel:
		return UCanvasPanel::StaticClass();
	case EFigmaUMGWidgetType::VerticalBox:
		return UVerticalBox::StaticClass();
	case EFigmaUMGWidgetType::HorizontalBox:
		return UHorizontalBox::StaticClass();
	case EFigmaUMGWidgetType::Overlay:
		return UOverlay::StaticClass();
	case EFigmaUMGWidgetType::WrapBox:
		return UWrapBox::StaticClass();
	case EFigmaUMGWidgetType::UniformGridPanel:
		return UUniformGridPanel::StaticClass();
	case EFigmaUMGWidgetType::GridPanel:
		return UGridPanel::StaticClass();
	case EFigmaUMGWidgetType::WidgetSwitcher:
		return UWidgetSwitcher::StaticClass();
	case EFigmaUMGWidgetType::Border:
		return UBorder::StaticClass();
	case EFigmaUMGWidgetType::SizeBox:
		return USizeBox::StaticClass();
	case EFigmaUMGWidgetType::ScaleBox:
		return UScaleBox::StaticClass();
	case EFigmaUMGWidgetType::SafeZone:
		return USafeZone::StaticClass();
	case EFigmaUMGWidgetType::MenuAnchor:
		return UMenuAnchor::StaticClass();
	case EFigmaUMGWidgetType::NamedSlot:
		return UNamedSlot::StaticClass();
	case EFigmaUMGWidgetType::BackgroundBlur:
		return UBackgroundBlur::StaticClass();
	case EFigmaUMGWidgetType::InvalidationBox:
		return UInvalidationBox::StaticClass();
	case EFigmaUMGWidgetType::RetainerBox:
		return URetainerBox::StaticClass();
	case EFigmaUMGWidgetType::WindowTitleBarArea:
		return UWindowTitleBarArea::StaticClass();
	case EFigmaUMGWidgetType::ScrollBox:
		return UScrollBox::StaticClass();
	case EFigmaUMGWidgetType::ScrollBar:
		return UScrollBar::StaticClass();
	case EFigmaUMGWidgetType::TextBlock:
		return UTextBlock::StaticClass();
	case EFigmaUMGWidgetType::RichTextBlock:
		return URichTextBlock::StaticClass();
	case EFigmaUMGWidgetType::EditableText:
		return UEditableText::StaticClass();
	case EFigmaUMGWidgetType::EditableTextBox:
		return UEditableTextBox::StaticClass();
	case EFigmaUMGWidgetType::MultiLineEditableText:
		return UMultiLineEditableText::StaticClass();
	case EFigmaUMGWidgetType::MultiLineEditableTextBox:
		return UMultiLineEditableTextBox::StaticClass();
	case EFigmaUMGWidgetType::Image:
		return UImage::StaticClass();
	case EFigmaUMGWidgetType::Button:
		return UButton::StaticClass();
	case EFigmaUMGWidgetType::CheckBox:
		return UCheckBox::StaticClass();
	case EFigmaUMGWidgetType::ComboBoxString:
		return UComboBoxString::StaticClass();
	case EFigmaUMGWidgetType::ProgressBar:
		return UProgressBar::StaticClass();
	case EFigmaUMGWidgetType::Slider:
		return USlider::StaticClass();
	case EFigmaUMGWidgetType::SpinBox:
		return USpinBox::StaticClass();
	case EFigmaUMGWidgetType::InputKeySelector:
		return UInputKeySelector::StaticClass();
	case EFigmaUMGWidgetType::Throbber:
		return UThrobber::StaticClass();
	case EFigmaUMGWidgetType::CircularThrobber:
		return UCircularThrobber::StaticClass();
	case EFigmaUMGWidgetType::Spacer:
		return USpacer::StaticClass();
	case EFigmaUMGWidgetType::ListView:
		return UListView::StaticClass();
	case EFigmaUMGWidgetType::TileView:
		return UTileView::StaticClass();
	case EFigmaUMGWidgetType::None:
	default:
		return nullptr;
	}
}

bool IsUMGPanelWidgetType(EFigmaUMGWidgetType WidgetType)
{
	const UClass* WidgetClass = GetUMGWidgetClass(WidgetType);
	return WidgetClass && WidgetClass->IsChildOf(UPanelWidget::StaticClass());
}

bool IsUMGContentWidgetType(EFigmaUMGWidgetType WidgetType)
{
	const UClass* WidgetClass = GetUMGWidgetClass(WidgetType);
	return WidgetClass && WidgetClass->IsChildOf(UContentWidget::StaticClass());
}

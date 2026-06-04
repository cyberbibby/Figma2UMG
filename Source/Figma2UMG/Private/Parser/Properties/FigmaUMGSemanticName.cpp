#include "Parser/Properties/FigmaUMGSemanticName.h"

#include "Figma2UMGModule.h"
#include "FigmaImportSubsystem.h"
#include "Editor.h"

namespace
{
	EFigmaUMGWidgetRole RoleFromWidgetType(EFigmaUMGWidgetType WidgetType)
	{
		switch (WidgetType)
		{
		case EFigmaUMGWidgetType::CanvasPanel:
			return EFigmaUMGWidgetRole::Canvas;
		case EFigmaUMGWidgetType::VerticalBox:
			return EFigmaUMGWidgetRole::VBox;
		case EFigmaUMGWidgetType::HorizontalBox:
			return EFigmaUMGWidgetRole::HBox;
		case EFigmaUMGWidgetType::Overlay:
			return EFigmaUMGWidgetRole::Overlay;
		case EFigmaUMGWidgetType::WrapBox:
			return EFigmaUMGWidgetRole::WrapBox;
		case EFigmaUMGWidgetType::UniformGridPanel:
			return EFigmaUMGWidgetRole::UniformGridPanel;
		case EFigmaUMGWidgetType::GridPanel:
			return EFigmaUMGWidgetRole::GridPanel;
		case EFigmaUMGWidgetType::WidgetSwitcher:
			return EFigmaUMGWidgetRole::WidgetSwitcher;
		case EFigmaUMGWidgetType::Border:
			return EFigmaUMGWidgetRole::Border;
		case EFigmaUMGWidgetType::SizeBox:
			return EFigmaUMGWidgetRole::SizeBox;
		case EFigmaUMGWidgetType::ScaleBox:
			return EFigmaUMGWidgetRole::ScaleBox;
		case EFigmaUMGWidgetType::SafeZone:
			return EFigmaUMGWidgetRole::SafeZone;
		case EFigmaUMGWidgetType::MenuAnchor:
			return EFigmaUMGWidgetRole::MenuAnchor;
		case EFigmaUMGWidgetType::NamedSlot:
			return EFigmaUMGWidgetRole::NamedSlot;
		case EFigmaUMGWidgetType::BackgroundBlur:
			return EFigmaUMGWidgetRole::BackgroundBlur;
		case EFigmaUMGWidgetType::InvalidationBox:
			return EFigmaUMGWidgetRole::InvalidationBox;
		case EFigmaUMGWidgetType::RetainerBox:
			return EFigmaUMGWidgetRole::RetainerBox;
		case EFigmaUMGWidgetType::WindowTitleBarArea:
			return EFigmaUMGWidgetRole::WindowTitleBarArea;
		case EFigmaUMGWidgetType::ScrollBox:
			return EFigmaUMGWidgetRole::ScrollBox;
		case EFigmaUMGWidgetType::ScrollBar:
			return EFigmaUMGWidgetRole::ScrollBar;
		case EFigmaUMGWidgetType::TextBlock:
			return EFigmaUMGWidgetRole::Text;
		case EFigmaUMGWidgetType::RichTextBlock:
			return EFigmaUMGWidgetRole::RichText;
		case EFigmaUMGWidgetType::EditableText:
			return EFigmaUMGWidgetRole::EditableText;
		case EFigmaUMGWidgetType::EditableTextBox:
			return EFigmaUMGWidgetRole::EditableTextBox;
		case EFigmaUMGWidgetType::MultiLineEditableText:
			return EFigmaUMGWidgetRole::MultiLineEditableText;
		case EFigmaUMGWidgetType::MultiLineEditableTextBox:
			return EFigmaUMGWidgetRole::MultiLineEditableTextBox;
		case EFigmaUMGWidgetType::Image:
			return EFigmaUMGWidgetRole::Image;
		case EFigmaUMGWidgetType::Button:
			return EFigmaUMGWidgetRole::Button;
		case EFigmaUMGWidgetType::CheckBox:
			return EFigmaUMGWidgetRole::CheckBox;
		case EFigmaUMGWidgetType::ComboBoxString:
			return EFigmaUMGWidgetRole::ComboBoxString;
		case EFigmaUMGWidgetType::ProgressBar:
			return EFigmaUMGWidgetRole::ProgressBar;
		case EFigmaUMGWidgetType::Slider:
			return EFigmaUMGWidgetRole::Slider;
		case EFigmaUMGWidgetType::SpinBox:
			return EFigmaUMGWidgetRole::SpinBox;
		case EFigmaUMGWidgetType::InputKeySelector:
			return EFigmaUMGWidgetRole::InputKeySelector;
		case EFigmaUMGWidgetType::Throbber:
			return EFigmaUMGWidgetRole::Throbber;
		case EFigmaUMGWidgetType::CircularThrobber:
			return EFigmaUMGWidgetRole::CircularThrobber;
		case EFigmaUMGWidgetType::Spacer:
			return EFigmaUMGWidgetRole::Spacer;
		case EFigmaUMGWidgetType::ListView:
			return EFigmaUMGWidgetRole::ListView;
		case EFigmaUMGWidgetType::TileView:
			return EFigmaUMGWidgetRole::TileView;
		case EFigmaUMGWidgetType::None:
		default:
			return EFigmaUMGWidgetRole::Auto;
		}
	}

	bool ParseConfiguredPrefix(const FString& NodeName, FFigmaUMGSemanticName& Result)
	{
		if (!GEditor)
		{
			return false;
		}

		const UFigmaImportSubsystem* Importer = GEditor->GetEditorSubsystem<UFigmaImportSubsystem>();
		if (!Importer)
		{
			return false;
		}

		const FWidgetPrefixMapping* Mapping = Importer->FindWidgetPrefixMappingForNode(NodeName);
		if (!Mapping)
		{
			return false;
		}

		Result.bHasWidgetPrefix = true;
		Result.WidgetType = Mapping->WidgetType;
		Result.Role = RoleFromWidgetType(Mapping->WidgetType);
		Result.SemanticName = NodeName.RightChop(Mapping->Prefix.Len()).TrimStartAndEnd();
		return Result.Role != EFigmaUMGWidgetRole::Auto;
	}
}

const TCHAR* LexToString(EFigmaUMGWidgetRole Role)
{
	switch (Role)
	{
	case EFigmaUMGWidgetRole::Auto:
		return TEXT("Auto");
	case EFigmaUMGWidgetRole::Button:
		return TEXT("Button");
	case EFigmaUMGWidgetRole::Text:
		return TEXT("Text");
	case EFigmaUMGWidgetRole::RichText:
		return TEXT("RichText");
	case EFigmaUMGWidgetRole::Image:
		return TEXT("Image");
	case EFigmaUMGWidgetRole::Border:
		return TEXT("Border");
	case EFigmaUMGWidgetRole::Canvas:
		return TEXT("Canvas");
	case EFigmaUMGWidgetRole::HBox:
		return TEXT("HBox");
	case EFigmaUMGWidgetRole::VBox:
		return TEXT("VBox");
	case EFigmaUMGWidgetRole::WrapBox:
		return TEXT("WrapBox");
	case EFigmaUMGWidgetRole::Overlay:
		return TEXT("Overlay");
	case EFigmaUMGWidgetRole::ScrollBox:
		return TEXT("ScrollBox");
	case EFigmaUMGWidgetRole::ProgressBar:
		return TEXT("ProgressBar");
	case EFigmaUMGWidgetRole::CheckBox:
		return TEXT("CheckBox");
	case EFigmaUMGWidgetRole::Slider:
		return TEXT("Slider");
	case EFigmaUMGWidgetRole::InputText:
		return TEXT("InputText");
	case EFigmaUMGWidgetRole::WidgetSwitcher:
		return TEXT("WidgetSwitcher");
	case EFigmaUMGWidgetRole::Panel:
		return TEXT("Panel");
	case EFigmaUMGWidgetRole::Decor:
		return TEXT("Decor");
	case EFigmaUMGWidgetRole::Ignore:
		return TEXT("Ignore");
	case EFigmaUMGWidgetRole::SizeBox:
		return TEXT("SizeBox");
	case EFigmaUMGWidgetRole::ScaleBox:
		return TEXT("ScaleBox");
	case EFigmaUMGWidgetRole::SafeZone:
		return TEXT("SafeZone");
	case EFigmaUMGWidgetRole::MenuAnchor:
		return TEXT("MenuAnchor");
	case EFigmaUMGWidgetRole::NamedSlot:
		return TEXT("NamedSlot");
	case EFigmaUMGWidgetRole::BackgroundBlur:
		return TEXT("BackgroundBlur");
	case EFigmaUMGWidgetRole::InvalidationBox:
		return TEXT("InvalidationBox");
	case EFigmaUMGWidgetRole::RetainerBox:
		return TEXT("RetainerBox");
	case EFigmaUMGWidgetRole::WindowTitleBarArea:
		return TEXT("WindowTitleBarArea");
	case EFigmaUMGWidgetRole::ScrollBar:
		return TEXT("ScrollBar");
	case EFigmaUMGWidgetRole::EditableText:
		return TEXT("EditableText");
	case EFigmaUMGWidgetRole::EditableTextBox:
		return TEXT("EditableTextBox");
	case EFigmaUMGWidgetRole::MultiLineEditableText:
		return TEXT("MultiLineEditableText");
	case EFigmaUMGWidgetRole::MultiLineEditableTextBox:
		return TEXT("MultiLineEditableTextBox");
	case EFigmaUMGWidgetRole::ComboBoxString:
		return TEXT("ComboBoxString");
	case EFigmaUMGWidgetRole::SpinBox:
		return TEXT("SpinBox");
	case EFigmaUMGWidgetRole::InputKeySelector:
		return TEXT("InputKeySelector");
	case EFigmaUMGWidgetRole::Throbber:
		return TEXT("Throbber");
	case EFigmaUMGWidgetRole::CircularThrobber:
		return TEXT("CircularThrobber");
	case EFigmaUMGWidgetRole::Spacer:
		return TEXT("Spacer");
	case EFigmaUMGWidgetRole::ListView:
		return TEXT("ListView");
	case EFigmaUMGWidgetRole::TileView:
		return TEXT("TileView");
	case EFigmaUMGWidgetRole::UniformGridPanel:
		return TEXT("UniformGridPanel");
	case EFigmaUMGWidgetRole::GridPanel:
		return TEXT("GridPanel");
	default:
		return TEXT("Unknown");
	}
}

FFigmaUMGSemanticName FFigmaUMGSemanticName::Parse(const FString& NodeName)
{
	FFigmaUMGSemanticName Result;
	Result.OriginalName = NodeName;
	ParseConfiguredPrefix(NodeName, Result);
	return Result;
}

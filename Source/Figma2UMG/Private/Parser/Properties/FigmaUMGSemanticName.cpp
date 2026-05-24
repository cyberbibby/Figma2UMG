#include "Parser/Properties/FigmaUMGSemanticName.h"

#include "Figma2UMGModule.h"

namespace
{
	bool ParseRole(const FString& RoleName, EFigmaUMGWidgetRole& OutRole)
	{
		const FString NormalizedRole = RoleName.TrimStartAndEnd();
		if (NormalizedRole.IsEmpty())
		{
			return false;
		}

		static const TMap<FString, EFigmaUMGWidgetRole> RoleMap = {
			{TEXT("BUTTON"), EFigmaUMGWidgetRole::Button},
			{TEXT("TEXT"), EFigmaUMGWidgetRole::Text},
			{TEXT("RICHTEXT"), EFigmaUMGWidgetRole::RichText},
			{TEXT("IMAGE"), EFigmaUMGWidgetRole::Image},
			{TEXT("BORDER"), EFigmaUMGWidgetRole::Border},
			{TEXT("CANVAS"), EFigmaUMGWidgetRole::Canvas},
			{TEXT("HBOX"), EFigmaUMGWidgetRole::HBox},
			{TEXT("VBOX"), EFigmaUMGWidgetRole::VBox},
			{TEXT("WRAPBOX"), EFigmaUMGWidgetRole::WrapBox},
			{TEXT("OVERLAY"), EFigmaUMGWidgetRole::Overlay},
			{TEXT("SCROLLBOX"), EFigmaUMGWidgetRole::ScrollBox},
			{TEXT("PROGRESSBAR"), EFigmaUMGWidgetRole::ProgressBar},
			{TEXT("CHECKBOX"), EFigmaUMGWidgetRole::CheckBox},
			{TEXT("SLIDER"), EFigmaUMGWidgetRole::Slider},
			{TEXT("INPUTTEXT"), EFigmaUMGWidgetRole::InputText},
			{TEXT("WIDGETSWITCHER"), EFigmaUMGWidgetRole::WidgetSwitcher},
			{TEXT("PANEL"), EFigmaUMGWidgetRole::Panel},
			{TEXT("DECOR"), EFigmaUMGWidgetRole::Decor},
			{TEXT("IGNORE"), EFigmaUMGWidgetRole::Ignore},
		};

		if (const EFigmaUMGWidgetRole* FoundRole = RoleMap.Find(NormalizedRole.ToUpper()))
		{
			OutRole = *FoundRole;
			return true;
		}

		return false;
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
	default:
		return TEXT("Unknown");
	}
}

FFigmaUMGSemanticName FFigmaUMGSemanticName::Parse(const FString& NodeName)
{
	FFigmaUMGSemanticName Result;
	Result.OriginalName = NodeName;

	if (!NodeName.StartsWith(TEXT("UMG/"), ESearchCase::IgnoreCase))
	{
		return Result;
	}

	Result.bHasUMGPrefix = true;

	TArray<FString> Segments;
	NodeName.ParseIntoArray(Segments, TEXT("/"), false);
	if (Segments.Num() < 2)
	{
		UE_LOG_Figma2UMG(Warning, TEXT("[FFigmaUMGSemanticName::Parse] Node '%s' uses the UMG prefix but does not declare a widget role."), *NodeName);
		return Result;
	}

	if (!ParseRole(Segments[1], Result.Role))
	{
		UE_LOG_Figma2UMG(Warning, TEXT("[FFigmaUMGSemanticName::Parse] Node '%s' uses unsupported UMG role '%s'."), *NodeName, *Segments[1]);
		Result.Role = EFigmaUMGWidgetRole::Auto;
	}

	if (Segments.Num() > 2)
	{
		TArray<FString> SemanticSegments;
		for (int32 SegmentIndex = 2; SegmentIndex < Segments.Num(); ++SegmentIndex)
		{
			SemanticSegments.Add(Segments[SegmentIndex]);
		}

		Result.SemanticName = FString::Join(SemanticSegments, TEXT("/"));
	}

	return Result;
}

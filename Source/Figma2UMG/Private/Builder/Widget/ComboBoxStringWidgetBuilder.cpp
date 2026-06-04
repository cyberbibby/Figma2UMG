// MIT License
// Copyright (c) 2024 Buvi Games

#include "Builder/Widget/ComboBoxStringWidgetBuilder.h"

#include "Blueprint/WidgetTree.h"
#include "Components/ComboBoxString.h"
#include "Components/Widget.h"
#include "Engine/Font.h"
#include "Engine/UserInterfaceSettings.h"
#include "Figma2UMGModule.h"
#include "FigmaImportSubsystem.h"
#include "Parser/Nodes/FigmaNode.h"
#include "Parser/Properties/FigmaTypeStyle.h"

void UComboBoxStringWidgetBuilder::SetBackgroundTexture2DBuilder(const TObjectPtr<UTexture2DBuilder>& InTexture2DBuilder)
{
	BackgroundTexture2DBuilder = InTexture2DBuilder;
	BackgroundTexture = nullptr;
}

void UComboBoxStringWidgetBuilder::SetBackgroundTexture(const TObjectPtr<UTexture2D>& InTexture)
{
	BackgroundTexture = InTexture;
	BackgroundTexture2DBuilder = nullptr;
}

void UComboBoxStringWidgetBuilder::SetBackgroundBrushSize(const FVector2D& InBrushSize)
{
	BackgroundBrushSize = InBrushSize;
}

void UComboBoxStringWidgetBuilder::SetArrowTexture2DBuilder(const TObjectPtr<UTexture2DBuilder>& InTexture2DBuilder)
{
	ArrowTexture2DBuilder = InTexture2DBuilder;
	ArrowTexture = nullptr;
}

void UComboBoxStringWidgetBuilder::SetArrowTexture(const TObjectPtr<UTexture2D>& InTexture)
{
	ArrowTexture = InTexture;
	ArrowTexture2DBuilder = nullptr;
}

void UComboBoxStringWidgetBuilder::SetArrowBrushSize(const FVector2D& InBrushSize)
{
	ArrowBrushSize = InBrushSize;
}

void UComboBoxStringWidgetBuilder::SetArrowPadding(const FMargin& InPadding)
{
	ArrowPadding = InPadding;
}

void UComboBoxStringWidgetBuilder::SetSelectedOption(const FString& InSelectedOption)
{
	SelectedOption = InSelectedOption;
}

void UComboBoxStringWidgetBuilder::SetSelectedOptionStyle(const FFigmaTypeStyle& InStyle)
{
	FSlateFontInfo FontInfo;

	const UFigmaImportSubsystem* Importer = GEditor ? GEditor->GetEditorSubsystem<UFigmaImportSubsystem>() : nullptr;
	const UFont* FoundFont = Importer ? Importer->FindFontAssetFromFamily(InStyle.FontFamily) : nullptr;
	if (FoundFont)
	{
		FontInfo.FontObject = FoundFont;
	}

	FontInfo.TypefaceFontName = *InStyle.GetFaceName();
#if (ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 3)
	const UUserInterfaceSettings* UISettings = GetDefault<UUserInterfaceSettings>();
	FontInfo.Size = FMath::GridSnap(InStyle.FontSize * UISettings->GetFontDisplayDPI() / static_cast<float>(FontConstants::RenderDPI), 0.01f);
#else
	FontInfo.Size = InStyle.FontSize;
#endif
	FontInfo.LetterSpacing = InStyle.LetterSpacing * 100.0f;
	SelectedOptionFont = FontInfo;
}

void UComboBoxStringWidgetBuilder::SetSelectedOptionColor(const FLinearColor& InColor)
{
	SelectedOptionColor = InColor;
}

void UComboBoxStringWidgetBuilder::PatchAndInsertWidget(TObjectPtr<UWidgetBlueprint> WidgetBlueprint, const TObjectPtr<UWidget>& WidgetToPatch)
{
	Widget = Cast<UComboBoxString>(WidgetToPatch);

	const FString NodeName = Node->GetNodeName();
	const FString WidgetName = Node->GetWidgetName();
	if (Widget)
	{
		UFigmaImportSubsystem* Importer = GEditor->GetEditorSubsystem<UFigmaImportSubsystem>();
		UClass* ClassOverride = Importer ? Importer->GetOverrideClassForNode<UComboBoxString>(NodeName) : nullptr;
		if (ClassOverride && Widget->GetClass() != ClassOverride)
		{
			Widget = UFigmaImportSubsystem::NewWidget<UComboBoxString>(WidgetBlueprint->WidgetTree, NodeName, WidgetName, ClassOverride);
		}
		UFigmaImportSubsystem::TryRenameWidget(WidgetName, Widget);
	}
	else
	{
		Widget = UFigmaImportSubsystem::NewWidget<UComboBoxString>(WidgetBlueprint->WidgetTree, NodeName, WidgetName);
	}

	Insert(WidgetBlueprint->WidgetTree, WidgetToPatch, Widget);
	Setup();
}

bool UComboBoxStringWidgetBuilder::TryInsertOrReplace(const TObjectPtr<UWidget>& PrePatchWidget, const TObjectPtr<UWidget>& PostPatchWidget)
{
	UE_LOG_Figma2UMG(Warning, TEXT("[UComboBoxStringWidgetBuilder::TryInsertOrReplace] Node %s is a ComboBoxString and consumes child visuals as style properties."), *Node->GetNodeName());
	return false;
}

void UComboBoxStringWidgetBuilder::SetWidget(const TObjectPtr<UWidget>& InWidget)
{
	Widget = Cast<UComboBoxString>(InWidget);
}

TObjectPtr<UWidget> UComboBoxStringWidgetBuilder::GetWidget() const
{
	return Widget;
}

void UComboBoxStringWidgetBuilder::ResetWidget()
{
	Widget = nullptr;
}

void UComboBoxStringWidgetBuilder::Setup() const
{
	if (!Widget)
	{
		return;
	}

	if (!SelectedOption.IsEmpty())
	{
		Widget->ClearOptions();
		Widget->AddOption(SelectedOption);
		Widget->SetSelectedOption(SelectedOption);
	}
	if (SelectedOptionFont.IsSet())
	{
PRAGMA_DISABLE_DEPRECATION_WARNINGS
		Widget->Font = SelectedOptionFont.GetValue();
PRAGMA_ENABLE_DEPRECATION_WARNINGS
	}
	if (SelectedOptionColor.IsSet())
	{
PRAGMA_DISABLE_DEPRECATION_WARNINGS
		Widget->ForegroundColor = FSlateColor(SelectedOptionColor.GetValue());
PRAGMA_ENABLE_DEPRECATION_WARNINGS
	}

	FComboBoxStyle ComboBoxStyle = Widget->GetWidgetStyle();
	FComboButtonStyle ComboButtonStyle = ComboBoxStyle.ComboButtonStyle;
	FButtonStyle ButtonStyle = ComboButtonStyle.ButtonStyle;

	FSlateBrush BackgroundBrush;
	if (Figma2UMGBrush::MakeTextureBrush(Node, BackgroundTexture2DBuilder.Get(), BackgroundTexture.Get(), BackgroundBrush))
	{
		if (!BackgroundBrushSize.IsNearlyZero())
		{
			BackgroundBrush.SetImageSize(Figma2UMGLayout::RoundLayoutVector(BackgroundBrushSize));
		}
		ButtonStyle.SetNormal(BackgroundBrush);
		ButtonStyle.SetHovered(BackgroundBrush);
		ButtonStyle.SetPressed(BackgroundBrush);
		ButtonStyle.SetDisabled(BackgroundBrush);
		ButtonStyle.SetNormalPadding(FMargin(0.0f));
		ButtonStyle.SetPressedPadding(FMargin(0.0f));
		ComboButtonStyle.SetButtonStyle(ButtonStyle);
	}

	FSlateBrush ArrowBrush;
	const bool bHasArrow = Figma2UMGBrush::MakeTextureBrush(Node, ArrowTexture2DBuilder.Get(), ArrowTexture.Get(), ArrowBrush);
	if (bHasArrow)
	{
		if (!ArrowBrushSize.IsNearlyZero())
		{
			ArrowBrush.SetImageSize(Figma2UMGLayout::RoundLayoutVector(ArrowBrushSize));
		}
		ComboButtonStyle.SetDownArrowImage(ArrowBrush);
		ComboButtonStyle.SetDownArrowPadding(ArrowPadding.Get(FMargin(0.0f)));
		ComboButtonStyle.SetDownArrowAlignment(VAlign_Center);
		ComboButtonStyle.SetShadowOffset(FVector2D::ZeroVector);
		ComboButtonStyle.SetShadowColorAndOpacity(FLinearColor::Transparent);
	}

	ComboButtonStyle.SetContentPadding(FMargin(0.0f));
	ComboBoxStyle.SetComboButtonStyle(ComboButtonStyle);
	ComboBoxStyle.SetContentPadding(FMargin(0.0f));

	Widget->SetWidgetStyle(ComboBoxStyle);
	Widget->SetContentPadding(FMargin(0.0f));
	Widget->SetHasDownArrow(bHasArrow);
	Widget->RefreshOptions();
}

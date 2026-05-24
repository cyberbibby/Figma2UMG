// MIT License
// Copyright (c) 2024 Buvi Games


#include "Builder/Widget/BorderWidgetBuilder.h"

#include "FigmaImportSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Parser/Nodes/FigmaGroup.h"
#include "Parser/Nodes/FigmaNode.h"
#include "Parser/Nodes/FigmaSection.h"


void UBorderWidgetBuilder::PatchAndInsertWidget(TObjectPtr<UWidgetBlueprint> WidgetBlueprint, const TObjectPtr<UWidget>& WidgetToPatch)
{
	if (const USizeBox* SizeBoxWrapper = Cast<USizeBox>(WidgetToPatch))
	{
		Widget = Cast<UBorder>(SizeBoxWrapper->GetContent());
	}
	else
	{
		Widget = Cast<UBorder>(WidgetToPatch);
	}	

	const FString NodeName = Node->GetNodeName();
	const FFigmaUMGSemanticName SemanticName = Node->GetUMGSemanticName();
	const bool bIsExplicitBorder = SemanticName.Role == EFigmaUMGWidgetRole::Border;
	const FString WidgetName = bIsExplicitBorder
		? Node->GetWidgetName()
		: TEXT("Border-") + Node->GetWidgetName();
	auto ForceRenameWidget = [](const FString& InName, const TObjectPtr<UWidget>& InWidget)
		{
			if (!InWidget)
				return;

			const FString CurrentName = InWidget->GetName();
			if (CurrentName.Equals(InName, ESearchCase::IgnoreCase) || CurrentName.StartsWith(InName + TEXT("_"), ESearchCase::IgnoreCase))
				return;

			const FString UniqueName = MakeUniqueObjectName(InWidget->GetOuter(), InWidget->GetClass(), *InName).ToString();
			InWidget->Rename(*UniqueName);
		};

	if (Widget)
	{
		UFigmaImportSubsystem* Importer = GEditor->GetEditorSubsystem<UFigmaImportSubsystem>();
		UClass* ClassOverride = Importer ? Importer->GetOverrideClassForNode<UBorder>(NodeName) : nullptr;
		if (ClassOverride && Widget->GetClass() != ClassOverride)
		{
			UBorder* NewBorder = UFigmaImportSubsystem::NewWidget<UBorder>(WidgetBlueprint->WidgetTree, NodeName, WidgetName, ClassOverride);
			NewBorder->SetContent(Widget->GetContent());
			Widget = NewBorder;
		}

		if (bIsExplicitBorder)
		{
			ForceRenameWidget(WidgetName, Widget);
		}
		else
		{
			UFigmaImportSubsystem::TryRenameWidget(WidgetName, Widget);
		}
	}
	else
	{
		Widget = UFigmaImportSubsystem::NewWidget<UBorder>(WidgetBlueprint->WidgetTree, NodeName, WidgetName);

		if (WidgetToPatch)
		{
			Widget->SetContent(WidgetToPatch);
		}
	}

#if WITH_EDITOR
	if (bIsExplicitBorder && Widget)
	{
		Widget->SetDisplayLabel(WidgetName);
	}
#endif

	Insert(WidgetBlueprint->WidgetTree, WidgetToPatch, Widget);

	Setup();

	PatchAndInsertChild(WidgetBlueprint, Widget);
}

void UBorderWidgetBuilder::SetWidget(const TObjectPtr<UWidget>& InWidget)
{
	Widget = Cast<UBorder>(InWidget);
	SetChildWidget(Widget);
}

void UBorderWidgetBuilder::ResetWidget()
{
	Super::ResetWidget();
	Widget = nullptr;
}

TObjectPtr<UContentWidget> UBorderWidgetBuilder::GetContentWidget() const
{
	return Widget;
}

void UBorderWidgetBuilder::GetPaddingValue(FMargin& Padding) const
{
	Padding.Left = 0.0f;
	Padding.Right = 0.0f;
	Padding.Top = 0.0f;
	Padding.Bottom = 0.0f;
}

bool UBorderWidgetBuilder::GetAlignmentValues(EHorizontalAlignment& HorizontalAlignment, EVerticalAlignment& VerticalAlignment) const
{
	HorizontalAlignment = HAlign_Fill;
	VerticalAlignment = VAlign_Fill;
	return true;
}

void UBorderWidgetBuilder::Setup() const
{
	FSlateBrush Brush = Widget->Background;
	if (Node->IsA<UFigmaSection>())
	{
		Brush.DrawAs = ESlateBrushDrawType::Image;
	}
	else
	{
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
	}
	SetBrush(Widget, Brush);

	if (const UFigmaSection* FigmaSection = Cast<UFigmaSection>(Node))
	{
		SetFill(FigmaSection->Fills);
		SetStroke(Widget, FigmaSection->Strokes, FigmaSection->StrokeWeight);
	}
	else if(const UFigmaGroup* FigmaGroup = Cast<UFigmaGroup>(Node))
	{
		SetFill(FigmaGroup->Fills);
		SetStroke(Widget, FigmaGroup->Strokes, FigmaGroup->StrokeWeight);

		const FVector4 Corners = FigmaGroup->RectangleCornerRadii.Num() == 4 ? FVector4(FigmaGroup->RectangleCornerRadii[0], FigmaGroup->RectangleCornerRadii[1], FigmaGroup->RectangleCornerRadii[2], FigmaGroup->RectangleCornerRadii[3])
																			 : FVector4(FigmaGroup->CornerRadius, FigmaGroup->CornerRadius, FigmaGroup->CornerRadius, FigmaGroup->CornerRadius);
		SetCorner(Widget, Corners);
	}
}

// MIT License
// Copyright (c) 2024 Buvi Games


#include "Builder/Widget/Panels/VBoxBuilder.h"

#include "Blueprint/WidgetTree.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Parser/Nodes/FigmaGroup.h"
#include "Parser/Nodes/FigmaInstance.h"
#include "Parser/Nodes/Vectors/FigmaText.h"
#include "Parser/Nodes/Vectors/FigmaVectorNode.h"

namespace
{
	struct FBoxChildLayoutData
	{
		EFigmaLayoutSizing HorizontalSizing = EFigmaLayoutSizing::FIXED;
		EFigmaLayoutSizing VerticalSizing = EFigmaLayoutSizing::FIXED;
		FString LayoutAlign;
	};

	bool TryGetChildLayoutData(const UFigmaNode* Node, FBoxChildLayoutData& OutLayout)
	{
		if (const UFigmaGroup* Group = Cast<UFigmaGroup>(Node))
		{
			OutLayout.HorizontalSizing = Group->LayoutSizingHorizontal;
			OutLayout.VerticalSizing = Group->LayoutSizingVertical;
			OutLayout.LayoutAlign = Group->LayoutAlign;
			return true;
		}

		if (const UFigmaInstance* Instance = Cast<UFigmaInstance>(Node))
		{
			OutLayout.HorizontalSizing = Instance->LayoutSizingHorizontal;
			OutLayout.VerticalSizing = Instance->LayoutSizingVertical;
			OutLayout.LayoutAlign = Instance->LayoutAlign;
			return true;
		}

		if (const UFigmaText* Text = Cast<UFigmaText>(Node))
		{
			OutLayout.HorizontalSizing = Text->LayoutSizingHorizontal;
			OutLayout.VerticalSizing = Text->LayoutSizingVertical;
			return true;
		}

		if (Cast<UFigmaVectorNode>(Node))
		{
			return true;
		}

		return false;
	}

	bool IsStretchAlign(const FString& LayoutAlign)
	{
		return LayoutAlign.Equals(TEXT("STRETCH"), ESearchCase::IgnoreCase);
	}

	EHorizontalAlignment ConvertCounterAxisAlignment(EFigmaCounterAxisAlignItems AlignItems)
	{
		switch (AlignItems)
		{
		case EFigmaCounterAxisAlignItems::MIN:
			return HAlign_Left;
		case EFigmaCounterAxisAlignItems::CENTER:
			return HAlign_Center;
		case EFigmaCounterAxisAlignItems::MAX:
			return HAlign_Right;
		case EFigmaCounterAxisAlignItems::BASELINE:
			return HAlign_Fill;
		default:
			return HAlign_Left;
		}
	}

	bool TryGetRelativeChildGeometry(const UFigmaNode* ChildNode, const UFigmaNode* ParentNode, FVector2D& OutPosition, FVector2D& OutSize)
	{
		if (ChildNode == nullptr || ParentNode == nullptr)
		{
			return false;
		}

		OutPosition = ChildNode->GetPosition();
		OutSize = ChildNode->GetAbsoluteSize(true);
		return OutSize.X > 0.0f && OutSize.Y > 0.0f;
	}

	bool ShouldUseGeometrySlots(const UFigmaGroup* Group)
	{
		return Group != nullptr
			&& Group->GetUMGSemanticName().Role == EFigmaUMGWidgetRole::VBox
			&& Group->LayoutMode != EFigmaLayoutMode::VERTICAL;
	}
}

void UVBoxBuilder::PatchAndInsertWidget(TObjectPtr<UWidgetBlueprint> WidgetBlueprint, const TObjectPtr<UWidget>& WidgetToPatch)
{
	Box = Patch<UVerticalBox>(WidgetBlueprint->WidgetTree, WidgetToPatch);

	Insert(WidgetBlueprint->WidgetTree, WidgetToPatch, Box);
	PatchAndInsertChildren(WidgetBlueprint, Box);
	Setup();
}

void UVBoxBuilder::SetWidget(const TObjectPtr<UWidget>& InWidget)
{
	Super::SetWidget(InWidget);
	Box = Cast<UVerticalBox>(Widget);
}

void UVBoxBuilder::ResetWidget()
{
	Super::ResetWidget();
	Box = nullptr;
}

void UVBoxBuilder::Setup() const
{
	const UFigmaGroup* FigmaGroup = Cast<UFigmaGroup>(Node);
	if (Box == nullptr || FigmaGroup == nullptr)
	{
		return;
	}

	const int32 ChildCount = ChildWidgetBuilders.Num();
	const FVector2D ParentSize = FigmaGroup->GetAbsoluteSize(true);
	const bool bUseGeometrySlots = ShouldUseGeometrySlots(FigmaGroup);
	for (int32 ChildIndex = 0; ChildIndex < ChildCount; ++ChildIndex)
	{
		const TScriptInterface<IWidgetBuilder>& ChildBuilder = ChildWidgetBuilders[ChildIndex];
		UWidget* ChildWidget = ChildBuilder ? ChildBuilder->GetWidget() : nullptr;
		UVerticalBoxSlot* Slot = ChildWidget ? Cast<UVerticalBoxSlot>(ChildWidget->Slot) : nullptr;
		if (Slot == nullptr)
		{
			continue;
		}

		FBoxChildLayoutData LayoutData;
		TryGetChildLayoutData(ChildBuilder->GetNode(), LayoutData);
		FVector2D ChildPosition = FVector2D::ZeroVector;
		FVector2D ChildSize = FVector2D::ZeroVector;
		const bool bHasGeometry = bUseGeometrySlots && TryGetRelativeChildGeometry(ChildBuilder->GetNode(), Node, ChildPosition, ChildSize);

		FSlateChildSize SlotSize;
		if (bHasGeometry && ParentSize.Y > KINDA_SMALL_NUMBER)
		{
			SlotSize.SizeRule = ESlateSizeRule::Fill;
			SlotSize.Value = FMath::Max(ChildSize.Y, 1.0f);
		}
		else
		{
			SlotSize.SizeRule = LayoutData.VerticalSizing == EFigmaLayoutSizing::FILL ? ESlateSizeRule::Fill : ESlateSizeRule::Automatic;
			SlotSize.Value = LayoutData.VerticalSizing == EFigmaLayoutSizing::FILL ? 1.0f : 0.0f;
		}
		Slot->SetSize(SlotSize);

		const bool bShouldStretchHorizontally = bHasGeometry || LayoutData.HorizontalSizing == EFigmaLayoutSizing::FILL || IsStretchAlign(LayoutData.LayoutAlign);
		Slot->SetHorizontalAlignment(bShouldStretchHorizontally ? HAlign_Fill : ConvertCounterAxisAlignment(FigmaGroup->CounterAxisAlignItems));
		Slot->SetVerticalAlignment((bHasGeometry || LayoutData.VerticalSizing == EFigmaLayoutSizing::FILL) ? VAlign_Fill : VAlign_Top);

		if (bHasGeometry)
		{
			float BottomPadding = ParentSize.Y - (ChildPosition.Y + ChildSize.Y);
			if (ChildIndex + 1 < ChildCount)
			{
				const TScriptInterface<IWidgetBuilder>& NextBuilder = ChildWidgetBuilders[ChildIndex + 1];
				FVector2D NextPosition = FVector2D::ZeroVector;
				FVector2D NextSize = FVector2D::ZeroVector;
				if (TryGetRelativeChildGeometry(NextBuilder ? NextBuilder->GetNode() : nullptr, Node, NextPosition, NextSize))
				{
					BottomPadding = NextPosition.Y - (ChildPosition.Y + ChildSize.Y);
				}
			}

			Slot->SetPadding(FMargin(
				ChildPosition.X,
				ChildIndex == 0 ? ChildPosition.Y : 0.0f,
				FMath::Max(0.0f, ParentSize.X - (ChildPosition.X + ChildSize.X)),
				FMath::Max(0.0f, BottomPadding)));
		}
		else
		{
			const bool bIsFirstChild = ChildIndex == 0;
			const bool bIsLastChild = ChildIndex == ChildCount - 1;
			Slot->SetPadding(FMargin(
				FigmaGroup->PaddingLeft,
				bIsFirstChild ? FigmaGroup->PaddingTop : 0.0f,
				FigmaGroup->PaddingRight,
				bIsLastChild ? FigmaGroup->PaddingBottom : FigmaGroup->ItemSpacing));
		}
	}
}

void UVBoxBuilder::GetPaddingValue(FMargin& Padding) const
{
	Padding = FMargin(0.0f);
}

bool UVBoxBuilder::GetSizeValue(FVector2D& Size, bool& SizeToContent) const
{
	bool IsValid = Super::GetSizeValue(Size, SizeToContent);
	if (IsValid)
	{
		if (const UFigmaGroup* FigmaGroup = Cast<UFigmaGroup>(Node))
		{
			SizeToContent = FigmaGroup->LayoutSizingVertical == EFigmaLayoutSizing::FILL || FigmaGroup->LayoutSizingVertical == EFigmaLayoutSizing::HUG;
		}
	}

	return IsValid;
}

// MIT License
// Copyright (c) 2024 Buvi Games


#include "Builder/Widget/Panels/HBoxBuilder.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
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

	EVerticalAlignment ConvertCounterAxisAlignment(EFigmaCounterAxisAlignItems AlignItems)
	{
		switch (AlignItems)
		{
		case EFigmaCounterAxisAlignItems::MIN:
			return VAlign_Top;
		case EFigmaCounterAxisAlignItems::CENTER:
			return VAlign_Center;
		case EFigmaCounterAxisAlignItems::MAX:
			return VAlign_Bottom;
		case EFigmaCounterAxisAlignItems::BASELINE:
			return VAlign_Fill;
		default:
			return VAlign_Top;
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
			&& Group->GetUMGSemanticName().Role == EFigmaUMGWidgetRole::HBox
			&& Group->LayoutMode != EFigmaLayoutMode::HORIZONTAL;
	}
}

void UHBoxBuilder::PatchAndInsertWidget(TObjectPtr<UWidgetBlueprint> WidgetBlueprint, const TObjectPtr<UWidget>& WidgetToPatch)
{
	Box = Patch<UHorizontalBox>(WidgetBlueprint->WidgetTree, WidgetToPatch);

	Insert(WidgetBlueprint->WidgetTree, WidgetToPatch, Box);
	PatchAndInsertChildren(WidgetBlueprint, Box);
	Setup();
}

void UHBoxBuilder::SetWidget(const TObjectPtr<UWidget>& InWidget)
{
	Super::SetWidget(InWidget);
	Box = Cast<UHorizontalBox>(Widget);
}

void UHBoxBuilder::ResetWidget()
{
	Super::ResetWidget();
	Box = nullptr;
}

void UHBoxBuilder::Setup() const
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
		UHorizontalBoxSlot* Slot = ChildWidget ? Cast<UHorizontalBoxSlot>(ChildWidget->Slot) : nullptr;
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
		if (bHasGeometry && ParentSize.X > KINDA_SMALL_NUMBER)
		{
			SlotSize.SizeRule = ESlateSizeRule::Fill;
			SlotSize.Value = FMath::Max(Figma2UMGLayout::RoundLayoutValue(ChildSize.X), 1.0f);
		}
		else
		{
			SlotSize.SizeRule = LayoutData.HorizontalSizing == EFigmaLayoutSizing::FILL ? ESlateSizeRule::Fill : ESlateSizeRule::Automatic;
			SlotSize.Value = LayoutData.HorizontalSizing == EFigmaLayoutSizing::FILL ? 1.0f : 0.0f;
		}
		Slot->SetSize(SlotSize);

		const bool bShouldStretchVertically = bHasGeometry || LayoutData.VerticalSizing == EFigmaLayoutSizing::FILL || IsStretchAlign(LayoutData.LayoutAlign);
		Slot->SetHorizontalAlignment((bHasGeometry || LayoutData.HorizontalSizing == EFigmaLayoutSizing::FILL) ? HAlign_Fill : HAlign_Left);
		Slot->SetVerticalAlignment(bShouldStretchVertically ? VAlign_Fill : ConvertCounterAxisAlignment(FigmaGroup->CounterAxisAlignItems));

		if (bHasGeometry)
		{
			float RightPadding = ParentSize.X - (ChildPosition.X + ChildSize.X);
			if (ChildIndex + 1 < ChildCount)
			{
				const TScriptInterface<IWidgetBuilder>& NextBuilder = ChildWidgetBuilders[ChildIndex + 1];
				FVector2D NextPosition = FVector2D::ZeroVector;
				FVector2D NextSize = FVector2D::ZeroVector;
				if (TryGetRelativeChildGeometry(NextBuilder ? NextBuilder->GetNode() : nullptr, Node, NextPosition, NextSize))
				{
					RightPadding = NextPosition.X - (ChildPosition.X + ChildSize.X);
				}
			}

			Slot->SetPadding(Figma2UMGLayout::RoundLayoutMargin(FMargin(
				ChildIndex == 0 ? ChildPosition.X : 0.0f,
				ChildPosition.Y,
				FMath::Max(0.0f, RightPadding),
				FMath::Max(0.0f, ParentSize.Y - (ChildPosition.Y + ChildSize.Y)))));
		}
		else
		{
			const bool bIsFirstChild = ChildIndex == 0;
			const bool bIsLastChild = ChildIndex == ChildCount - 1;
			Slot->SetPadding(Figma2UMGLayout::RoundLayoutMargin(FMargin(
				bIsFirstChild ? FigmaGroup->PaddingLeft : 0.0f,
				FigmaGroup->PaddingTop,
				bIsLastChild ? FigmaGroup->PaddingRight : FigmaGroup->ItemSpacing,
				FigmaGroup->PaddingBottom)));
		}
	}
}

void UHBoxBuilder::SortChildrenForLayout()
{
	ChildWidgetBuilders.Sort([](const TScriptInterface<IWidgetBuilder>& A, const TScriptInterface<IWidgetBuilder>& B)
	{
		const UFigmaNode* NodeA = A.GetInterface() ? A->GetNode() : nullptr;
		const UFigmaNode* NodeB = B.GetInterface() ? B->GetNode() : nullptr;
		if (NodeA == nullptr || NodeB == nullptr)
		{
			return NodeA != nullptr;
		}

		const FVector2D PositionA = NodeA->GetPosition();
		const FVector2D PositionB = NodeB->GetPosition();
		if (!FMath::IsNearlyEqual(PositionA.X, PositionB.X))
		{
			return PositionA.X < PositionB.X;
		}

		return PositionA.Y < PositionB.Y;
	});
}

void UHBoxBuilder::GetPaddingValue(FMargin& Padding) const
{
	Padding = FMargin(0.0f);
}

bool UHBoxBuilder::GetSizeValue(FVector2D& Size, bool& SizeToContent) const
{
	bool IsValid = Super::GetSizeValue(Size, SizeToContent);
	if (IsValid)
	{
		if (const UFigmaGroup* FigmaGroup = Cast<UFigmaGroup>(Node))
		{
			SizeToContent = FigmaGroup->LayoutSizingHorizontal == EFigmaLayoutSizing::FILL || FigmaGroup->LayoutSizingHorizontal == EFigmaLayoutSizing::HUG;
		}
	}

	return IsValid;
}

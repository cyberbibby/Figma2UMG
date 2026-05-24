#include "Builder/Widget/ProgressBarWidgetBuilder.h"

#include "Figma2UMGModule.h"
#include "FigmaImportSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/ProgressBar.h"
#include "Interfaces/FigmaContainer.h"
#include "Parser/Nodes/FigmaGroup.h"
#include "Parser/Nodes/Vectors/FigmaVectorNode.h"
#include "Parser/Properties/FigmaUMGSemanticName.h"

namespace
{
	bool HasRectOverlap(const UFigmaNode* A, const UFigmaNode* B)
	{
		if (A == nullptr || B == nullptr)
		{
			return false;
		}

		const FVector2D APos = A->GetAbsolutePosition(true);
		const FVector2D ASize = A->GetAbsoluteSize(true);
		const FVector2D BPos = B->GetAbsolutePosition(true);
		const FVector2D BSize = B->GetAbsoluteSize(true);

		return APos.X < BPos.X + BSize.X
			&& APos.X + ASize.X > BPos.X
			&& APos.Y < BPos.Y + BSize.Y
			&& APos.Y + ASize.Y > BPos.Y;
	}

	FString GetLeadingSemanticToken(const FString& SemanticName)
	{
		int32 TokenEnd = INDEX_NONE;
		for (int32 Index = 1; Index < SemanticName.Len(); ++Index)
		{
			if (FChar::IsUpper(SemanticName[Index]))
			{
				TokenEnd = Index;
				break;
			}
		}

		return TokenEnd == INDEX_NONE ? SemanticName : SemanticName.Left(TokenEnd);
	}

	bool IsLikelyNamedFill(const FString& ProgressSemanticName, const FString& FillSemanticName)
	{
		if (ProgressSemanticName.IsEmpty() || FillSemanticName.IsEmpty() || !FillSemanticName.EndsWith(TEXT("Fill"), ESearchCase::IgnoreCase))
		{
			return false;
		}

		const FString ExpectedFillName = ProgressSemanticName + TEXT("Fill");
		if (FillSemanticName.Equals(ExpectedFillName, ESearchCase::IgnoreCase))
		{
			return true;
		}

		const FString ProgressToken = GetLeadingSemanticToken(ProgressSemanticName);
		const FString FillToken = GetLeadingSemanticToken(FillSemanticName);
		return !ProgressToken.IsEmpty() && ProgressToken.Equals(FillToken, ESearchCase::IgnoreCase);
	}

	FLinearColor GetFirstVisibleFillColor(const UFigmaNode* Node)
	{
		if (const UFigmaGroup* GroupNode = Cast<UFigmaGroup>(Node))
		{
			for (const FFigmaPaint& Fill : GroupNode->Fills)
			{
				if (Fill.Visible)
				{
					return Fill.GetLinearColor();
				}
			}
		}
		else if (const UFigmaVectorNode* VectorNode = Cast<UFigmaVectorNode>(Node))
		{
			for (const FFigmaPaint& Fill : VectorNode->Fills)
			{
				if (Fill.Visible)
				{
					return Fill.GetLinearColor();
				}
			}
		}

		return FLinearColor(1.0f, 1.0f, 1.0f, 0.0f);
	}

	FVector4 GetCornerRadii(const UFigmaNode* Node)
	{
		if (const UFigmaGroup* GroupNode = Cast<UFigmaGroup>(Node))
		{
			if (GroupNode->RectangleCornerRadii.Num() == 4)
			{
				return FVector4(
					GroupNode->RectangleCornerRadii[0],
					GroupNode->RectangleCornerRadii[1],
					GroupNode->RectangleCornerRadii[2],
					GroupNode->RectangleCornerRadii[3]);
			}

			return FVector4(GroupNode->CornerRadius, GroupNode->CornerRadius, GroupNode->CornerRadius, GroupNode->CornerRadius);
		}

		return FVector4::Zero();
	}

	FSlateBrush MakeRoundedBrush(const UFigmaNode* Node, const FLinearColor& Color)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(nullptr);
		Brush.TintColor = Color;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.ImageType = ESlateBrushImageType::NoImage;
		Brush.ImageSize = Node ? Node->GetAbsoluteSize(true) : FVector2D::ZeroVector;
		Brush.Margin = FMargin(0.0f);
		Brush.OutlineSettings.Width = 0.0f;
		Brush.OutlineSettings.Color = FLinearColor::Transparent;
		Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		Brush.OutlineSettings.CornerRadii = GetCornerRadii(Node);
		return Brush;
	}
}

void UProgressBarWidgetBuilder::PatchAndInsertWidget(TObjectPtr<UWidgetBlueprint> WidgetBlueprint, const TObjectPtr<UWidget>& WidgetToPatch)
{
	Widget = Cast<UProgressBar>(WidgetToPatch);

	const FString NodeName = Node->GetNodeName();
	const FString WidgetName = Node->GetWidgetName();
	if (Widget)
	{
		UFigmaImportSubsystem::TryRenameWidget(WidgetName, Widget);
	}
	else
	{
		Widget = UFigmaImportSubsystem::NewWidget<UProgressBar>(WidgetBlueprint->WidgetTree, NodeName, WidgetName);
	}

	Insert(WidgetBlueprint->WidgetTree, WidgetToPatch, Widget);
	Setup();
}

bool UProgressBarWidgetBuilder::TryInsertOrReplace(const TObjectPtr<UWidget>& PrePatchWidget, const TObjectPtr<UWidget>& PostPatchWidget)
{
	UE_LOG_Figma2UMG(Warning, TEXT("[UProgressBarWidgetBuilder::TryInsertOrReplace] Node %s is a ProgressBar and can't insert widgets."), *Node->GetNodeName());
	return false;
}

void UProgressBarWidgetBuilder::SetWidget(const TObjectPtr<UWidget>& InWidget)
{
	Widget = Cast<UProgressBar>(InWidget);
}

TObjectPtr<UWidget> UProgressBarWidgetBuilder::GetWidget() const
{
	return Widget;
}

void UProgressBarWidgetBuilder::ResetWidget()
{
	Widget = nullptr;
}

void UProgressBarWidgetBuilder::Setup() const
{
	if (!Widget)
	{
		return;
	}

	FProgressBarStyle Style;
	const FLinearColor FillColor = GetFillColor();
	const UFigmaNode* FillNode = FindFillNode();
	Style.BackgroundImage = MakeRoundedBrush(Node, GetTrackColor());
	Style.FillImage = MakeRoundedBrush(FillNode, FillColor);
	Style.MarqueeImage = MakeRoundedBrush(FillNode, FillColor);

	Widget->SetWidgetStyle(Style);
	Widget->SetFillColorAndOpacity(FillColor);
	Widget->SetBorderPadding(FVector2D::ZeroVector);
	Widget->SetBarFillType(EProgressBarFillType::LeftToRight);
	Widget->SetPercent(GetInitialPercent());
}

const UFigmaNode* UProgressBarWidgetBuilder::FindFillNode() const
{
	const IFigmaContainer* ParentContainer = Cast<IFigmaContainer>(Node ? Node->GetParentNode() : nullptr);
	const UFigmaNode* FillNode = nullptr;
	if (!ParentContainer || Node == nullptr)
	{
		return FillNode;
	}

	const FFigmaUMGSemanticName SemanticName = Node->GetUMGSemanticName();
	if (SemanticName.SemanticName.IsEmpty())
	{
		return FillNode;
	}

	const FString ExpectedFillName = SemanticName.SemanticName + TEXT("Fill");
	for (const UFigmaNode* SiblingNode : ParentContainer->GetChildrenConst())
	{
		if (SiblingNode == nullptr || SiblingNode == Node)
		{
			continue;
		}

		const FFigmaUMGSemanticName SiblingSemanticName = SiblingNode->GetUMGSemanticName();
		if (SiblingSemanticName.Role == EFigmaUMGWidgetRole::Ignore
			&& SiblingSemanticName.SemanticName.Equals(ExpectedFillName, ESearchCase::IgnoreCase))
		{
			FillNode = SiblingNode;
			break;
		}
	}

	if (FillNode != nullptr)
	{
		return FillNode;
	}

	for (const UFigmaNode* SiblingNode : ParentContainer->GetChildrenConst())
	{
		if (SiblingNode == nullptr || SiblingNode == Node)
		{
			continue;
		}

		const FFigmaUMGSemanticName SiblingSemanticName = SiblingNode->GetUMGSemanticName();
		if (SiblingSemanticName.Role == EFigmaUMGWidgetRole::Ignore
			&& IsLikelyNamedFill(SemanticName.SemanticName, SiblingSemanticName.SemanticName)
			&& HasRectOverlap(Node, SiblingNode))
		{
			FillNode = SiblingNode;
			break;
		}
	}

	if (FillNode != nullptr)
	{
		return FillNode;
	}

	float BestOverlapWidth = 0.0f;
	const FVector2D TrackPosition = Node->GetAbsolutePosition(true);
	const FVector2D TrackSize = Node->GetAbsoluteSize(true);
	for (const UFigmaNode* SiblingNode : ParentContainer->GetChildrenConst())
	{
		if (SiblingNode == nullptr || SiblingNode == Node)
		{
			continue;
		}

		const FFigmaUMGSemanticName SiblingSemanticName = SiblingNode->GetUMGSemanticName();
		if (SiblingSemanticName.Role != EFigmaUMGWidgetRole::Ignore
			|| !SiblingSemanticName.SemanticName.EndsWith(TEXT("Fill"), ESearchCase::IgnoreCase)
			|| !HasRectOverlap(Node, SiblingNode))
		{
			continue;
		}

		const FVector2D FillPosition = SiblingNode->GetAbsolutePosition(true);
		const FVector2D FillSize = SiblingNode->GetAbsoluteSize(true);
		const float OverlapLeft = FMath::Max(TrackPosition.X, FillPosition.X);
		const float OverlapRight = FMath::Min(TrackPosition.X + TrackSize.X, FillPosition.X + FillSize.X);
		const float OverlapWidth = FMath::Max(0.0f, OverlapRight - OverlapLeft);
		if (OverlapWidth > BestOverlapWidth)
		{
			BestOverlapWidth = OverlapWidth;
			FillNode = SiblingNode;
		}
	}

	return FillNode;
}

FLinearColor UProgressBarWidgetBuilder::GetTrackColor() const
{
	return GetFirstVisibleFillColor(Node);
}

FLinearColor UProgressBarWidgetBuilder::GetFillColor() const
{
	if (const UFigmaNode* FillNode = FindFillNode())
	{
		return GetFirstVisibleFillColor(FillNode);
	}

	return FLinearColor::White;
}

float UProgressBarWidgetBuilder::GetInitialPercent() const
{
	const UFigmaNode* FillNode = FindFillNode();
	if (FillNode == nullptr)
	{
		UE_LOG_Figma2UMG(Warning, TEXT("[UProgressBarWidgetBuilder::GetInitialPercent] ProgressBar node %s is missing a sibling fill node named UMG/Ignore/%sFill."), *Node->GetNodeName(), *Node->GetUMGSemanticName().SemanticName);
		return 0.0f;
	}

	const FVector2D TrackSize = Node->GetAbsoluteSize(true);
	const FVector2D FillSize = FillNode->GetAbsoluteSize(true);
	const bool bTreatAsVertical = TrackSize.Y > TrackSize.X;
	const float TrackAxisSize = bTreatAsVertical ? TrackSize.Y : TrackSize.X;
	const float FillAxisSize = bTreatAsVertical ? FillSize.Y : FillSize.X;
	if (TrackAxisSize <= KINDA_SMALL_NUMBER)
	{
		UE_LOG_Figma2UMG(Warning, TEXT("[UProgressBarWidgetBuilder::GetInitialPercent] ProgressBar node %s has invalid %s axis %.3f."), *Node->GetNodeName(), bTreatAsVertical ? TEXT("height") : TEXT("width"), TrackAxisSize);
		return 0.0f;
	}

	return FMath::Clamp(FillAxisSize / TrackAxisSize, 0.0f, 1.0f);
}

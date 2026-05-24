// MIT License
// Copyright (c) 2024 Buvi Games


#include "Parser/Nodes/FigmaGroup.h"

#include "Parser/Nodes/FigmaComponentSet.h"
#include "Parser/Nodes/FigmaCanvas.h"
#include "Builder/Asset/MaterialBuilder.h"
#include "Builder/Widget/BorderWidgetBuilder.h"
#include "Builder/Widget/ButtonWidgetBuilder.h"
#include "Builder/Widget/ImageWidgetBuilder.h"
#include "Builder/Widget/PanelWidgetBuilder.h"
#include "Builder/Widget/ProgressBarWidgetBuilder.h"
#include "Builder/Widget/SizeBoxWidgetBuilder.h"
#include "Builder/Widget/WidgetBuilder.h"
#include "Builder/Widget/Panels/CanvasBuilder.h"
#include "Builder/Widget/Panels/HBoxBuilder.h"
#include "Builder/Widget/Panels/VBoxBuilder.h"
#include "Builder/Widget/Panels/WBoxBuilder.h"
#include "Components/CanvasPanel.h"
#include "Components/Spacer.h"
#include "Components/WrapBox.h"
#include "Figma2UMGModule.h"
#include "Parser/Properties/FigmaAction.h"
#include "Parser/Properties/FigmaTrigger.h"
#include "Parser/Properties/FigmaUMGSemanticName.h"
#include "UObject/ScriptInterface.h"

namespace
{
	bool HasVisiblePaint(const TArray<FFigmaPaint>& Paints)
	{
		for (const FFigmaPaint& Paint : Paints)
		{
			if (Paint.Visible)
			{
				return true;
			}
		}

		return false;
	}

	int32 GetMeaningfulChildCount(const TArray<TObjectPtr<UFigmaNode>>& Children)
	{
		int32 Result = 0;
		for (const UFigmaNode* Child : Children)
		{
			if (Child == nullptr)
			{
				continue;
			}

			if (Child->GetUMGSemanticName().Role == EFigmaUMGWidgetRole::Ignore)
			{
				continue;
			}

			++Result;
		}

		return Result;
	}

	UImageWidgetBuilder* CreateImageBuilderForGroup(const UFigmaGroup* Group)
	{
		UImageWidgetBuilder* ImageBuilder = NewObject<UImageWidgetBuilder>();
		ImageBuilder->SetNode(Group);

		for (const FFigmaPaint& Fill : Group->Fills)
		{
			if (!Fill.Visible)
			{
				continue;
			}

			if (UTexture2D* Texture = Fill.GetTexture())
			{
				ImageBuilder->SetTexture(Texture);
				return ImageBuilder;
			}

			if (UMaterialInterface* Material = Fill.GetMaterial())
			{
				ImageBuilder->SetMaterial(Material, Fill.GetLinearColor());
				return ImageBuilder;
			}

			ImageBuilder->SetColor(Fill.GetLinearColor());
			return ImageBuilder;
		}

		return ImageBuilder;
	}

	bool ParentUsesFreeLayout(const UFigmaNode* ParentNode)
	{
		if (ParentNode == nullptr || ParentNode->IsA<UFigmaCanvas>())
		{
			return true;
		}

		if (const UFigmaGroup* ParentGroup = Cast<UFigmaGroup>(ParentNode))
		{
			return ParentGroup->LayoutMode == EFigmaLayoutMode::NONE;
		}

		return false;
	}

	bool HasFixedSizing(const UFigmaGroup* Group)
	{
		return Group != nullptr
			&& (Group->LayoutSizingHorizontal == EFigmaLayoutSizing::FIXED
				|| Group->LayoutSizingVertical == EFigmaLayoutSizing::FIXED);
	}

	FString MakeContentWidgetName(const UFigmaNode* Node)
	{
		if (Node == nullptr)
		{
			return TEXT("Content");
		}

		FString BaseName = Node->GetWidgetName();
		if (BaseName.StartsWith(TEXT("Button_")))
		{
			BaseName.RemoveFromStart(TEXT("Button_"));
		}

		return TEXT("Content_") + BaseName;
	}
}

void UFigmaGroup::PostSerialize(const TObjectPtr<UFigmaNode> InParent, const TSharedRef<FJsonObject> JsonObj)
{
	Super::PostSerialize(InParent, JsonObj);

	PostSerializeProperty(JsonObj, "fills", Fills);
	PostSerializeProperty(JsonObj, "strokes", Strokes);
	PostSerializeProperty(JsonObj, "interactions", Interactions);
}

bool UFigmaGroup::CreateAssetBuilder(const FString& InFileKey, TArray<TScriptInterface<IAssetBuilder>>& AssetBuilders)
{
	CreatePaintAssetBuilderIfNeeded(InFileKey, AssetBuilders, Fills, Strokes);

	return Super::CreateAssetBuilder(InFileKey, AssetBuilders);
}

FString UFigmaGroup::GetPackageNameForBuilder(const TScriptInterface<IAssetBuilder>& InAssetBuilder) const
{
	if (Cast<UMaterialBuilder>(InAssetBuilder.GetObject()))
	{
		TObjectPtr<UFigmaNode> TopParentNode = ParentNode;
		while (TopParentNode && TopParentNode->GetParentNode())
		{
			TopParentNode = TopParentNode->GetParentNode();
		}
		return TopParentNode->GetCurrentPackagePath() + TEXT("/") + "Material";
	}

	return Super::GetPackageNameForBuilder(InAssetBuilder);
}

TScriptInterface<IWidgetBuilder> UFigmaGroup::CreateWidgetBuilders(bool IsRoot/*= false*/, bool AllowFrameButton/*= true*/) const
{
	const FFigmaUMGSemanticName SemanticName = GetUMGSemanticName();
	switch (SemanticName.Role)
	{
	case EFigmaUMGWidgetRole::Ignore:
		return nullptr;
	case EFigmaUMGWidgetRole::Button:
	{
		if (!AllowFrameButton)
		{
			return CreateContainersBuilder(EFigmaUMGWidgetRole::Auto, false, true, MakeContentWidgetName(this));
		}

		if (Children.IsEmpty() && !HasVisiblePaint(Fills) && !HasVisiblePaint(Strokes))
		{
			UE_LOG_Figma2UMG(Warning, TEXT("[UFigmaGroup::CreateWidgetBuilders] Node %s explicitly requests Button but has no visible paints and no children."), *GetNodeName());
		}

		TScriptInterface<UButtonWidgetBuilder> Button = CreateButtonBuilder();
		Button->SetChild(CreateContainersBuilder(EFigmaUMGWidgetRole::Auto, false, true, MakeContentWidgetName(this)));

#if (ENGINE_MAJOR_VERSION < 5 || ENGINE_MINOR_VERSION <= 2)
		return Button.GetInterface();
#else
		return Button;
#endif
	}
	case EFigmaUMGWidgetRole::Canvas:
	case EFigmaUMGWidgetRole::HBox:
	case EFigmaUMGWidgetRole::VBox:
	case EFigmaUMGWidgetRole::WrapBox:
	case EFigmaUMGWidgetRole::Panel:
	case EFigmaUMGWidgetRole::Decor:
		return CreateContainersBuilder(SemanticName.Role);
	case EFigmaUMGWidgetRole::Border:
		if (GetMeaningfulChildCount(Children) > 1)
		{
			UE_LOG_Figma2UMG(Warning, TEXT("[UFigmaGroup::CreateWidgetBuilders] Node %s explicitly requests Border but has multiple meaningful direct children."), *GetNodeName());
		}
		return CreateBorderBuilder();
	case EFigmaUMGWidgetRole::ProgressBar:
	{
		UProgressBarWidgetBuilder* ProgressBarBuilder = NewObject<UProgressBarWidgetBuilder>();
		ProgressBarBuilder->SetNode(this);
		return ProgressBarBuilder;
	}
	case EFigmaUMGWidgetRole::Text:
	case EFigmaUMGWidgetRole::RichText:
		UE_LOG_Figma2UMG(Warning, TEXT("[UFigmaGroup::CreateWidgetBuilders] Node %s is a %s but explicitly requests UMG/%s. Falling back to existing container heuristics."), *GetNodeName(), *GetClass()->GetName(), LexToString(SemanticName.Role));
		break;
	case EFigmaUMGWidgetRole::Image:
		if (!Children.IsEmpty())
		{
			UE_LOG_Figma2UMG(Warning, TEXT("[UFigmaGroup::CreateWidgetBuilders] Node %s explicitly requests Image but has children. Importing the frame itself as UImage and skipping its subtree."), *GetNodeName());
		}
		return CreateImageBuilderForGroup(this);
	case EFigmaUMGWidgetRole::WidgetSwitcher:
	case EFigmaUMGWidgetRole::Overlay:
	case EFigmaUMGWidgetRole::ScrollBox:
	case EFigmaUMGWidgetRole::CheckBox:
	case EFigmaUMGWidgetRole::Slider:
	case EFigmaUMGWidgetRole::InputText:
		UE_LOG_Figma2UMG(Warning, TEXT("[UFigmaGroup::CreateWidgetBuilders] Node %s requests UMG/%s, which is not yet implemented for group nodes. Falling back to existing heuristics."), *GetNodeName(), LexToString(SemanticName.Role));
		break;
	case EFigmaUMGWidgetRole::Auto:
	default:
		break;
	}

	if (AllowFrameButton && IsButton())
	{
		TScriptInterface<UButtonWidgetBuilder> Button = CreateButtonBuilder();
		const TScriptInterface<IWidgetBuilder> Container = CreateContainersBuilder(EFigmaUMGWidgetRole::Auto, false, true, MakeContentWidgetName(this));
		Button->SetChild(Container);

#if (ENGINE_MAJOR_VERSION < 5 || ENGINE_MINOR_VERSION <= 2)
		return Button.GetInterface();
#else
		return Button;
#endif
	}
	else
	{
		TScriptInterface<IWidgetBuilder> WidgetBuilder = CreateContainersBuilder();
		return WidgetBuilder;
	}
}

FVector2D UFigmaGroup::GetAbsolutePosition(const bool IsTopWidgetForNode) const
{
	const float CurrentRotation = IsTopWidgetForNode ? GetAbsoluteRotation() : 0.0f;
	return AbsoluteBoundingBox.GetPosition(CurrentRotation);
}

FVector2D UFigmaGroup::GetAbsoluteSize(const bool IsTopWidgetForNode) const
{
	return AbsoluteBoundingBox.GetSize(IsTopWidgetForNode ? GetAbsoluteRotation() : 0.0f);
}

FVector2D UFigmaGroup::GetAbsoluteCenter() const
{
	return AbsoluteBoundingBox.GetCenter();
}

FMargin UFigmaGroup::GetPadding() const
{
	FMargin Padding;
	Padding.Left = PaddingLeft;
	Padding.Right = PaddingRight;
	Padding.Top = PaddingTop;
	Padding.Bottom = PaddingBottom;

	return Padding;
}

const FFigmaInteraction& UFigmaGroup::GetInteractionFromTrigger(const EFigmaTriggerType TriggerType) const
{
	return UFigmaNode::GetInteractionFromTrigger(Interactions, TriggerType);
}

const FFigmaInteraction& UFigmaGroup::GetInteractionFromAction(const EFigmaActionType ActionType, const EFigmaActionNodeNavigation Navigation) const
{
	return UFigmaNode::GetInteractionFromAction(Interactions, ActionType, Navigation);
}

const FString& UFigmaGroup::GetDestinationIdFromEvent(const FName& EventName) const
{
	const FFigmaInteraction& Interaction = GetInteractionFromAction(EFigmaActionType::NODE, EFigmaActionNodeNavigation::NAVIGATE);
	if (!Interaction.Trigger || !Interaction.Trigger->MatchEvent(EventName.ToString()))
		return TransitionNodeID;

	const UFigmaNodeAction* Action = Interaction.FindActionNode(EFigmaActionNodeNavigation::NAVIGATE);
	if (!Action || Action->DestinationId.IsEmpty())
		return TransitionNodeID;

	return Action->DestinationId;
}

bool UFigmaGroup::IsButton() const
{
	if (!TransitionNodeID.IsEmpty())
	{
		return true;
	}
	else
	{
		const UFigmaImportSubsystem* Importer = GEditor->GetEditorSubsystem<UFigmaImportSubsystem>();
		return  Importer ? Importer->ShouldGenerateButton(GetNodeName()) : false;
	}
}

TScriptInterface<UButtonWidgetBuilder> UFigmaGroup::CreateButtonBuilder() const
{
	UButtonWidgetBuilder* ButtonBuilder = NewObject<UButtonWidgetBuilder>();
	ButtonBuilder->SetNode(this);

	ButtonBuilder->SetDefaultNode(this);
	ButtonBuilder->SetHoveredNode(this);
	ButtonBuilder->SetPressedNode(this);
	ButtonBuilder->SetDisabledNode(this);
	ButtonBuilder->SetFocusedNode(this);

	return ButtonBuilder;
}

TScriptInterface<IWidgetBuilder> UFigmaGroup::CreateBorderBuilder() const
{
	USizeBoxWidgetBuilder* SizeBoxWidgetBuilder = nullptr;
	if (HasFixedSizing(this) && !ParentUsesFreeLayout(ParentNode))
	{
		SizeBoxWidgetBuilder = NewObject<USizeBoxWidgetBuilder>();
		SizeBoxWidgetBuilder->SetNode(this);
	}

	UBorderWidgetBuilder* BorderWidgetBuilder = NewObject<UBorderWidgetBuilder>();
	BorderWidgetBuilder->SetNode(this);
	if (SizeBoxWidgetBuilder)
	{
		SizeBoxWidgetBuilder->SetChild(BorderWidgetBuilder);
	}

	TArray<TScriptInterface<IWidgetBuilder>> ChildBuilders;
	ChildBuilders.Reserve(Children.Num());
	for (const UFigmaNode* Child : Children)
	{
		if (Child == nullptr)
		{
			continue;
		}

		if (TScriptInterface<IWidgetBuilder> SubBuilder = Child->CreateWidgetBuilders())
		{
			ChildBuilders.Add(SubBuilder);
		}
	}

	if (ChildBuilders.Num() == 1)
	{
		BorderWidgetBuilder->SetChild(ChildBuilders[0]);
	}
	else if (ChildBuilders.Num() > 1)
	{
		UPanelWidgetBuilder* PanelWidgetBuilder = CreatePanelBuilderForLayout();
		PanelWidgetBuilder->SetNode(this);
		PanelWidgetBuilder->SetWidgetNameOverride(MakeContentWidgetName(this));

		for (const TScriptInterface<IWidgetBuilder>& ChildBuilder : ChildBuilders)
		{
			PanelWidgetBuilder->AddChild(ChildBuilder);
		}

		BorderWidgetBuilder->SetChild(PanelWidgetBuilder);
	}

	if (SizeBoxWidgetBuilder)
	{
		return SizeBoxWidgetBuilder;
	}

	return BorderWidgetBuilder;
}

TScriptInterface<IWidgetBuilder> UFigmaGroup::CreateContainersBuilder(EFigmaUMGWidgetRole ForcedRole, bool bForceBorder, bool bSuppressVisualWrappers, const FString& PanelNameOverride) const
{
	USizeBoxWidgetBuilder* SizeBoxWidgetBuilder = nullptr;
	UBorderWidgetBuilder* BorderWidgetBuilder = nullptr;
	UPanelWidgetBuilder* PanelWidgetBuilder = nullptr;
	if (!bSuppressVisualWrappers && HasFixedSizing(this) && !ParentUsesFreeLayout(ParentNode))
	{
		SizeBoxWidgetBuilder = NewObject<USizeBoxWidgetBuilder>();
		SizeBoxWidgetBuilder->SetNode(this);
	}

	bool RequireBorder = false;
	if (bForceBorder)
	{
		RequireBorder = true;
	}
	else if (!bSuppressVisualWrappers && (!ParentNode || !ParentNode->IsA<UFigmaComponentSet>()))
	{
		for (int i = 0; i < Fills.Num() && !RequireBorder; i++)
		{
			if (Fills[i].Visible)
				RequireBorder = true;
		}
		for (int i = 0; i < Strokes.Num() && !RequireBorder; i++)
		{
			if (Strokes[i].Visible)
				RequireBorder = true;
		}
	}

	if (RequireBorder)
	{
		BorderWidgetBuilder = NewObject<UBorderWidgetBuilder>();
		BorderWidgetBuilder->SetNode(this);
		if (SizeBoxWidgetBuilder)
		{
			SizeBoxWidgetBuilder->SetChild(BorderWidgetBuilder);
		}
	}

	PanelWidgetBuilder = CreatePanelBuilderForRole(ForcedRole);
	if (PanelWidgetBuilder == nullptr)
	{
		PanelWidgetBuilder = CreatePanelBuilderForLayout();
	}

	PanelWidgetBuilder->SetNode(this);
	PanelWidgetBuilder->SetWidgetNameOverride(PanelNameOverride);
	int32 AddedChildrenCount = 0;
	for (const UFigmaNode* Child : Children)
	{
		if (TScriptInterface<IWidgetBuilder> SubBuilder = Child->CreateWidgetBuilders())
		{
			PanelWidgetBuilder->AddChild(SubBuilder);
			++AddedChildrenCount;
		}
	}

	if (AddedChildrenCount == 0 && BorderWidgetBuilder && ForcedRole == EFigmaUMGWidgetRole::Auto)
	{
		if (SizeBoxWidgetBuilder)
		{
			SizeBoxWidgetBuilder->SetChild(BorderWidgetBuilder);
			return SizeBoxWidgetBuilder;
		}

		return BorderWidgetBuilder;
	}

	if (BorderWidgetBuilder)
	{
		BorderWidgetBuilder->SetChild(PanelWidgetBuilder);
	}
	else if (SizeBoxWidgetBuilder)
	{
		SizeBoxWidgetBuilder->SetChild(PanelWidgetBuilder);
	}

	if (SizeBoxWidgetBuilder)
	{
		return SizeBoxWidgetBuilder;
	}

	if (BorderWidgetBuilder)
	{
		return BorderWidgetBuilder;
	}

	return PanelWidgetBuilder;
}

UPanelWidgetBuilder* UFigmaGroup::CreatePanelBuilderForRole(EFigmaUMGWidgetRole Role) const
{
	switch (Role)
	{
	case EFigmaUMGWidgetRole::Canvas:
		return NewObject<UCanvasBuilder>();
	case EFigmaUMGWidgetRole::HBox:
		return NewObject<UHBoxBuilder>();
	case EFigmaUMGWidgetRole::VBox:
		return NewObject<UVBoxBuilder>();
	case EFigmaUMGWidgetRole::WrapBox:
		return NewObject<UWBoxBuilder>();
	case EFigmaUMGWidgetRole::Panel:
	case EFigmaUMGWidgetRole::Decor:
	case EFigmaUMGWidgetRole::Auto:
	default:
		return nullptr;
	}
}

UPanelWidgetBuilder* UFigmaGroup::CreatePanelBuilderForLayout() const
{
	switch (LayoutMode)
	{
	case EFigmaLayoutMode::NONE:
		return NewObject<UCanvasBuilder>();
	case EFigmaLayoutMode::HORIZONTAL:
		return LayoutWrap == EFigmaLayoutWrap::NO_WRAP ? static_cast<UPanelWidgetBuilder*>(NewObject<UHBoxBuilder>()) : static_cast<UPanelWidgetBuilder*>(NewObject<UWBoxBuilder>());
	case EFigmaLayoutMode::VERTICAL:
		return LayoutWrap == EFigmaLayoutWrap::NO_WRAP ? static_cast<UPanelWidgetBuilder*>(NewObject<UVBoxBuilder>()) : static_cast<UPanelWidgetBuilder*>(NewObject<UWBoxBuilder>());
	default:
		return NewObject<UCanvasBuilder>();
	}
}

void UFigmaGroup::FixSpacers(const TObjectPtr<UPanelWidget>& PanelWidget) const
{
	if (!PanelWidget)
		return;

	if (PanelWidget->IsA<UCanvasPanel>() || PanelWidget->IsA<UWrapBox>())
	{
		for (int i = 0; i < PanelWidget->GetChildrenCount(); i++)
		{
			UWidget* Widget = PanelWidget->GetChildAt(i);
			if (!Widget || Widget->IsA<USpacer>())
			{
				PanelWidget->RemoveChildAt(i);
				i--;
			}
		}
		if (UWrapBox* WrapBox = Cast<UWrapBox>(PanelWidget))
		{
			WrapBox->SetInnerSlotPadding(FVector2D(ItemSpacing, CounterAxisSpacing));
		}
	}
	else
	{
		for (int i = 0; i < PanelWidget->GetChildrenCount(); i++)
		{
			UWidget* Widget = PanelWidget->GetChildAt(i);
			const bool ShouldBeSpacer = (((i + 1) % 2) == 0);
			const bool IsSpacer = Widget && Widget->IsA<USpacer>();
			if (!Widget || (IsSpacer && !ShouldBeSpacer))
			{
				PanelWidget->RemoveChildAt(i);
				i--;
			}
			else if (ShouldBeSpacer && !IsSpacer)
			{
				USpacer* Spacer = NewObject<USpacer>(PanelWidget->GetOuter());
				Spacer->SetSize(FVector2D(ItemSpacing, ItemSpacing));
				PanelWidget->InsertChildAt(i, Spacer);
			}
			else if (ShouldBeSpacer && IsSpacer)
			{
				USpacer* Spacer = Cast<USpacer>(Widget);
				Spacer->SetSize(FVector2D(ItemSpacing, ItemSpacing));
			}
		}
	}

}

// MIT License
// Copyright (c) 2024 Buvi Games


#include "Parser/Nodes/Vectors/FigmaVectorNode.h"

#include "Builder/Asset/MaterialBuilder.h"
#include "Builder/Asset/Texture2DBuilder.h"
#include "Builder/Widget/ImageWidgetBuilder.h"
#include "Figma2UMGModule.h"
#include "Parser/Properties/FigmaAction.h"
#include "Parser/Properties/FigmaTrigger.h"
#include "Parser/Properties/FigmaUMGSemanticName.h"
#include "REST/FigmaImporter.h"

void UFigmaVectorNode::PostSerialize(const TObjectPtr<UFigmaNode> InParent, const TSharedRef<FJsonObject> JsonObj)
{
	Super::PostSerialize(InParent, JsonObj);

	PostSerializeProperty(JsonObj, "fills", Fills);
	PostSerializeProperty(JsonObj, "strokes", Strokes);
}

FVector2D UFigmaVectorNode::GetAbsolutePosition(const bool IsTopWidgetForNode) const
{
	if (AssetBuilder)
	{
		return AbsoluteRenderBounds.GetPosition(IsTopWidgetForNode ? GetAbsoluteRotation() : 0.0f);
	}
	else
	{
		return AbsoluteBoundingBox.GetPosition(IsTopWidgetForNode ? GetAbsoluteRotation() : 0.0f);
	}
}

FVector2D UFigmaVectorNode::GetAbsoluteSize(const bool IsTopWidgetForNode) const
{
	if (AssetBuilder)
	{
		return AbsoluteRenderBounds.GetSize(IsTopWidgetForNode ? GetAbsoluteRotation() : 0.0f);	
	}
	else
	{
		return AbsoluteBoundingBox.GetSize(IsTopWidgetForNode ? GetAbsoluteRotation() : 0.0f);	
	}
}

FVector2D UFigmaVectorNode::GetAbsoluteCenter() const
{
	if (AssetBuilder)
	{
		return AbsoluteRenderBounds.GetCenter();
	}
	else
	{
		return AbsoluteBoundingBox.GetCenter();
	}
}

bool UFigmaVectorNode::CreateAssetBuilder(const FString& InFileKey, TArray<TScriptInterface<IAssetBuilder>>& AssetBuilders)
{
	if (HasProjectTextureReferencePrefix())
	{
		return false;
	}

	AssetBuilder = NewObject<UTexture2DBuilder>();
	AssetBuilder->SetNode(InFileKey, this);
	AssetBuilders.Add(AssetBuilder);

	if (HasTextureOnlyImagePrefix())
	{
		return true;
	}

	if (DoesSupportImageRef())
	{
		CreatePaintAssetBuilderIfNeeded(InFileKey, AssetBuilders, Fills, Strokes);
	}

	return true;
}

FString UFigmaVectorNode::GetPackageNameForBuilder(const TScriptInterface<IAssetBuilder>& InAssetBuilder) const
{
	TObjectPtr<UFigmaNode> TopParentNode = ParentNode;
	while (TopParentNode && TopParentNode->GetParentNode())
	{
		TopParentNode = TopParentNode->GetParentNode();
	}

	FString Suffix = "Textures";
	if (Cast<UMaterialBuilder>(InAssetBuilder.GetObject()))
	{
		Suffix = "Materials";
	}

	return TopParentNode->GetCurrentPackagePath() + TEXT("/") + Suffix;
}

TScriptInterface<IWidgetBuilder> UFigmaVectorNode::CreateWidgetBuilders(bool IsRoot/*= false*/, bool AllowFrameButton/*= true*/) const
{
	if (HasTextureOnlyImagePrefix())
	{
		return nullptr;
	}

	const FFigmaUMGSemanticName SemanticName = GetUMGSemanticName();
	if (SemanticName.Role == EFigmaUMGWidgetRole::Ignore)
	{
		return nullptr;
	}

	if (SemanticName.Role == EFigmaUMGWidgetRole::Text || SemanticName.Role == EFigmaUMGWidgetRole::RichText)
	{
		UE_LOG_Figma2UMG(Warning, TEXT("[UFigmaVectorNode::CreateWidgetBuilders] VECTOR node %s uses the %s widget prefix. Falling back to image import."), *GetNodeName(), LexToString(SemanticName.Role));
	}
	else if (SemanticName.Role != EFigmaUMGWidgetRole::Auto
		&& SemanticName.Role != EFigmaUMGWidgetRole::Image
		&& SemanticName.Role != EFigmaUMGWidgetRole::Decor)
	{
		UE_LOG_Figma2UMG(Warning, TEXT("[UFigmaVectorNode::CreateWidgetBuilders] VECTOR node %s uses the %s widget prefix. Falling back to image import."), *GetNodeName(), LexToString(SemanticName.Role));
	}

	UImageWidgetBuilder* ImageWidgetBuilder = NewObject<UImageWidgetBuilder>();
	ImageWidgetBuilder->SetNode(this);
	if (HasImageWidgetPrefix())
	{
		const FString TextureName = GetImageWidgetTextureAssetName();
		if (UTexture2D* Texture = FindProjectTextureByName(TextureName))
		{
			ImageWidgetBuilder->SetTexture(Texture);
			return ImageWidgetBuilder;
		}

		if (!TextureName.IsEmpty())
		{
			UE_LOG_Figma2UMG(Warning, TEXT("[UFigmaVectorNode::CreateWidgetBuilders] IMG_ node %s references project texture %s from its own name, but no matching UTexture2D was found."), *GetNodeName(), *TextureName);
		}
	}
	ImageWidgetBuilder->SetTexture2DBuilder(AssetBuilder);

	return ImageWidgetBuilder;
}

const FFigmaInteraction& UFigmaVectorNode::GetInteractionFromTrigger(const EFigmaTriggerType TriggerType) const
{
	return UFigmaNode::GetInteractionFromTrigger(Interactions, TriggerType);
}

const FFigmaInteraction& UFigmaVectorNode::GetInteractionFromAction(const EFigmaActionType ActionType, const EFigmaActionNodeNavigation Navigation) const
{
	return UFigmaNode::GetInteractionFromAction(Interactions, ActionType, Navigation);
}

const FString& UFigmaVectorNode::GetDestinationIdFromEvent(const FName& EventName) const
{
	const FFigmaInteraction& Interaction = GetInteractionFromAction(EFigmaActionType::NODE, EFigmaActionNodeNavigation::NAVIGATE);
	if (!Interaction.Trigger || !Interaction.Trigger->MatchEvent(EventName.ToString()))
		return TransitionNodeID;

	const UFigmaNodeAction* Action = Interaction.FindActionNode(EFigmaActionNodeNavigation::NAVIGATE);
	if (!Action || Action->DestinationId.IsEmpty())
		return TransitionNodeID;

	return Action->DestinationId;
}

bool UFigmaVectorNode::DoesSupportImageRef() const
{
	return false;
}

bool UFigmaVectorNode::ShouldIgnoreRotation() const
{
	switch (GetType())
	{
		case ENodeTypes::VECTOR:
		case ENodeTypes::STAR:
		case ENodeTypes::LINE:
		case ENodeTypes::REGULAR_POLYGON:
			return true;
		default:
			return false;
	}
}

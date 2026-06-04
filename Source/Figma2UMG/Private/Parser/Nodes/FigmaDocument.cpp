// MIT License
// Copyright (c) 2024 Buvi Games


#include "Parser/Nodes/FigmaDocument.h"

#include "Figma2UMGModule.h"
#include "Builder/Asset/WidgetBlueprintBuilder.h"
#include "Builder/Widget/PanelWidgetBuilder.h"
#include "Builder/Widget/WidgetSwitcherBuilder.h"
#include "Parser/FigmaFile.h"
#include "Parser/Properties/FigmaUMGSemanticName.h"

namespace
{
	void AppendValidChildren(const TArray<TObjectPtr<UFigmaNode>>& Children, TArray<const UFigmaNode*>& OutNodes)
	{
		for (const UFigmaNode* Child : Children)
		{
			if (Child)
			{
				OutNodes.AddUnique(Child);
			}
		}
	}

	TScriptInterface<IWidgetBuilder> CreateRootWidgetBuildersFromNodes(const UFigmaNode* SwitcherNode, const TArray<const UFigmaNode*>& RootNodes, const bool bNodesAreRootWidgets)
	{
		if (RootNodes.Num() > 1)
		{
			UWidgetSwitcherBuilder* Builder = NewObject<UWidgetSwitcherBuilder>();
			Builder->SetNode(SwitcherNode);

			for (const UFigmaNode* RootNode : RootNodes)
			{
				TScriptInterface<IWidgetBuilder> SubBuilder = RootNode->CreateWidgetBuilders(bNodesAreRootWidgets);
				if (SubBuilder)
				{
					Builder->AddChild(SubBuilder);
				}
			}

			return Builder;
		}

		if (RootNodes.Num() == 1)
		{
			return RootNodes[0]->CreateWidgetBuilders(bNodesAreRootWidgets);
		}

		return nullptr;
	}

	TScriptInterface<IWidgetBuilder> CreateRootWidgetBuildersFromChildren(const UFigmaNode* SwitcherNode, const TArray<TObjectPtr<UFigmaNode>>& Children, const bool bChildrenAreRootWidgets)
	{
		TArray<const UFigmaNode*> RootNodes;
		RootNodes.Reserve(Children.Num());
		AppendValidChildren(Children, RootNodes);

		return CreateRootWidgetBuildersFromNodes(SwitcherNode, RootNodes, bChildrenAreRootWidgets);
	}
}

void UFigmaDocument::SetFigmaFile(UFigmaFile* InFigmaFile)
{
	FigmaFile = InFigmaFile;
	SetCurrentPackagePath(FigmaFile->GetPackagePath());
}

bool UFigmaDocument::CreateAssetBuilder(const FString& InFileKey, TArray<TScriptInterface<IAssetBuilder>>& AssetBuilders)
{
	UWidgetBlueprintBuilder* AssetBuilder = NewObject<UWidgetBlueprintBuilder>();
	AssetBuilder->SetNode(InFileKey, this);
	AssetBuilders.Add(AssetBuilder);

	return true;
}

FString UFigmaDocument::GetPackageNameForBuilder(const TScriptInterface<IAssetBuilder>& InAssetBuilder) const
{
	return PackagePath;
}

FString UFigmaDocument::GetUAssetName() const
{
	return FigmaFile ? FigmaFile->GetUAssetName() : FString();
}

TScriptInterface<IWidgetBuilder> UFigmaDocument::CreateWidgetBuilders(bool IsRoot /*= false*/, bool AllowFrameButton/*= true*/) const
{
	if (FigmaFile)
	{
		if (TObjectPtr<UFigmaNode> PrimaryImportNode = FigmaFile->GetPrimaryImportNode())
		{
			TArray<const UFigmaNode*> RootNodes;
			const FFigmaUMGSemanticName PrimarySemanticName = PrimaryImportNode->GetUMGSemanticName();
			if (PrimarySemanticName.bHasWidgetPrefix && PrimarySemanticName.Role != EFigmaUMGWidgetRole::Ignore)
			{
				RootNodes.Add(PrimaryImportNode);
			}
			else if (const IFigmaContainer* ImportRootContainer = Cast<IFigmaContainer>(PrimaryImportNode))
			{
				const TArray<TObjectPtr<UFigmaNode>>& RootChildren = ImportRootContainer->GetChildrenConst();
				if (!RootChildren.IsEmpty())
				{
					AppendValidChildren(RootChildren, RootNodes);
				}
			}

			if (RootNodes.IsEmpty())
			{
				UE_LOG_Figma2UMG(Warning, TEXT("Figma node %s has no direct child nodes. Falling back to importing the selected node itself as root content."), *PrimaryImportNode->GetId());
				RootNodes.Add(PrimaryImportNode);
			}

			TArray<TObjectPtr<UFigmaNode>> AdditionalImportNodes;
			FigmaFile->GetAdditionalImportNodes(AdditionalImportNodes);
			for (const UFigmaNode* AdditionalImportNode : AdditionalImportNodes)
			{
				if (AdditionalImportNode)
				{
					RootNodes.AddUnique(AdditionalImportNode);
				}
			}

			return CreateRootWidgetBuildersFromNodes(PrimaryImportNode, RootNodes, true);
		}
	}

	return CreateRootWidgetBuildersFromChildren(this, Children, false);
}

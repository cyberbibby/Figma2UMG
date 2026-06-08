// MIT License
// Copyright (c) 2024 Buvi Games


#include "Parser/Nodes/FigmaFrame.h"

#include "Builder/Asset/MaterialBuilder.h"
#include "Builder/Asset/Texture2DBuilder.h"

TScriptInterface<IWidgetBuilder> UFigmaFrame::CreateWidgetBuilders(bool IsRoot/*= false*/, bool AllowFrameButton/*= true*/) const
{
	return Super::CreateWidgetBuilders(IsRoot, AllowFrameButton);
}

bool UFigmaFrame::CreateAssetBuilder(const FString& InFileKey, TArray<TScriptInterface<IAssetBuilder>>& AssetBuilders)
{
	if (GetUMGSemanticName().Role == EFigmaUMGWidgetRole::Ignore)
	{
		return false;
	}

	return Super::CreateAssetBuilder(InFileKey, AssetBuilders);
}

FString UFigmaFrame::GetPackageNameForBuilder(const TScriptInterface<IAssetBuilder>& InAssetBuilder) const
{
	TObjectPtr<UFigmaNode> TopParentNode = ParentNode;
	while (TopParentNode && TopParentNode->GetParentNode())
	{
		TopParentNode = TopParentNode->GetParentNode();
	}

	FString Suffix = "Menu";
	if (Cast<UMaterialBuilder>(InAssetBuilder.GetObject()))
	{
		Suffix = "Materials";
	}
	else if (Cast<UTexture2DBuilder>(InAssetBuilder.GetObject()))
	{
		Suffix = "Textures";
	}

	return TopParentNode->GetCurrentPackagePath() + TEXT("/") + Suffix;
}

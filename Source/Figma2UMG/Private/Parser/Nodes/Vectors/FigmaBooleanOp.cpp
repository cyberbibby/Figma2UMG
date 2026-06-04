// MIT License
// Copyright (c) 2024 Buvi Games


#include "Parser/Nodes/Vectors/FigmaBooleanOp.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Builder/Asset/Texture2DBuilder.h"
#include "Builder/Widget/ButtonWidgetBuilder.h"
#include "Builder/Widget/GenericWidgetBuilder.h"
#include "Builder/Widget/ImageWidgetBuilder.h"
#include "Engine/Texture2D.h"
#include "Figma2UMGModule.h"
#include "Parser/Nodes/FigmaGroup.h"
#include "Parser/Nodes/FigmaInstance.h"
#include "Parser/Nodes/FigmaSection.h"
#include "Parser/Properties/FigmaUMGSemanticName.h"

namespace
{
	UTexture2D* FindProjectTextureByNodeName(const FString& TextureName)
	{
		if (TextureName.IsEmpty())
		{
			return nullptr;
		}

		FARFilter Filter;
		Filter.ClassPaths.Add(UTexture2D::StaticClass()->GetClassPathName());
		Filter.PackagePaths.Add(FName(TEXT("/Game")));
		Filter.bRecursivePaths = true;

		TArray<FAssetData> AssetDataList;
		const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		AssetRegistryModule.Get().GetAssets(Filter, AssetDataList);

		for (const FAssetData& AssetData : AssetDataList)
		{
			if (AssetData.AssetName.ToString().Equals(TextureName, ESearchCase::CaseSensitive))
			{
				return Cast<UTexture2D>(AssetData.FastGetAsset(true));
			}
		}

		return nullptr;
	}

	UTexture2DBuilder* GetTextureOnlyImageBuilder(const UFigmaNode* Node)
	{
		if (!Node || !Node->HasTextureOnlyImagePrefix())
		{
			return nullptr;
		}

		if (const UFigmaGroup* Group = Cast<UFigmaGroup>(Node))
		{
			return Group->Texture2DBuilder;
		}
		if (const UFigmaSection* Section = Cast<UFigmaSection>(Node))
		{
			return Section->Texture2DBuilder;
		}
		if (const UFigmaInstance* Instance = Cast<UFigmaInstance>(Node))
		{
			return Instance->GetTexture2DBuilder();
		}
		if (const UFigmaVectorNode* VectorNode = Cast<UFigmaVectorNode>(Node))
		{
			return VectorNode->GetTexture2DBuilder();
		}

		return nullptr;
	}

	UImageWidgetBuilder* CreateImageBuilderFromTextureSource(const UFigmaNode* OwnerNode, const TArray<TObjectPtr<UFigmaNode>>& Children)
	{
		for (const UFigmaNode* Child : Children)
		{
			if (!Child)
			{
				continue;
			}

			UImageWidgetBuilder* ImageBuilder = NewObject<UImageWidgetBuilder>();
			ImageBuilder->SetNode(Child);

			if (Child->HasProjectTextureReferencePrefix())
			{
				if (UTexture2D* Texture = FindProjectTextureByNodeName(Child->GetNodeName()))
				{
					ImageBuilder->SetTexture(Texture);
					return ImageBuilder;
				}

				UE_LOG_Figma2UMG(Warning, TEXT("[UFigmaBooleanOp] Node %s references project texture %s, but no matching UTexture2D was found."), *OwnerNode->GetNodeName(), *Child->GetNodeName());
			}
			else if (Child->HasTextureOnlyImagePrefix())
			{
				if (UTexture2DBuilder* TextureBuilder = GetTextureOnlyImageBuilder(Child))
				{
					ImageBuilder->SetTexture2DBuilder(TextureBuilder);
					return ImageBuilder;
				}

				UE_LOG_Figma2UMG(Warning, TEXT("[UFigmaBooleanOp] Node %s uses texture-only child %s, but no texture builder was created."), *OwnerNode->GetNodeName(), *Child->GetNodeName());
			}
		}

		return nullptr;
	}

	struct FVisualTextureSource
	{
		const UFigmaNode* Node = nullptr;
		UTexture2DBuilder* TextureBuilder = nullptr;
		UTexture2D* Texture = nullptr;
	};

	bool TryGetVisualTextureSource(const UFigmaNode* OwnerNode, const UFigmaNode* Child, FVisualTextureSource& OutSource)
	{
		if (!OwnerNode || !Child)
		{
			return false;
		}

		if (Child->HasProjectTextureReferencePrefix())
		{
			if (UTexture2D* Texture = FindProjectTextureByNodeName(Child->GetNodeName()))
			{
				OutSource.Node = Child;
				OutSource.Texture = Texture;
				return true;
			}

			UE_LOG_Figma2UMG(Warning, TEXT("[UFigmaBooleanOp] Node %s references project texture %s, but no matching UTexture2D was found."), *OwnerNode->GetNodeName(), *Child->GetNodeName());
			return false;
		}

		if (Child->HasTextureOnlyImagePrefix())
		{
			if (UTexture2DBuilder* TextureBuilder = GetTextureOnlyImageBuilder(Child))
			{
				OutSource.Node = Child;
				OutSource.TextureBuilder = TextureBuilder;
				return true;
			}

			UE_LOG_Figma2UMG(Warning, TEXT("[UFigmaBooleanOp] Node %s uses texture-only child %s, but no texture builder was created."), *OwnerNode->GetNodeName(), *Child->GetNodeName());
		}

		return false;
	}

	template<typename WidgetBuilderT>
	bool ApplyVisualTextureFromChildren(WidgetBuilderT* WidgetBuilder, const UFigmaNode* OwnerNode, const TArray<TObjectPtr<UFigmaNode>>& Children)
	{
		if (!WidgetBuilder || !OwnerNode)
		{
			return false;
		}

		for (const UFigmaNode* Child : Children)
		{
			if (!Child)
			{
				continue;
			}

			FVisualTextureSource Source;
			if (TryGetVisualTextureSource(OwnerNode, Child, Source))
			{
				if (Source.TextureBuilder)
				{
					WidgetBuilder->SetVisualTexture2DBuilder(Source.TextureBuilder);
				}
				else
				{
					WidgetBuilder->SetVisualTexture(Source.Texture);
				}
				return true;
			}
		}

		return false;
	}

	void ApplyCheckBoxVisualTextureSource(UGenericLeafWidgetBuilder* WidgetBuilder, const FVisualTextureSource& Source, bool bChecked)
	{
		if (!WidgetBuilder)
		{
			return;
		}

		if (bChecked)
		{
			if (Source.TextureBuilder)
			{
				WidgetBuilder->SetCheckedVisualTexture2DBuilder(Source.TextureBuilder);
			}
			else
			{
				WidgetBuilder->SetCheckedVisualTexture(Source.Texture);
			}
		}
		else
		{
			if (Source.TextureBuilder)
			{
				WidgetBuilder->SetUncheckedVisualTexture2DBuilder(Source.TextureBuilder);
			}
			else
			{
				WidgetBuilder->SetUncheckedVisualTexture(Source.Texture);
			}
		}
	}

	bool ApplyCheckBoxVisualTexturesFromChildren(UGenericLeafWidgetBuilder* WidgetBuilder, const UFigmaNode* OwnerNode, const TArray<TObjectPtr<UFigmaNode>>& Children)
	{
		if (!WidgetBuilder || !OwnerNode)
		{
			return false;
		}

		TArray<FVisualTextureSource> Sources;
		Sources.Reserve(Children.Num());
		for (const UFigmaNode* Child : Children)
		{
			FVisualTextureSource Source;
			if (TryGetVisualTextureSource(OwnerNode, Child, Source))
			{
				Sources.Add(Source);
			}
		}

		if (Sources.IsEmpty())
		{
			return false;
		}

		if (Sources.Num() == 1)
		{
			if (Sources[0].TextureBuilder)
			{
				WidgetBuilder->SetVisualTexture2DBuilder(Sources[0].TextureBuilder);
			}
			else
			{
				WidgetBuilder->SetVisualTexture(Sources[0].Texture);
			}
			return true;
		}

		int32 CheckedIndex = INDEX_NONE;
		int32 UncheckedIndex = INDEX_NONE;
		for (int32 Index = 0; Index < Sources.Num(); ++Index)
		{
			const FString SourceName = Sources[Index].Node ? Sources[Index].Node->GetNodeName() : FString();
			if (SourceName.Contains(TEXT("Uncheck"), ESearchCase::IgnoreCase))
			{
				if (UncheckedIndex == INDEX_NONE)
				{
					UncheckedIndex = Index;
				}
			}
			else if (SourceName.Contains(TEXT("Check"), ESearchCase::IgnoreCase))
			{
				if (CheckedIndex == INDEX_NONE)
				{
					CheckedIndex = Index;
				}
			}
		}

		if (CheckedIndex == INDEX_NONE || UncheckedIndex == INDEX_NONE)
		{
			CheckedIndex = 0;
			UncheckedIndex = 1;
		}

		ApplyCheckBoxVisualTextureSource(WidgetBuilder, Sources[CheckedIndex], true);
		ApplyCheckBoxVisualTextureSource(WidgetBuilder, Sources[UncheckedIndex], false);
		return true;
	}
}

void UFigmaBooleanOp::PostSerialize(const TObjectPtr<UFigmaNode> InParent, const TSharedRef<FJsonObject> JsonObj)
{
	Super::PostSerialize(InParent, JsonObj);
	SerializeArray(Children, JsonObj, GetJsonArrayName());
}

bool UFigmaBooleanOp::CreateAssetBuilder(const FString& InFileKey, TArray<TScriptInterface<IAssetBuilder>>& AssetBuilders)
{
	if (HasProjectTextureReferencePrefix())
	{
		return false;
	}

	const FFigmaUMGSemanticName SemanticName = GetUMGSemanticName();
	if (SemanticName.Role == EFigmaUMGWidgetRole::Button || SemanticName.Role == EFigmaUMGWidgetRole::CheckBox)
	{
		return false;
	}

	if (HasTextureOnlyImagePrefix())
	{
		return Super::CreateAssetBuilder(InFileKey, AssetBuilders);
	}

	return Super::CreateAssetBuilder(InFileKey, AssetBuilders);
}

TScriptInterface<IWidgetBuilder> UFigmaBooleanOp::CreateWidgetBuilders(bool IsRoot, bool AllowFrameButton) const
{
	const FFigmaUMGSemanticName SemanticName = GetUMGSemanticName();
	if (SemanticName.Role == EFigmaUMGWidgetRole::Ignore || HasTextureOnlyImagePrefix() || HasProjectTextureReferencePrefix())
	{
		return nullptr;
	}

	if (SemanticName.Role == EFigmaUMGWidgetRole::Button)
	{
		UButtonWidgetBuilder* ButtonBuilder = NewObject<UButtonWidgetBuilder>();
		ButtonBuilder->SetNode(this);
		ApplyVisualTextureFromChildren(ButtonBuilder, this, Children);
		return ButtonBuilder;
	}

	if (SemanticName.Role == EFigmaUMGWidgetRole::CheckBox)
	{
		UGenericLeafWidgetBuilder* CheckBoxBuilder = NewObject<UGenericLeafWidgetBuilder>();
		CheckBoxBuilder->SetNode(this);
		CheckBoxBuilder->SetWidgetType(EFigmaUMGWidgetType::CheckBox);
		ApplyCheckBoxVisualTexturesFromChildren(CheckBoxBuilder, this, Children);
		return CheckBoxBuilder;
	}

	if (SemanticName.Role == EFigmaUMGWidgetRole::Image || HasImageWidgetPrefix())
	{
		if (UImageWidgetBuilder* ImageBuilder = CreateImageBuilderFromTextureSource(this, Children))
		{
			ImageBuilder->SetNode(this);
			return ImageBuilder;
		}
		return nullptr;
	}

	return nullptr;
}

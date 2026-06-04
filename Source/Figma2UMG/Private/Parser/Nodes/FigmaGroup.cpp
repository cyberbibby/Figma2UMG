// MIT License
// Copyright (c) 2024 Buvi Games


#include "Parser/Nodes/FigmaGroup.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Parser/Nodes/FigmaComponentSet.h"
#include "Parser/Nodes/FigmaCanvas.h"
#include "Parser/Nodes/FigmaInstance.h"
#include "Parser/Nodes/FigmaSection.h"
#include "Parser/Nodes/Vectors/FigmaVectorNode.h"
#include "Builder/Asset/MaterialBuilder.h"
#include "Builder/Asset/Texture2DBuilder.h"
#include "Builder/Asset/WidgetBlueprintBuilder.h"
#include "Builder/Widget/BorderWidgetBuilder.h"
#include "Builder/Widget/ButtonWidgetBuilder.h"
#include "Builder/Widget/ComboBoxStringWidgetBuilder.h"
#include "Builder/Widget/GenericWidgetBuilder.h"
#include "Builder/Widget/ImageWidgetBuilder.h"
#include "Builder/Widget/PanelWidgetBuilder.h"
#include "Builder/Widget/ProgressBarWidgetBuilder.h"
#include "Builder/Widget/SizeBoxWidgetBuilder.h"
#include "Builder/Widget/UserWidgetBuilder.h"
#include "Builder/Widget/WidgetBuilder.h"
#include "Builder/Widget/WidgetSwitcherBuilder.h"
#include "Builder/Widget/Panels/CanvasBuilder.h"
#include "Builder/Widget/Panels/HBoxBuilder.h"
#include "Builder/Widget/Panels/VBoxBuilder.h"
#include "Builder/Widget/Panels/WBoxBuilder.h"
#include "Components/CanvasPanel.h"
#include "Components/Spacer.h"
#include "Components/WrapBox.h"
#include "Engine/Texture2D.h"
#include "Figma2UMGModule.h"
#include "Parser/Properties/FigmaAction.h"
#include "Parser/Properties/FigmaTrigger.h"
#include "Parser/Properties/FigmaUMGSemanticName.h"
#include "Parser/Nodes/Vectors/FigmaText.h"
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

	bool ApplyImageSourceFromChildren(UImageWidgetBuilder* ImageBuilder, const UFigmaGroup* Group)
	{
		if (!ImageBuilder || !Group)
		{
			return false;
		}

		for (const UFigmaNode* Child : Group->Children)
		{
			if (!Child)
			{
				continue;
			}

			if (Child->HasProjectTextureReferencePrefix())
			{
				if (UTexture2D* Texture = FindProjectTextureByNodeName(Child->GetNodeName()))
				{
					ImageBuilder->SetTexture(Texture);
					return true;
				}

				UE_LOG_Figma2UMG(Warning, TEXT("[UFigmaGroup::ApplyImageSourceFromChildren] Node %s references project texture %s, but no matching UTexture2D was found."), *Group->GetNodeName(), *Child->GetNodeName());
				continue;
			}

			if (Child->HasTextureOnlyImagePrefix())
			{
				if (UTexture2DBuilder* TextureBuilder = GetTextureOnlyImageBuilder(Child))
				{
					ImageBuilder->SetTexture2DBuilder(TextureBuilder);
					return true;
				}

				UE_LOG_Figma2UMG(Warning, TEXT("[UFigmaGroup::ApplyImageSourceFromChildren] Node %s uses texture-only child %s, but no texture builder was created."), *Group->GetNodeName(), *Child->GetNodeName());
			}
		}

		for (const FFigmaPaint& Fill : Group->Fills)
		{
			if (!Fill.Visible)
			{
				continue;
			}

			if (UTexture2D* Texture = Fill.GetTexture())
			{
				ImageBuilder->SetTexture(Texture);
				return true;
			}

			if (UMaterialInterface* Material = Fill.GetMaterial())
			{
				ImageBuilder->SetMaterial(Material, Fill.GetLinearColor());
				return true;
			}
		}

		return false;
	}

	bool IsTextureSourceNode(const UFigmaNode* Node)
	{
		return Node && (Node->HasProjectTextureReferencePrefix() || Node->HasTextureOnlyImagePrefix());
	}

	struct FVisualTextureSource
	{
		const UFigmaNode* Node = nullptr;
		UTexture2DBuilder* TextureBuilder = nullptr;
		UTexture2D* Texture = nullptr;
	};

	bool TryGetVisualTextureSource(const UFigmaGroup* Group, const UFigmaNode* Child, FVisualTextureSource& OutSource)
	{
		if (!Group || !Child)
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

			UE_LOG_Figma2UMG(Warning, TEXT("[UFigmaGroup::TryGetVisualTextureSource] Node %s references project texture %s, but no matching UTexture2D was found."), *Group->GetNodeName(), *Child->GetNodeName());
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

			UE_LOG_Figma2UMG(Warning, TEXT("[UFigmaGroup::TryGetVisualTextureSource] Node %s uses texture-only child %s, but no texture builder was created."), *Group->GetNodeName(), *Child->GetNodeName());
		}

		return false;
	}

	bool TryGetVisualTextureSourceRecursive(const UFigmaGroup* Group, const UFigmaNode* Node, FVisualTextureSource& OutSource)
	{
		if (TryGetVisualTextureSource(Group, Node, OutSource))
		{
			return true;
		}

		if (const UFigmaGroup* ChildGroup = Cast<UFigmaGroup>(Node))
		{
			if (ChildGroup->Texture2DBuilder)
			{
				OutSource.Node = ChildGroup;
				OutSource.TextureBuilder = ChildGroup->Texture2DBuilder;
				return true;
			}

			for (const FFigmaPaint& Fill : ChildGroup->Fills)
			{
				if (Fill.Visible)
				{
					if (UTexture2D* Texture = Fill.GetTexture())
					{
						OutSource.Node = ChildGroup;
						OutSource.Texture = Texture;
						return true;
					}
				}
			}
		}
		else if (const UFigmaSection* ChildSection = Cast<UFigmaSection>(Node))
		{
			if (ChildSection->Texture2DBuilder)
			{
				OutSource.Node = ChildSection;
				OutSource.TextureBuilder = ChildSection->Texture2DBuilder;
				return true;
			}
		}
		else if (const UFigmaInstance* ChildInstance = Cast<UFigmaInstance>(Node))
		{
			if (UTexture2DBuilder* TextureBuilder = ChildInstance->GetTexture2DBuilder())
			{
				OutSource.Node = ChildInstance;
				OutSource.TextureBuilder = TextureBuilder;
				return true;
			}
		}
		else if (const UFigmaVectorNode* ChildVectorNode = Cast<UFigmaVectorNode>(Node))
		{
			if (UTexture2DBuilder* TextureBuilder = ChildVectorNode->GetTexture2DBuilder())
			{
				OutSource.Node = ChildVectorNode;
				OutSource.TextureBuilder = TextureBuilder;
				return true;
			}
		}

		if (const IFigmaContainer* Container = Cast<IFigmaContainer>(Node))
		{
			for (const UFigmaNode* Child : Container->GetChildrenConst())
			{
				if (TryGetVisualTextureSourceRecursive(Group, Child, OutSource))
				{
					return true;
				}
			}
		}

		return false;
	}

	template<typename WidgetBuilderT>
	bool ApplyVisualTextureFromChildren(WidgetBuilderT* WidgetBuilder, const UFigmaGroup* Group)
	{
		if (!WidgetBuilder || !Group)
		{
			return false;
		}

		for (const UFigmaNode* Child : Group->Children)
		{
			if (!Child)
			{
				continue;
			}

			FVisualTextureSource Source;
			if (TryGetVisualTextureSource(Group, Child, Source))
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

	bool ApplyCheckBoxVisualTexturesFromChildren(UGenericLeafWidgetBuilder* WidgetBuilder, const UFigmaGroup* Group)
	{
		if (!WidgetBuilder || !Group)
		{
			return false;
		}

		TArray<FVisualTextureSource> Sources;
		Sources.Reserve(Group->Children.Num());
		for (const UFigmaNode* Child : Group->Children)
		{
			FVisualTextureSource Source;
			if (TryGetVisualTextureSource(Group, Child, Source))
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

	bool ApplyComboBoxVisualsFromChildren(UComboBoxStringWidgetBuilder* WidgetBuilder, const UFigmaGroup* Group)
	{
		if (!WidgetBuilder || !Group)
		{
			return false;
		}

		bool bAppliedAnyVisual = false;
		bool bHasBackground = false;
		for (const UFigmaNode* Child : Group->Children)
		{
			if (!Child)
			{
				continue;
			}

			if (const UFigmaText* TextNode = Cast<UFigmaText>(Child))
			{
				if (!TextNode->Characters.IsEmpty())
				{
					WidgetBuilder->SetSelectedOption(TextNode->Characters);
					WidgetBuilder->SetSelectedOptionStyle(TextNode->Style);
					if (!TextNode->Fills.IsEmpty())
					{
						WidgetBuilder->SetSelectedOptionColor(TextNode->Fills[0].GetLinearColor());
					}
				}
				continue;
			}

			const FString ChildName = Child->GetNodeName();
			const bool bLooksLikeArrow = ChildName.Contains(TEXT("Arrow"), ESearchCase::IgnoreCase)
				|| ChildName.Contains(TEXT("Aroow"), ESearchCase::IgnoreCase);
			const bool bLooksLikeBackground = ChildName.Contains(TEXT("Bg"), ESearchCase::IgnoreCase)
				|| ChildName.Contains(TEXT("Background"), ESearchCase::IgnoreCase);

			FVisualTextureSource Source;
			if (!TryGetVisualTextureSourceRecursive(Group, Child, Source))
			{
				continue;
			}

			if (bLooksLikeArrow)
			{
				if (Source.TextureBuilder)
				{
					WidgetBuilder->SetArrowTexture2DBuilder(Source.TextureBuilder);
				}
					else
					{
						WidgetBuilder->SetArrowTexture(Source.Texture);
					}
					if (Source.Node)
					{
						const FVector2D ArrowSize = Source.Node->GetAbsoluteSize(true);
						const FVector2D ArrowPosition = Source.Node->GetAbsolutePosition(false) - Group->GetAbsolutePosition(false);
						const FVector2D GroupSize = Group->GetAbsoluteSize(true);
						WidgetBuilder->SetArrowBrushSize(ArrowSize);
						WidgetBuilder->SetArrowPadding(FMargin(
							0.0f,
							FMath::Max(0.0f, ArrowPosition.Y),
							FMath::Max(0.0f, GroupSize.X - ArrowPosition.X - ArrowSize.X),
							FMath::Max(0.0f, GroupSize.Y - ArrowPosition.Y - ArrowSize.Y)));
					}
					bAppliedAnyVisual = true;
					continue;
				}

			if (!bHasBackground || bLooksLikeBackground)
			{
				if (Source.TextureBuilder)
				{
					WidgetBuilder->SetBackgroundTexture2DBuilder(Source.TextureBuilder);
				}
				else
				{
					WidgetBuilder->SetBackgroundTexture(Source.Texture);
				}
				if (Source.Node)
				{
					WidgetBuilder->SetBackgroundBrushSize(Source.Node->GetAbsoluteSize(true));
				}
				bHasBackground = true;
				bAppliedAnyVisual = true;
			}
		}

		return bAppliedAnyVisual;
	}

	bool IsListWidgetType(EFigmaUMGWidgetType WidgetType)
	{
		return WidgetType == EFigmaUMGWidgetType::ListView
			|| WidgetType == EFigmaUMGWidgetType::TileView;
	}

	const UFigmaNode* FindListEntryNode(const TArray<TObjectPtr<UFigmaNode>>& Children)
	{
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

			return Child;
		}

		return nullptr;
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

void UFigmaGroup::SetGenerateFile(bool Value /*= true*/)
{
	GenerateFile = Value;
}

void UFigmaGroup::PostSerialize(const TObjectPtr<UFigmaNode> InParent, const TSharedRef<FJsonObject> JsonObj)
{
	Super::PostSerialize(InParent, JsonObj);

	PostSerializeProperty(JsonObj, "fills", Fills);
	PostSerializeProperty(JsonObj, "strokes", Strokes);
	PostSerializeProperty(JsonObj, "interactions", Interactions);

	if (HasWidgetBlueprintPrefix())
	{
		SetGenerateFile();
	}
}

bool UFigmaGroup::CreateAssetBuilder(const FString& InFileKey, TArray<TScriptInterface<IAssetBuilder>>& AssetBuilders)
{
	if (HasTextureOnlyImagePrefix())
	{
		Texture2DBuilder = NewObject<UTexture2DBuilder>();
		Texture2DBuilder->SetNode(InFileKey, this);
		AssetBuilders.Add(Texture2DBuilder);
		return true;
	}

	const FFigmaUMGSemanticName SemanticName = GetUMGSemanticName();
	if (SemanticName.WidgetType == EFigmaUMGWidgetType::Image)
	{
		if (HasImageWidgetPrefix())
		{
			const int32 AssetBuilderCount = AssetBuilders.Num();
			CreatePaintAssetBuilderIfNeeded(InFileKey, AssetBuilders, Fills, Strokes);
			return AssetBuilders.Num() > AssetBuilderCount;
		}

		Texture2DBuilder = NewObject<UTexture2DBuilder>();
		Texture2DBuilder->SetNode(InFileKey, this);
		AssetBuilders.Add(Texture2DBuilder);
		return true;
	}

	bool bCreated = WidgetBlueprintBuilder != nullptr;
	if (GenerateFile && !WidgetBlueprintBuilder)
	{
		WidgetBlueprintBuilder = NewObject<UWidgetBlueprintBuilder>();
		WidgetBlueprintBuilder->SetNode(InFileKey, this);
		AssetBuilders.Add(WidgetBlueprintBuilder);
		bCreated = true;
	}

	if (IsListWidgetType(SemanticName.WidgetType))
	{
		const UFigmaNode* EntryNode = FindListEntryNode(Children);
		if (EntryNode)
		{
			ListEntryWidgetBlueprintBuilder = NewObject<UWidgetBlueprintBuilder>();
			ListEntryWidgetBlueprintBuilder->SetNode(InFileKey, EntryNode);
			ListEntryWidgetBlueprintBuilder->SetImplementListEntryInterface(true);
			AssetBuilders.Insert(ListEntryWidgetBlueprintBuilder, 0);

			if (const UFigmaGroup* EntryGroup = Cast<UFigmaGroup>(EntryNode))
			{
				UFigmaGroup* MutableEntryGroup = const_cast<UFigmaGroup*>(EntryGroup);
				MutableEntryGroup->SetGenerateFile();
				MutableEntryGroup->WidgetBlueprintBuilder = ListEntryWidgetBlueprintBuilder;
			}
		}
		else
		{
			UE_LOG_Figma2UMG(Warning, TEXT("[UFigmaGroup::CreateAssetBuilder] Node %s is a %s but has no valid direct child to use as EntryWidgetClass."), *GetNodeName(), LexToString(SemanticName.WidgetType));
		}
	}

	CreatePaintAssetBuilderIfNeeded(InFileKey, AssetBuilders, Fills, Strokes);

	return Super::CreateAssetBuilder(InFileKey, AssetBuilders) || bCreated;
}

FString UFigmaGroup::GetPackageNameForBuilder(const TScriptInterface<IAssetBuilder>& InAssetBuilder) const
{
	if (Cast<UMaterialBuilder>(InAssetBuilder.GetObject()) || Cast<UTexture2DBuilder>(InAssetBuilder.GetObject()) || Cast<UWidgetBlueprintBuilder>(InAssetBuilder.GetObject()))
	{
		TObjectPtr<UFigmaNode> TopParentNode = ParentNode;
		while (TopParentNode && TopParentNode->GetParentNode())
		{
			TopParentNode = TopParentNode->GetParentNode();
		}

		FString Suffix = TEXT("Menu");
		if (Cast<UMaterialBuilder>(InAssetBuilder.GetObject()))
		{
			Suffix = TEXT("Material");
		}
		else if (Cast<UTexture2DBuilder>(InAssetBuilder.GetObject()))
		{
			Suffix = TEXT("Textures");
		}
		return TopParentNode->GetCurrentPackagePath() + TEXT("/") + Suffix;
	}

	return Super::GetPackageNameForBuilder(InAssetBuilder);
}

TScriptInterface<IWidgetBuilder> UFigmaGroup::CreateWidgetBuilders(bool IsRoot/*= false*/, bool AllowFrameButton/*= true*/) const
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

	if (GenerateFile && !IsRoot)
	{
		UUserWidgetBuilder* UserWidgetBuilder = NewObject<UUserWidgetBuilder>();
		UserWidgetBuilder->SetNode(this);
		UserWidgetBuilder->SetWidgetBlueprintBuilder(GetAssetBuilder());
		return UserWidgetBuilder;
	}

	if (SemanticName.bHasWidgetPrefix)
	{
		if (TScriptInterface<IWidgetBuilder> ExplicitBuilder = CreateBuilderForWidgetType(SemanticName.WidgetType, AllowFrameButton))
		{
			return ExplicitBuilder;
		}
	}

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
		if (UButtonWidgetBuilder* ButtonBuilder = Cast<UButtonWidgetBuilder>(Button.GetObject()))
		{
			ApplyVisualTextureFromChildren(ButtonBuilder, this);
		}
		Button->SetChild(CreateContainersBuilder(EFigmaUMGWidgetRole::Auto, false, true, MakeContentWidgetName(this), true));

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
		UE_LOG_Figma2UMG(Warning, TEXT("[UFigmaGroup::CreateWidgetBuilders] Node %s is a %s but uses the %s widget prefix. Falling back to existing container heuristics."), *GetNodeName(), *GetClass()->GetName(), LexToString(SemanticName.Role));
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
		UE_LOG_Figma2UMG(Warning, TEXT("[UFigmaGroup::CreateWidgetBuilders] Node %s uses the %s widget prefix, which is not yet implemented for group nodes. Falling back to existing heuristics."), *GetNodeName(), LexToString(SemanticName.Role));
		break;
	case EFigmaUMGWidgetRole::Auto:
	default:
		break;
	}

	if (AllowFrameButton && IsButton())
	{
		TScriptInterface<UButtonWidgetBuilder> Button = CreateButtonBuilder();
		if (UButtonWidgetBuilder* ButtonBuilder = Cast<UButtonWidgetBuilder>(Button.GetObject()))
		{
			ApplyVisualTextureFromChildren(ButtonBuilder, this);
		}
		const TScriptInterface<IWidgetBuilder> Container = CreateContainersBuilder(EFigmaUMGWidgetRole::Auto, false, true, MakeContentWidgetName(this), true);
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

const TObjectPtr<UWidgetBlueprintBuilder>& UFigmaGroup::GetAssetBuilder() const
{
	return WidgetBlueprintBuilder;
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

TScriptInterface<IWidgetBuilder> UFigmaGroup::CreateBuilderForWidgetType(EFigmaUMGWidgetType WidgetType, bool AllowFrameButton) const
{
	switch (WidgetType)
	{
	case EFigmaUMGWidgetType::Button:
	{
		if (!AllowFrameButton)
		{
			return CreateContainersBuilder(EFigmaUMGWidgetRole::Auto, false, true, MakeContentWidgetName(this));
		}

		TScriptInterface<UButtonWidgetBuilder> Button = CreateButtonBuilder();
		if (UButtonWidgetBuilder* ButtonBuilder = Cast<UButtonWidgetBuilder>(Button.GetObject()))
		{
			ApplyVisualTextureFromChildren(ButtonBuilder, this);
		}
		Button->SetChild(CreateContainersBuilder(EFigmaUMGWidgetRole::Auto, false, true, MakeContentWidgetName(this), true));
#if (ENGINE_MAJOR_VERSION < 5 || ENGINE_MINOR_VERSION <= 2)
		return Button.GetInterface();
#else
		return Button;
#endif
	}
	case EFigmaUMGWidgetType::CheckBox:
	{
		UGenericLeafWidgetBuilder* LeafWidgetBuilder = NewObject<UGenericLeafWidgetBuilder>();
		LeafWidgetBuilder->SetNode(this);
		LeafWidgetBuilder->SetWidgetType(WidgetType);
		ApplyCheckBoxVisualTexturesFromChildren(LeafWidgetBuilder, this);
		return LeafWidgetBuilder;
	}
	case EFigmaUMGWidgetType::ComboBoxString:
	{
		UComboBoxStringWidgetBuilder* ComboBoxBuilder = NewObject<UComboBoxStringWidgetBuilder>();
		ComboBoxBuilder->SetNode(this);
		ApplyComboBoxVisualsFromChildren(ComboBoxBuilder, this);
		return ComboBoxBuilder;
	}
	case EFigmaUMGWidgetType::Border:
		return CreateBorderBuilder();
	case EFigmaUMGWidgetType::Image:
		if (HasImageWidgetPrefix())
		{
			UImageWidgetBuilder* ImageBuilder = NewObject<UImageWidgetBuilder>();
			ImageBuilder->SetNode(this);
			if (!ApplyImageSourceFromChildren(ImageBuilder, this))
			{
				UE_LOG_Figma2UMG(Warning, TEXT("[UFigmaGroup::CreateBuilderForWidgetType] IMG_ node %s has no direct T_ or Image_ child to use as its texture."), *GetNodeName());
			}
			return ImageBuilder;
		}

		if (!Children.IsEmpty())
		{
			UE_LOG_Figma2UMG(Display, TEXT("[UFigmaGroup::CreateBuilderForWidgetType] Node %s uses the Image prefix. Merging its subtree into a texture-backed UImage."), *GetNodeName());
		}
		if (Texture2DBuilder)
		{
			UImageWidgetBuilder* ImageBuilder = NewObject<UImageWidgetBuilder>();
			ImageBuilder->SetNode(this);
			ImageBuilder->SetTexture2DBuilder(Texture2DBuilder);
			return ImageBuilder;
		}
		return CreateImageBuilderForGroup(this);
	case EFigmaUMGWidgetType::ProgressBar:
	{
		UProgressBarWidgetBuilder* ProgressBarBuilder = NewObject<UProgressBarWidgetBuilder>();
		ProgressBarBuilder->SetNode(this);
		return ProgressBarBuilder;
	}
	case EFigmaUMGWidgetType::WidgetSwitcher:
	{
		UWidgetSwitcherBuilder* WidgetSwitcherBuilder = NewObject<UWidgetSwitcherBuilder>();
		WidgetSwitcherBuilder->SetNode(this);
		for (const UFigmaNode* Child : Children)
		{
			if (TScriptInterface<IWidgetBuilder> SubBuilder = Child->CreateWidgetBuilders())
			{
				WidgetSwitcherBuilder->AddChild(SubBuilder);
			}
		}

		return WidgetSwitcherBuilder;
	}
	case EFigmaUMGWidgetType::CanvasPanel:
	case EFigmaUMGWidgetType::VerticalBox:
	case EFigmaUMGWidgetType::HorizontalBox:
	case EFigmaUMGWidgetType::WrapBox:
	case EFigmaUMGWidgetType::Overlay:
	case EFigmaUMGWidgetType::UniformGridPanel:
	case EFigmaUMGWidgetType::GridPanel:
	case EFigmaUMGWidgetType::ScrollBox:
	{
		UPanelWidgetBuilder* PanelWidgetBuilder = CreatePanelBuilderForWidgetType(WidgetType);
		if (!PanelWidgetBuilder)
		{
			return nullptr;
		}

		PanelWidgetBuilder->SetNode(this);
		for (const UFigmaNode* Child : Children)
		{
			if (TScriptInterface<IWidgetBuilder> SubBuilder = Child->CreateWidgetBuilders())
			{
				PanelWidgetBuilder->AddChild(SubBuilder);
			}
		}

		return PanelWidgetBuilder;
	}
	case EFigmaUMGWidgetType::SizeBox:
	{
		USizeBoxWidgetBuilder* SizeBoxWidgetBuilder = NewObject<USizeBoxWidgetBuilder>();
		SizeBoxWidgetBuilder->SetNode(this);
		SizeBoxWidgetBuilder->SetChild(CreateContentBuilderForChildren(MakeContentWidgetName(this)));
		return SizeBoxWidgetBuilder;
	}
	case EFigmaUMGWidgetType::ListView:
	case EFigmaUMGWidgetType::TileView:
	{
		UGenericLeafWidgetBuilder* LeafWidgetBuilder = NewObject<UGenericLeafWidgetBuilder>();
		LeafWidgetBuilder->SetNode(this);
		LeafWidgetBuilder->SetWidgetType(WidgetType);
		LeafWidgetBuilder->SetListEntryWidgetBlueprintBuilder(ListEntryWidgetBlueprintBuilder);
		LeafWidgetBuilder->SetDesignerPreviewEntryCount(GetMeaningfulChildCount(Children));
		return LeafWidgetBuilder;
	}
	default:
		break;
	}

	if (IsUMGContentWidgetType(WidgetType))
	{
		UGenericContentWidgetBuilder* ContentWidgetBuilder = NewObject<UGenericContentWidgetBuilder>();
		ContentWidgetBuilder->SetNode(this);
		ContentWidgetBuilder->SetWidgetType(WidgetType);
		ContentWidgetBuilder->SetChild(CreateContentBuilderForChildren(MakeContentWidgetName(this)));
		return ContentWidgetBuilder;
	}

	if (GetUMGWidgetClass(WidgetType))
	{
		UGenericLeafWidgetBuilder* LeafWidgetBuilder = NewObject<UGenericLeafWidgetBuilder>();
		LeafWidgetBuilder->SetNode(this);
		LeafWidgetBuilder->SetWidgetType(WidgetType);
		return LeafWidgetBuilder;
	}

	return nullptr;
}

TScriptInterface<IWidgetBuilder> UFigmaGroup::CreateContentBuilderForChildren(const FString& PanelNameOverride, bool bSkipTextureSourceChildren) const
{
	TArray<TScriptInterface<IWidgetBuilder>> ChildBuilders;
	ChildBuilders.Reserve(Children.Num());
	for (const UFigmaNode* Child : Children)
	{
		if (Child == nullptr)
		{
			continue;
		}

		if (bSkipTextureSourceChildren && IsTextureSourceNode(Child))
		{
			continue;
		}

		if (TScriptInterface<IWidgetBuilder> SubBuilder = Child->CreateWidgetBuilders())
		{
			ChildBuilders.Add(SubBuilder);
		}
	}

	if (ChildBuilders.Num() == 0)
	{
		return nullptr;
	}

	if (ChildBuilders.Num() == 1)
	{
		return ChildBuilders[0];
	}

	UPanelWidgetBuilder* PanelWidgetBuilder = CreatePanelBuilderForLayout();
	PanelWidgetBuilder->SetNode(this);
	PanelWidgetBuilder->SetWidgetNameOverride(PanelNameOverride);
	for (const TScriptInterface<IWidgetBuilder>& ChildBuilder : ChildBuilders)
	{
		PanelWidgetBuilder->AddChild(ChildBuilder);
	}

	return PanelWidgetBuilder;
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

TScriptInterface<IWidgetBuilder> UFigmaGroup::CreateContainersBuilder(EFigmaUMGWidgetRole ForcedRole, bool bForceBorder, bool bSuppressVisualWrappers, const FString& PanelNameOverride, bool bSkipTextureSourceChildren) const
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
		if (bSkipTextureSourceChildren && IsTextureSourceNode(Child))
		{
			continue;
		}

		if (TScriptInterface<IWidgetBuilder> SubBuilder = Child->CreateWidgetBuilders())
		{
			PanelWidgetBuilder->AddChild(SubBuilder);
			++AddedChildrenCount;
		}
	}

	if (AddedChildrenCount == 0 && bSkipTextureSourceChildren && !BorderWidgetBuilder && !SizeBoxWidgetBuilder)
	{
		return nullptr;
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

UPanelWidgetBuilder* UFigmaGroup::CreatePanelBuilderForWidgetType(EFigmaUMGWidgetType WidgetType) const
{
	switch (WidgetType)
	{
	case EFigmaUMGWidgetType::CanvasPanel:
		return NewObject<UCanvasBuilder>();
	case EFigmaUMGWidgetType::HorizontalBox:
		return NewObject<UHBoxBuilder>();
	case EFigmaUMGWidgetType::VerticalBox:
		return NewObject<UVBoxBuilder>();
	case EFigmaUMGWidgetType::WrapBox:
		return NewObject<UWBoxBuilder>();
	default:
		break;
	}

	if (IsUMGPanelWidgetType(WidgetType))
	{
		UGenericPanelWidgetBuilder* GenericPanelWidgetBuilder = NewObject<UGenericPanelWidgetBuilder>();
		GenericPanelWidgetBuilder->SetWidgetType(WidgetType);
		return GenericPanelWidgetBuilder;
	}

	return nullptr;
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
			WrapBox->SetInnerSlotPadding(Figma2UMGLayout::RoundLayoutVector(FVector2D(ItemSpacing, CounterAxisSpacing)));
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
				Spacer->SetSize(Figma2UMGLayout::RoundLayoutVector(FVector2D(ItemSpacing, ItemSpacing)));
				PanelWidget->InsertChildAt(i, Spacer);
			}
			else if (ShouldBeSpacer && IsSpacer)
			{
				USpacer* Spacer = Cast<USpacer>(Widget);
				Spacer->SetSize(Figma2UMGLayout::RoundLayoutVector(FVector2D(ItemSpacing, ItemSpacing)));
			}
		}
	}

}

// MIT License
// Copyright (c) 2024 Buvi Games


#include "Parser/Nodes/Vectors/FigmaText.h"

#include "Figma2UMGModule.h"
#include "WidgetBlueprint.h"
#include "Builder/WidgetBlueprintHelper.h"
#include "Builder/Asset/FontBuilder.h"
#include "Builder/Asset/Texture2DBuilder.h"
#include "Builder/Widget/GenericWidgetBuilder.h"
#include "Builder/Widget/TextBlockWidgetBuilder.h"
#include "Components/TextBlock.h"
#include "Dom/JsonObject.h"
#include "Parser/Properties/FigmaUMGSemanticName.h"
#include "Serialization/JsonTypes.h"

namespace
{
	FFigmaRectangle GetEffectiveTextBounds(const UFigmaText* TextNode)
	{
		if (!TextNode)
		{
			return FFigmaRectangle();
		}

		FFigmaRectangle Bounds = TextNode->AbsoluteBoundingBox;
		const FFigmaRectangle& RenderBounds = TextNode->AbsoluteRenderBounds;
		const bool bHasRenderBounds = RenderBounds.Width > KINDA_SMALL_NUMBER && RenderBounds.Height > KINDA_SMALL_NUMBER;
		if (bHasRenderBounds)
		{
			if (Bounds.Width <= KINDA_SMALL_NUMBER)
			{
				Bounds.X = RenderBounds.X;
				Bounds.Width = RenderBounds.Width;
			}
			if (Bounds.Height <= KINDA_SMALL_NUMBER)
			{
				Bounds.Y = RenderBounds.Y;
				Bounds.Height = RenderBounds.Height;
			}
		}

		return Bounds;
	}

	bool TryGetFirstVisibleSolidPaintColor(const TArray<FFigmaPaint>& Paints, FLinearColor& OutColor)
	{
		for (const FFigmaPaint& Paint : Paints)
		{
			if (Paint.Visible && Paint.Type == EPaintTypes::SOLID)
			{
				OutColor = Paint.GetLinearColorFromSRGB();
				return true;
			}
		}

		return false;
	}
}

void UFigmaText::PostSerialize(const TObjectPtr<UFigmaNode> InParent, const TSharedRef<FJsonObject> JsonObj)
{
	Super::PostSerialize(InParent, JsonObj);

	PostSerializeProperty(JsonObj, "fills", Fills);
	PostSerializeProperty(JsonObj, "strokes", Strokes);

	static FString StyleStr("style");
	if (JsonObj->HasTypedField<EJson::Object>(StyleStr))
	{
		const TSharedPtr<FJsonObject> StyleJson = JsonObj->GetObjectField(StyleStr);
		Style.PostSerialize(StyleJson);
	}
}

FVector2D UFigmaText::GetAbsolutePosition(const bool IsTopWidgetForNode) const
{
	return GetEffectiveTextBounds(this).GetPosition(IsTopWidgetForNode ? GetAbsoluteRotation() : 0.0f);
}

FVector2D UFigmaText::GetAbsoluteSize(const bool IsTopWidgetForNode) const
{
	return GetEffectiveTextBounds(this).GetSize(IsTopWidgetForNode ? GetAbsoluteRotation() : 0.0f);
}

FVector2D UFigmaText::GetAbsoluteCenter() const
{
	return GetEffectiveTextBounds(this).GetCenter();
}

bool UFigmaText::CreateAssetBuilder(const FString& InFileKey, TArray<TScriptInterface<IAssetBuilder>>& AssetBuilders)
{
	if (HasTextureOnlyImagePrefix())
	{
		UTexture2DBuilder* Texture2DBuilder = NewObject<UTexture2DBuilder>();
		Texture2DBuilder->SetNode(InFileKey, this);
		AssetBuilders.Add(Texture2DBuilder);
		return true;
	}

	//TODO: Look if font is already imported.
	UFontBuilder* AssetBuilder = NewObject<UFontBuilder>();
	AssetBuilder->SetNode(InFileKey, this);
	AssetBuilder->SetFontFamily(Style.FontFamily);
	AssetBuilders.Add(AssetBuilder);

	CreatePaintAssetBuilderIfNeeded(InFileKey, AssetBuilders, Fills, Strokes);

	return true;
}

FString UFigmaText::GetPackageNameForBuilder(const TScriptInterface<IAssetBuilder>& InAssetBuilder) const
{
	TObjectPtr<UFigmaNode> TopParentNode = ParentNode;
	while (TopParentNode && TopParentNode->GetParentNode())
	{
		TopParentNode = TopParentNode->GetParentNode();
	}
	if (HasTextureOnlyImagePrefix() && Cast<UTexture2DBuilder>(InAssetBuilder.GetObject()))
	{
		return TopParentNode->GetCurrentPackagePath() + TEXT("/Textures");
	}

	const FString Suffix = "Fonts";
	return TopParentNode->GetCurrentPackagePath() + TEXT("/") + Suffix;
}

TScriptInterface<IWidgetBuilder> UFigmaText::CreateWidgetBuilders(bool IsRoot/*= false*/, bool AllowFrameButton/*= true*/) const
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

	switch (SemanticName.WidgetType)
	{
	case EFigmaUMGWidgetType::RichTextBlock:
	case EFigmaUMGWidgetType::EditableText:
	case EFigmaUMGWidgetType::EditableTextBox:
	case EFigmaUMGWidgetType::MultiLineEditableText:
	case EFigmaUMGWidgetType::MultiLineEditableTextBox:
	{
		UGenericLeafWidgetBuilder* LeafWidgetBuilder = NewObject<UGenericLeafWidgetBuilder>();
		LeafWidgetBuilder->SetNode(this);
		LeafWidgetBuilder->SetWidgetType(SemanticName.WidgetType);
		return LeafWidgetBuilder;
	}
	default:
		break;
	}

	if (SemanticName.Role != EFigmaUMGWidgetRole::Auto
		&& SemanticName.Role != EFigmaUMGWidgetRole::Text
		&& SemanticName.Role != EFigmaUMGWidgetRole::RichText)
	{
		UE_LOG_Figma2UMG(Warning, TEXT("[UFigmaText::CreateWidgetBuilders] TEXT node %s uses the %s widget prefix. Keeping UTextBlock fallback."), *GetNodeName(), LexToString(SemanticName.Role));
	}

	UTextBlockWidgetBuilder* TextBlockWidgetBuilder = NewObject<UTextBlockWidgetBuilder>();
	TextBlockWidgetBuilder->SetNode(this);

	return TextBlockWidgetBuilder;
}

bool UFigmaText::TryGetTextColorFromFigmaSRGB(FLinearColor& OutColor) const
{
	if (TryGetFirstVisibleSolidPaintColor(Fills, OutColor))
	{
		return true;
	}

	return TryGetFirstVisibleSolidPaintColor(Style.Fills, OutColor);
}

bool UFigmaText::TryGetTextStrokeFromFigmaSRGB(FLinearColor& OutColor, float& OutStrokeWeight) const
{
	if (StrokeWeight <= 0.0f)
	{
		return false;
	}

	if (!TryGetFirstVisibleSolidPaintColor(Strokes, OutColor))
	{
		return false;
	}

	OutStrokeWeight = StrokeWeight;
	return true;
}

void UFigmaText::ProcessComponentPropertyReference(TObjectPtr<UWidgetBlueprint> WidgetBP, TObjectPtr<UWidget> Widget, const TPair<FString, FString>& PropertyReference) const
{
	static const FString CharactersStr("characters");
	if (PropertyReference.Key == CharactersStr)
	{
		const FBPVariableDescription* VariableDescription = WidgetBP->NewVariables.FindByPredicate([PropertyReference](const FBPVariableDescription& VariableDescription)
			{
#if (ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 3)
				return VariableDescription.VarName == PropertyReference.Value;
#else
				return VariableDescription.VarName.ToString() == PropertyReference.Value;
#endif
			});

		if (VariableDescription != nullptr)
		{
			UE_LOG_Figma2UMG(Display, TEXT("[ProcessComponentPropertyReference] Variable '%s' found in UWidgetBlueprint %s."), *PropertyReference.Value, *WidgetBP->GetName());
			TObjectPtr<UTextBlock> TextBlock = Cast<UTextBlock>(Widget);
			if (TextBlock == nullptr)
			{
				UE_LOG_Figma2UMG(Error, TEXT("[ProcessComponentPropertyReference] UWidgetBlueprint %s's Widget '%s' is not a UTextBlock. Fail to bind %s."), *WidgetBP->GetName(), *Widget->GetName(), *PropertyReference.Value);
				return;
			}

			WidgetBlueprintHelper::PatchTextBind(WidgetBP, TextBlock, *PropertyReference.Value);
			return;
		}
		else
		{
			UClass* WidgetClass = Widget->GetClass();
			FProperty* Property = WidgetClass ? FindFProperty<FProperty>(WidgetClass, *PropertyReference.Value) : nullptr;
			if (Property)
			{
				const FStrProperty* StringProperty = CastField<FStrProperty>(Property);
				void* Value = StringProperty->ContainerPtrToValuePtr<uint8>(Widget);
				StringProperty->SetPropertyValue(Value, Characters);

				UE_LOG_Figma2UMG(Display, TEXT("[ProcessComponentPropertyReference] Variable '%s' found in UWidget %s."), *PropertyReference.Value, *Widget->GetName());
				return;
			}

		}

		UE_LOG_Figma2UMG(Error, TEXT("[ProcessComponentPropertyReference] Variable '%s' not found in UWidgetBlueprint %s or UWidget %s."), *PropertyReference.Value, *WidgetBP->GetName(), *Widget->GetName());
	}
	else
	{
		Super::ProcessComponentPropertyReference(WidgetBP, Widget, PropertyReference);
	}
}

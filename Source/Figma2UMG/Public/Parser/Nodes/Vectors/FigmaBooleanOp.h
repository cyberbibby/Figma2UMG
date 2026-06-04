// MIT License
// Copyright (c) 2024 Buvi Games

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/FigmaContainer.h"
#include "Parser/Nodes/Vectors/FigmaVectorNode.h"

#include "FigmaBooleanOp.generated.h"

UCLASS()
class FIGMA2UMG_API UFigmaBooleanOp : public UFigmaVectorNode, public IFigmaContainer
{
public:
	GENERATED_BODY()

	virtual void PostSerialize(const TObjectPtr<UFigmaNode> InParent, const TSharedRef<FJsonObject> JsonObj) override;
	virtual bool CreateAssetBuilder(const FString& InFileKey, TArray<TScriptInterface<IAssetBuilder>>& AssetBuilders) override;
	virtual TScriptInterface<IWidgetBuilder> CreateWidgetBuilders(bool IsRoot = false, bool AllowFrameButton = true) const override;

	// IFigmaContainer
	virtual FString GetJsonArrayName() const override { return FString("Children"); };
	virtual TArray<TObjectPtr<UFigmaNode>>& GetChildren() override { return Children; }
	virtual const TArray<TObjectPtr<UFigmaNode>>& GetChildrenConst() const override { return Children; }

	UPROPERTY(Transient)
	TArray<TObjectPtr<UFigmaNode>> Children;
};

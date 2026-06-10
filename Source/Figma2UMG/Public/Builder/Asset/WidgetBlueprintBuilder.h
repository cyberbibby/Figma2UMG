// MIT License
// Copyright (c) 2024 Buvi Games

#pragma once

#include "CoreMinimal.h"
#include "AssetBuilder.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "WidgetBlueprintBuilder.generated.h"

class IWidgetBuilder;
struct FFigmaComponentPropertyDefinition;
class UWidgetBlueprint;

UCLASS()
class FIGMA2UMG_API UWidgetBlueprintBuilder : public UObject, public IAssetBuilder
{
	GENERATED_BODY()
public:
	virtual void LoadOrCreateAssets() override;
	virtual void LoadAssets() override;
	virtual void Reset() override;
	void SetImplementListEntryInterface(bool bInImplementListEntryInterface);
	bool IsListEntryWidgetBlueprintBuilder() const;

	virtual void ResetWidgets();

	void CompileBP(EBlueprintCompileOptions CompileFlags);

	void CreateWidgetBuilders();
	void PatchAndInsertWidgets();
	void PatchWidgetBinds();
	void PatchWidgetProperties();

	TObjectPtr<UWidgetBlueprint> GetAsset() const;

	virtual UPackage* GetAssetPackage() const override;
protected:
	bool ShouldReuseExistingWidgetBlueprint() const;
	TObjectPtr<UWidgetBlueprint> FindExistingWidgetBlueprintByNodeName() const;
	TScriptInterface<IWidgetBuilder> CreateListEntryRootWidgetBuilder() const;
	void ApplyDesignPreviewSize(UWidgetBlueprint* WidgetBP) const;
	void FillType(const FFigmaComponentPropertyDefinition& Def, FEdGraphPinType& MemberType) const;
	void PatchMemberVariable(UWidgetBlueprint* WidgetBP, TPair<FString, FFigmaComponentPropertyDefinition> Property) const;
	void PatchPropertyDefinitions(const TMap<FString, FFigmaComponentPropertyDefinition>& ComponentPropertyDefinitions) const;

	UPROPERTY()
	TObjectPtr<UWidgetBlueprint> Asset = nullptr;

	UPROPERTY()
	TScriptInterface<IWidgetBuilder> RootWidgetBuilder = nullptr;

	UPROPERTY()
	bool bImplementListEntryInterface = false;

	UPROPERTY()
	bool bReusingExistingWidgetBlueprint = false;
};

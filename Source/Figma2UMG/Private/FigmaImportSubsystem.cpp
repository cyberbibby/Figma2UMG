// MIT License
// Copyright (c) 2024 Buvi Games

#include "FigmaImportSubsystem.h"

#include "PackageTools.h"
#include "Engine/Font.h"
#include "Engine/ObjectLibrary.h"
#include "REST/FigmaImporter.h"
#include "REST/RequestParams.h"


UFigmaImporter* UFigmaImportSubsystem::Request(const TObjectPtr<URequestParams> InProperties, const FOnFigmaImportUpdateStatusCB& InRequesterCallback)
{
	UFigmaImporter* request = Requests.Emplace_GetRef(NewObject<UFigmaImporter>());
	WidgetOverrides = &InProperties->WidgetOverrides;
	FrameToButtonOverride = &InProperties->FrameToButton;
	WidgetPrefixMappings = &InProperties->WidgetPrefixMappings;
	bDebugNodeName = InProperties->DebugNodeName;

	RefreshFontAssets();
	request->Init(InProperties, InRequesterCallback);;
	request->Run();
	return request;
}

void UFigmaImportSubsystem::RemoveRequest(UFigmaImporter* FigmaImporter)
{
	WidgetOverrides = nullptr;
	FrameToButtonOverride = nullptr;
	WidgetPrefixMappings = nullptr;
	bDebugNodeName = false;
	Requests.Remove(FigmaImporter);
}

const FWidgetPrefixMapping* UFigmaImportSubsystem::FindWidgetPrefixMappingForNode(const FString& NodeName) const
{
	if (!WidgetPrefixMappings || NodeName.IsEmpty())
	{
		return nullptr;
	}

	const FWidgetPrefixMapping* BestMapping = nullptr;
	for (const FWidgetPrefixMapping& Mapping : *WidgetPrefixMappings)
	{
		if (!Mapping.Match(NodeName))
		{
			continue;
		}

		if (!BestMapping || Mapping.Prefix.Len() > BestMapping->Prefix.Len())
		{
			BestMapping = &Mapping;
		}
	}

	return BestMapping;
}

bool UFigmaImportSubsystem::ShouldGenerateButton(const FString& NodeName) const
{
	if (!FrameToButtonOverride)
		return false;

	if (NodeName.IsEmpty())
		return false;

	for (const FWidgetOverride& Override : FrameToButtonOverride->Rules)
	{
		if (Override.Match(NodeName))
			return true;
	}

	return false;
}

void UFigmaImportSubsystem::RefreshFontAssets()
{
	NewFonts.Reset();
	if(FontObjectLibrary)
	{
		FontObjectLibrary->ClearLoaded();
	}
	else
	{
		FontObjectLibrary = UObjectLibrary::CreateLibrary(UFont::StaticClass(), false, GIsEditor);
	}

	TArray<FString> Paths;
	Paths.Add(TEXT("/Game"));
	Paths.Add(TEXT("/Engine/EngineFonts"));
	FontObjectLibrary->LoadAssetDataFromPaths(Paths);
}

void UFigmaImportSubsystem::AddNewFont(UFont* NewFont)
{
	NewFonts.AddUnique(NewFont);
}

UFont* UFigmaImportSubsystem::FindFontAssetFromFamily(const FString& FamilyName) const
{
	TArray<FAssetData> AssetDatas;
	FontObjectLibrary->GetAssetDataList(AssetDatas);

	const FString FontFamily = UPackageTools::SanitizePackageName(FamilyName.Replace(TEXT(" "), TEXT("")));
	for (const FAssetData& AssetData : AssetDatas)
	{
		if (!FontFamily.Equals(AssetData.AssetName.ToString(), ESearchCase::IgnoreCase))
			continue;

		UObject* Asset = AssetData.GetAsset();
		UFont* Font = Cast<UFont>(Asset);
		if (Font && Font->GetName().Equals(FontFamily, ESearchCase::IgnoreCase))
		{
			return Font;
		}
	}

	for (UFont* Font : NewFonts)
	{
		if (Font && Font->GetName().Equals(FontFamily, ESearchCase::IgnoreCase))
		{
			return Font;
		}
	}

	return nullptr;
}

FGFontFamilyInfo* UFigmaImportSubsystem::FindGoogleFontsInfo(const FString& FamilyName)
{
	FGFontFamilyInfo* GFontFamilyInfo = GoogleFontsInfo.FindByPredicate([FamilyName](const FGFontFamilyInfo& GFontFamilyInfo)
		{
			return GFontFamilyInfo.Family.Equals(FamilyName, ESearchCase::IgnoreCase);
		});

	return GFontFamilyInfo;
}

void UFigmaImportSubsystem::TryRenameWidget(const FString& InName, TObjectPtr<UWidget> Widget)
{
	if (!Widget)
		return;

	if (Widget->GetName().Equals(InName, ESearchCase::CaseSensitive))
		return;

	const FName ObjectName = MakeWidgetObjectName(Widget->GetOuter(), Widget->GetClass(), InName, Widget);
	Widget->Rename(*ObjectName.ToString());
}

FName UFigmaImportSubsystem::MakeWidgetObjectName(UObject* Outer, UClass* WidgetClass, const FString& WidgetName, const UObject* ExistingObject)
{
	const FName DesiredName(*WidgetName);
	if (Outer && !DesiredName.IsNone())
	{
		const UObject* ExistingNamedObject = FindObject<UObject>(Outer, *WidgetName);
		if (!ExistingNamedObject || ExistingNamedObject == ExistingObject)
		{
			return DesiredName;
		}
	}

	return MakeUniqueObjectName(Outer, WidgetClass, DesiredName);
}

UMaterialInstanceConstant* UFigmaImportSubsystem::GetBorderMaterialInstances(float StrokeWeight) const
{
	if (BorderMaterialInstances.Contains(StrokeWeight))
	{
		return BorderMaterialInstances[StrokeWeight];
	}

	return nullptr;
}

void UFigmaImportSubsystem::AddBorderMaterialInstances(float StrokeWeight, UMaterialInstanceConstant* MaterialInstanceConstant)
{
	BorderMaterialInstances.Add(StrokeWeight, MaterialInstanceConstant);
}

void UFigmaImportSubsystem::ResetBorderMaterials()
{
	BorderMaterial = nullptr;
	BorderMaterialInstances.Reset();
}

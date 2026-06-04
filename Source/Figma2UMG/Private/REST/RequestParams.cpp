// MIT License
// Copyright (c) 2024 Buvi Games


#include "REST/RequestParams.h"

#include "Figma2UMGModule.h"
#include "Settings/Figma2UMGSettings.h"

namespace
{
	FString NormalizeFigmaNodeId(FString NodeId)
	{
		NodeId = NodeId.TrimStartAndEnd();
		NodeId.ReplaceInline(TEXT("%3A"), TEXT(":"), ESearchCase::IgnoreCase);
		NodeId.ReplaceInline(TEXT("%3B"), TEXT(";"), ESearchCase::IgnoreCase);
		NodeId.ReplaceInline(TEXT("%2D"), TEXT("-"), ESearchCase::IgnoreCase);
		NodeId.ReplaceInline(TEXT("-"), TEXT(":"), ESearchCase::CaseSensitive);
		return NodeId;
	}
}

FString URequestParams::ExtractFileKeyFromInput(const FString& InLayerURL)
{
	FString LayerURL = InLayerURL.TrimStartAndEnd();
	if (LayerURL.IsEmpty())
	{
		return LayerURL;
	}

	const TCHAR* FigmaPathPrefixes[] =
	{
		TEXT("/design/"),
		TEXT("/file/"),
		TEXT("/proto/")
	};

	for (const TCHAR* Prefix : FigmaPathPrefixes)
	{
		const int32 PrefixIndex = LayerURL.Find(Prefix, ESearchCase::IgnoreCase);
		if (PrefixIndex == INDEX_NONE)
		{
			continue;
		}

		const int32 KeyStartIndex = PrefixIndex + FCString::Strlen(Prefix);
		FString KeyRemainder = LayerURL.Mid(KeyStartIndex);
		if (KeyRemainder.IsEmpty())
		{
			continue;
		}

		int32 KeyEndIndex = KeyRemainder.Len();
		const TCHAR Delimiters[] = { TEXT('/'), TEXT('?'), TEXT('#'), TEXT('&'), TEXT(')'), TEXT(']') };
		for (const TCHAR Delimiter : Delimiters)
		{
			int32 DelimiterIndex = INDEX_NONE;
			if (KeyRemainder.FindChar(Delimiter, DelimiterIndex))
			{
				KeyEndIndex = FMath::Min(KeyEndIndex, DelimiterIndex);
			}
		}

		FString FileKey = KeyRemainder.Left(KeyEndIndex).TrimStartAndEnd();
		if (!FileKey.IsEmpty())
		{
			return FileKey;
		}
	}

	return LayerURL;
}

FString URequestParams::ExtractNodeIdFromInput(const FString& InLayerURL)
{
	FString LayerURL = InLayerURL.TrimStartAndEnd();
	if (LayerURL.IsEmpty())
	{
		return FString();
	}

	static const FString NodeIdKey(TEXT("node-id="));
	const int32 NodeIdIndex = LayerURL.Find(NodeIdKey, ESearchCase::IgnoreCase);
	if (NodeIdIndex == INDEX_NONE)
	{
		return FString();
	}

	const int32 NodeIdStartIndex = NodeIdIndex + NodeIdKey.Len();
	FString NodeIdRemainder = LayerURL.Mid(NodeIdStartIndex);
	if (NodeIdRemainder.IsEmpty())
	{
		return FString();
	}

	int32 NodeIdEndIndex = NodeIdRemainder.Len();
	const TCHAR Delimiters[] = { TEXT('&'), TEXT('#'), TEXT('/'), TEXT(')'), TEXT(']') };
	for (const TCHAR Delimiter : Delimiters)
	{
		int32 DelimiterIndex = INDEX_NONE;
		if (NodeIdRemainder.FindChar(Delimiter, DelimiterIndex))
		{
			NodeIdEndIndex = FMath::Min(NodeIdEndIndex, DelimiterIndex);
		}
	}

	return NormalizeFigmaNodeId(NodeIdRemainder.Left(NodeIdEndIndex));
}

URequestParams::URequestParams(const FObjectInitializer& ObjectInitializer)
{
	FFigma2UMGModule& Figma2UMGModule = FModuleManager::LoadModuleChecked<FFigma2UMGModule>("Figma2UMG");
	UFigma2UMGSettings* Settings = Figma2UMGModule.GetSettings();
	if (Settings)
	{
		AccessToken = Settings->AccessToken;
		LayerURL = Settings->LayerURL;
		LibraryFileKeys = Settings->LibraryFileKeys;
		DownloadFontsFromGoogle = Settings->DownloadFontsFromGoogle;
		GFontsAPIKey = Settings->GFontsAPIKey;
		UsePrototypeFlow = Settings->UsePrototypeFlow;
		FrameToButton = Settings->FrameToButton;
		WidgetPrefixMappings = Settings->WidgetPrefixMappings;
		WidgetOverrides = Settings->WidgetOverrides;
		SaveAllAtEnd = Settings->SaveAllAtEnd;
		MaxURLImageRequest = Settings->MaxURLImageRequest;
		NodeImageScale = Settings->NodeImageScale;
		ContentRootFolder = Settings->ContentRootFolder;
	}

	if (WidgetPrefixMappings.IsEmpty())
	{
		ResetUMGWidgetPrefixMappingsToDefault(WidgetPrefixMappings);
	}


	//#if !UE_BUILD_SHIPPING
	//Ids.Add("212:1394"); // Impost Sections
	//Ids.Add("146:1357"); // SectionProperty
	//Ids.Add("212:1393"); // SectionVariationButton
	//Ids.Add("294:1393"); // SectionVariation
	//Ids.Add("212:1395"); // SectionRemoteLib
	//#endif

}

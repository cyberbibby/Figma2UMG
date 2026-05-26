// MIT License
// Copyright (c) 2024 Buvi Games


#include "REST/RequestParams.h"

#include "Figma2UMGModule.h"
#include "Settings/Figma2UMGSettings.h"

FString URequestParams::ExtractFileKeyFromInput(const FString& InFileURL)
{
	FString FileURL = InFileURL.TrimStartAndEnd();
	if (FileURL.IsEmpty())
	{
		return FileURL;
	}

	const TCHAR* FigmaPathPrefixes[] =
	{
		TEXT("/design/"),
		TEXT("/file/"),
		TEXT("/proto/")
	};

	for (const TCHAR* Prefix : FigmaPathPrefixes)
	{
		const int32 PrefixIndex = FileURL.Find(Prefix, ESearchCase::IgnoreCase);
		if (PrefixIndex == INDEX_NONE)
		{
			continue;
		}

		const int32 KeyStartIndex = PrefixIndex + FCString::Strlen(Prefix);
		FString KeyRemainder = FileURL.Mid(KeyStartIndex);
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

	return FileURL;
}

URequestParams::URequestParams(const FObjectInitializer& ObjectInitializer)
{
	FFigma2UMGModule& Figma2UMGModule = FModuleManager::LoadModuleChecked<FFigma2UMGModule>("Figma2UMG");
	UFigma2UMGSettings* Settings = Figma2UMGModule.GetSettings();
	if (Settings)
	{
		AccessToken = Settings->AccessToken;
		FileKey = Settings->FileKey;
		LibraryFileKeys = Settings->LibraryFileKeys;
		DownloadFontsFromGoogle = Settings->DownloadFontsFromGoogle;
		GFontsAPIKey = Settings->GFontsAPIKey;
		UsePrototypeFlow = Settings->UsePrototypeFlow;
		FrameToButton = Settings->FrameToButton;
		WidgetOverrides = Settings->WidgetOverrides;
		SaveAllAtEnd = Settings->SaveAllAtEnd;
		MaxURLImageRequest = Settings->MaxURLImageRequest;
		NodeImageScale = Settings->NodeImageScale;
		ContentRootFolder = Settings->ContentRootFolder;
	}


	//#if !UE_BUILD_SHIPPING
	//Ids.Add("212:1394"); // Impost Sections
	//Ids.Add("146:1357"); // SectionProperty
	//Ids.Add("212:1393"); // SectionVariationButton
	//Ids.Add("294:1393"); // SectionVariation
	//Ids.Add("212:1395"); // SectionRemoteLib
	//#endif

}

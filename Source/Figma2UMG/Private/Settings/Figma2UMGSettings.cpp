// MIT License
// Copyright (c) 2024 Buvi Games

#include "Settings/Figma2UMGSettings.h"

#include "Misc/ConfigCacheIni.h"

UFigma2UMGSettings::UFigma2UMGSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	if (LayerURL.IsEmpty() && GConfig)
	{
		static const TCHAR* SettingsSection = TEXT("/Script/Figma2UMG.Figma2UMGSettings");
		if (!GConfig->GetString(SettingsSection, TEXT("NodeURL"), LayerURL, GEngineIni))
		{
			GConfig->GetString(SettingsSection, TEXT("FileKey"), LayerURL, GEngineIni);
		}
	}

	ResetUMGWidgetPrefixMappingsToDefault(WidgetPrefixMappings);
}

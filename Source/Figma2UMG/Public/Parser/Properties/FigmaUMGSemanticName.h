#pragma once

#include "CoreMinimal.h"

#include "FigmaUMGSemanticName.generated.h"

UENUM()
enum class EFigmaUMGWidgetRole : uint8
{
	Auto,
	Button,
	Text,
	RichText,
	Image,
	Border,
	Canvas,
	HBox,
	VBox,
	WrapBox,
	Overlay,
	ScrollBox,
	ProgressBar,
	CheckBox,
	Slider,
	InputText,
	WidgetSwitcher,
	Panel,
	Decor,
	Ignore
};

USTRUCT()
struct FIGMA2UMG_API FFigmaUMGSemanticName
{
	GENERATED_BODY()

	UPROPERTY()
	bool bHasUMGPrefix = false;

	UPROPERTY()
	EFigmaUMGWidgetRole Role = EFigmaUMGWidgetRole::Auto;

	UPROPERTY()
	FString SemanticName;

	UPROPERTY()
	FString OriginalName;

	static FFigmaUMGSemanticName Parse(const FString& NodeName);

	bool HasExplicitRole() const
	{
		return bHasUMGPrefix && Role != EFigmaUMGWidgetRole::Auto;
	}
};

FIGMA2UMG_API const TCHAR* LexToString(EFigmaUMGWidgetRole Role);

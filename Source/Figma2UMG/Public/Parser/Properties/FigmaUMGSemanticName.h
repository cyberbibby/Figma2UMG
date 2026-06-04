#pragma once

#include "CoreMinimal.h"
#include "Settings/ClassOverrides.h"

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
	Ignore,
	SizeBox,
	ScaleBox,
	SafeZone,
	MenuAnchor,
	NamedSlot,
	BackgroundBlur,
	InvalidationBox,
	RetainerBox,
	WindowTitleBarArea,
	ScrollBar,
	EditableText,
	EditableTextBox,
	MultiLineEditableText,
	MultiLineEditableTextBox,
	ComboBoxString,
	SpinBox,
	InputKeySelector,
	Throbber,
	CircularThrobber,
	Spacer,
	ListView,
	TileView,
	UniformGridPanel,
	GridPanel,
};

USTRUCT()
struct FIGMA2UMG_API FFigmaUMGSemanticName
{
	GENERATED_BODY()

	UPROPERTY()
	bool bHasWidgetPrefix = false;

	UPROPERTY()
	EFigmaUMGWidgetRole Role = EFigmaUMGWidgetRole::Auto;

	UPROPERTY()
	EFigmaUMGWidgetType WidgetType = EFigmaUMGWidgetType::None;

	UPROPERTY()
	FString SemanticName;

	UPROPERTY()
	FString OriginalName;

	static FFigmaUMGSemanticName Parse(const FString& NodeName);

	bool HasExplicitRole() const
	{
		return bHasWidgetPrefix && Role != EFigmaUMGWidgetRole::Auto;
	}
};

FIGMA2UMG_API const TCHAR* LexToString(EFigmaUMGWidgetRole Role);

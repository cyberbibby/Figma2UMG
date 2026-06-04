// MIT License
// Copyright (c) 2024 Buvi Games

#include "Builder/Widget/GenericWidgetBuilder.h"

#include "Blueprint/WidgetTree.h"
#include "Blueprint/IUserListEntry.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Builder/Asset/WidgetBlueprintBuilder.h"
#include "Builder/Widget/Figma2UMGListEntryWidget.h"
#include "Components/ContentWidget.h"
#include "Components/CheckBox.h"
#include "Components/EditableText.h"
#include "Components/EditableTextBox.h"
#include "Components/ListViewBase.h"
#include "Components/MultiLineEditableText.h"
#include "Components/MultiLineEditableTextBox.h"
#include "Components/PanelWidget.h"
#include "Components/RichTextBlock.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Figma2UMGModule.h"
#include "FigmaImportSubsystem.h"
#include "Parser/Nodes/FigmaNode.h"
#include "Parser/Nodes/Vectors/FigmaText.h"
#include "UObject/UnrealType.h"
#include "WidgetBlueprint.h"

namespace
{
	UClass* GetWidgetClassChecked(EFigmaUMGWidgetType WidgetType, const UClass* RequiredBaseClass, const FString& NodeName)
	{
		UClass* WidgetClass = GetUMGWidgetClass(WidgetType);
		if (!WidgetClass || !WidgetClass->IsChildOf(RequiredBaseClass))
		{
			UE_LOG_Figma2UMG(Warning, TEXT("[GenericWidgetBuilder] Node %s requested unsupported widget type %s."), *NodeName, LexToString(WidgetType));
			return nullptr;
		}

		return WidgetClass;
	}

	void ApplyTextIfSupported(const UFigmaNode* Node, const TObjectPtr<UWidget>& Widget)
	{
		const UFigmaText* FigmaText = Cast<UFigmaText>(Node);
		if (!FigmaText || !Widget)
		{
			return;
		}

		const FText Text = FText::FromString(FigmaText->Characters);
		if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
		{
			TextBlock->SetText(Text);
		}
		else if (URichTextBlock* RichTextBlock = Cast<URichTextBlock>(Widget))
		{
			RichTextBlock->SetText(Text);
		}
		else if (UEditableText* EditableText = Cast<UEditableText>(Widget))
		{
			EditableText->SetText(Text);
		}
		else if (UEditableTextBox* EditableTextBox = Cast<UEditableTextBox>(Widget))
		{
			EditableTextBox->SetText(Text);
		}
		else if (UMultiLineEditableText* MultiLineEditableText = Cast<UMultiLineEditableText>(Widget))
		{
			MultiLineEditableText->SetText(Text);
		}
		else if (UMultiLineEditableTextBox* MultiLineEditableTextBox = Cast<UMultiLineEditableTextBox>(Widget))
		{
			MultiLineEditableTextBox->SetText(Text);
		}
	}

	UClass* ResolveListEntryWidgetClass(const TObjectPtr<UWidgetBlueprintBuilder>& EntryWidgetBlueprintBuilder)
	{
		if (!EntryWidgetBlueprintBuilder)
		{
			return UFigma2UMGListEntryWidget::StaticClass();
		}

		UWidgetBlueprint* EntryWidgetBlueprint = EntryWidgetBlueprintBuilder->GetAsset();
		if (!EntryWidgetBlueprint)
		{
			EntryWidgetBlueprintBuilder->LoadAssets();
			EntryWidgetBlueprint = EntryWidgetBlueprintBuilder->GetAsset();
		}

		if (EntryWidgetBlueprint)
		{
			if (EntryWidgetBlueprint->GeneratedClass)
			{
				return EntryWidgetBlueprint->GeneratedClass;
			}

			if (UClass* BlueprintClass = EntryWidgetBlueprint->GetBlueprintClass())
			{
				return BlueprintClass;
			}
		}

		UE_LOG_Figma2UMG(Error, TEXT("[GenericWidgetBuilder] Could not resolve generated EntryWidgetClass from list entry blueprint builder."));
		return nullptr;
	}

	FClassProperty* FindEntryWidgetClassProperty(const UListViewBase* ListViewBase)
	{
		if (ListViewBase)
		{
			if (FClassProperty* EntryWidgetClassProperty = FindFProperty<FClassProperty>(ListViewBase->GetClass(), TEXT("EntryWidgetClass")))
			{
				return EntryWidgetClassProperty;
			}
		}

		return FindFProperty<FClassProperty>(UListViewBase::StaticClass(), TEXT("EntryWidgetClass"));
	}

	void ApplyListViewDefaults(const TObjectPtr<UWidget>& Widget, const TObjectPtr<UWidgetBlueprintBuilder>& EntryWidgetBlueprintBuilder, int32 DesignerPreviewEntryCount)
	{
		UListViewBase* ListViewBase = Cast<UListViewBase>(Widget);
		if (!ListViewBase)
		{
			return;
		}

		UClass* EntryWidgetClass = ResolveListEntryWidgetClass(EntryWidgetBlueprintBuilder);
		if (!EntryWidgetClass)
		{
			return;
		}
		if (!EntryWidgetClass->ImplementsInterface(UUserListEntry::StaticClass()))
		{
			if (!EntryWidgetClass->ImplementsInterface(UUserObjectListEntry::StaticClass()))
			{
				UE_LOG_Figma2UMG(Warning, TEXT("[GenericWidgetBuilder] Entry widget class %s does not implement UserListEntry/UserObjectListEntry. Falling back to UFigma2UMGListEntryWidget."), *EntryWidgetClass->GetName());
				EntryWidgetClass = UFigma2UMGListEntryWidget::StaticClass();
			}
		}

		FClassProperty* EntryWidgetClassProperty = FindEntryWidgetClassProperty(ListViewBase);
		if (!EntryWidgetClassProperty)
		{
			UE_LOG_Figma2UMG(Warning, TEXT("[GenericWidgetBuilder] Could not find UListViewBase::EntryWidgetClass."));
			return;
		}

		ListViewBase->Modify();
		if (EntryWidgetBlueprintBuilder || !ListViewBase->GetEntryWidgetClass())
		{
			void* EntryWidgetClassPtr = EntryWidgetClassProperty->ContainerPtrToValuePtr<void>(ListViewBase);
			EntryWidgetClassProperty->SetObjectPropertyValue(EntryWidgetClassPtr, EntryWidgetClass);
			UE_LOG_Figma2UMG(Display, TEXT("[GenericWidgetBuilder] Set %s EntryWidgetClass to %s."), *ListViewBase->GetName(), *EntryWidgetClass->GetPathName());
		}

		if (DesignerPreviewEntryCount != INDEX_NONE)
		{
			if (FIntProperty* PreviewEntriesProperty = FindFProperty<FIntProperty>(UListViewBase::StaticClass(), TEXT("NumDesignerPreviewEntries")))
			{
				PreviewEntriesProperty->SetPropertyValue_InContainer(ListViewBase, FMath::Max(DesignerPreviewEntryCount, 0));
			}
		}

		ListViewBase->RequestRefresh();
	}
}

void UGenericPanelWidgetBuilder::SetWidgetType(EFigmaUMGWidgetType InWidgetType)
{
	WidgetType = InWidgetType;
}

void UGenericPanelWidgetBuilder::PatchAndInsertWidget(TObjectPtr<UWidgetBlueprint> WidgetBlueprint, const TObjectPtr<UWidget>& WidgetToPatch)
{
	UClass* WidgetClass = GetWidgetClassChecked(WidgetType, UPanelWidget::StaticClass(), Node ? Node->GetNodeName() : FString());
	if (!WidgetClass || !WidgetBlueprint || !WidgetBlueprint->WidgetTree)
	{
		return;
	}

	const FString NodeName = Node->GetNodeName();
	const FString WidgetName = GetWidgetName();
	UPanelWidget* PatchedWidget = nullptr;
	if (WidgetToPatch && WidgetToPatch->GetClass()->IsChildOf(WidgetClass))
	{
		PatchedWidget = Cast<UPanelWidget>(WidgetToPatch);
	}

	if (!PatchedWidget)
	{
		const FName ObjectName = UFigmaImportSubsystem::MakeWidgetObjectName(WidgetBlueprint->WidgetTree, WidgetClass, WidgetName);
		PatchedWidget = NewObject<UPanelWidget>(WidgetBlueprint->WidgetTree, WidgetClass, ObjectName);
		if (UPanelWidget* ExistingPanel = Cast<UPanelWidget>(WidgetToPatch))
		{
			while (ExistingPanel->GetChildrenCount() > 0)
			{
				PatchedWidget->AddChild(ExistingPanel->GetChildAt(0));
			}
		}
		else if (WidgetToPatch)
		{
			PatchedWidget->AddChild(WidgetToPatch);
		}
	}
	else
	{
		UFigmaImportSubsystem::TryRenameWidget(WidgetName, PatchedWidget);
	}

	Widget = PatchedWidget;
	Insert(WidgetBlueprint->WidgetTree, WidgetToPatch, Widget);
	PatchAndInsertChildren(WidgetBlueprint, Widget);
	Setup();
}

void UGenericPanelWidgetBuilder::Setup() const
{
}

void UGenericContentWidgetBuilder::SetWidgetType(EFigmaUMGWidgetType InWidgetType)
{
	WidgetType = InWidgetType;
}

void UGenericContentWidgetBuilder::PatchAndInsertWidget(TObjectPtr<UWidgetBlueprint> WidgetBlueprint, const TObjectPtr<UWidget>& WidgetToPatch)
{
	UClass* WidgetClass = GetWidgetClassChecked(WidgetType, UContentWidget::StaticClass(), Node ? Node->GetNodeName() : FString());
	if (!WidgetClass || !WidgetBlueprint || !WidgetBlueprint->WidgetTree)
	{
		return;
	}

	const FString NodeName = Node->GetNodeName();
	const FString WidgetName = GetWidgetName();
	UContentWidget* PatchedWidget = nullptr;
	if (WidgetToPatch && WidgetToPatch->GetClass()->IsChildOf(WidgetClass))
	{
		PatchedWidget = Cast<UContentWidget>(WidgetToPatch);
	}

	if (!PatchedWidget)
	{
		const FName ObjectName = UFigmaImportSubsystem::MakeWidgetObjectName(WidgetBlueprint->WidgetTree, WidgetClass, WidgetName);
		PatchedWidget = NewObject<UContentWidget>(WidgetBlueprint->WidgetTree, WidgetClass, ObjectName);
		if (UContentWidget* ExistingContentWidget = Cast<UContentWidget>(WidgetToPatch))
		{
			PatchedWidget->SetContent(ExistingContentWidget->GetContent());
		}
		else if (WidgetToPatch)
		{
			PatchedWidget->SetContent(WidgetToPatch);
		}
	}
	else
	{
		UFigmaImportSubsystem::TryRenameWidget(WidgetName, PatchedWidget);
	}

	Widget = PatchedWidget;
	Insert(WidgetBlueprint->WidgetTree, WidgetToPatch, Widget);
	PatchAndInsertChild(WidgetBlueprint, Widget);
}

void UGenericContentWidgetBuilder::SetWidget(const TObjectPtr<UWidget>& InWidget)
{
	Widget = Cast<UContentWidget>(InWidget);
	SetChildWidget(Widget);
}

void UGenericContentWidgetBuilder::ResetWidget()
{
	Super::ResetWidget();
	Widget = nullptr;
}

TObjectPtr<UContentWidget> UGenericContentWidgetBuilder::GetContentWidget() const
{
	return Widget;
}

void UGenericLeafWidgetBuilder::SetWidgetType(EFigmaUMGWidgetType InWidgetType)
{
	WidgetType = InWidgetType;
}

void UGenericLeafWidgetBuilder::SetListEntryWidgetBlueprintBuilder(const TObjectPtr<UWidgetBlueprintBuilder>& InWidgetBlueprintBuilder)
{
	ListEntryWidgetBlueprintBuilder = InWidgetBlueprintBuilder;
}

void UGenericLeafWidgetBuilder::SetDesignerPreviewEntryCount(int32 InDesignerPreviewEntryCount)
{
	DesignerPreviewEntryCount = InDesignerPreviewEntryCount;
}

void UGenericLeafWidgetBuilder::SetVisualTexture2DBuilder(const TObjectPtr<UTexture2DBuilder>& InTexture2DBuilder)
{
	VisualTexture2DBuilder = InTexture2DBuilder;
	VisualTexture = nullptr;
}

void UGenericLeafWidgetBuilder::SetVisualTexture(const TObjectPtr<UTexture2D>& InTexture)
{
	VisualTexture = InTexture;
	VisualTexture2DBuilder = nullptr;
}

void UGenericLeafWidgetBuilder::SetCheckedVisualTexture2DBuilder(const TObjectPtr<UTexture2DBuilder>& InTexture2DBuilder)
{
	CheckedVisualTexture2DBuilder = InTexture2DBuilder;
	CheckedVisualTexture = nullptr;
}

void UGenericLeafWidgetBuilder::SetCheckedVisualTexture(const TObjectPtr<UTexture2D>& InTexture)
{
	CheckedVisualTexture = InTexture;
	CheckedVisualTexture2DBuilder = nullptr;
}

void UGenericLeafWidgetBuilder::SetUncheckedVisualTexture2DBuilder(const TObjectPtr<UTexture2DBuilder>& InTexture2DBuilder)
{
	UncheckedVisualTexture2DBuilder = InTexture2DBuilder;
	UncheckedVisualTexture = nullptr;
}

void UGenericLeafWidgetBuilder::SetUncheckedVisualTexture(const TObjectPtr<UTexture2D>& InTexture)
{
	UncheckedVisualTexture = InTexture;
	UncheckedVisualTexture2DBuilder = nullptr;
}

void UGenericLeafWidgetBuilder::PatchAndInsertWidget(TObjectPtr<UWidgetBlueprint> WidgetBlueprint, const TObjectPtr<UWidget>& WidgetToPatch)
{
	UClass* WidgetClass = GetWidgetClassChecked(WidgetType, UWidget::StaticClass(), Node ? Node->GetNodeName() : FString());
	if (!WidgetClass || !WidgetBlueprint || !WidgetBlueprint->WidgetTree)
	{
		return;
	}

	const FString NodeName = Node->GetNodeName();
	const FString WidgetName = Node->GetWidgetName();
	if (WidgetToPatch && WidgetToPatch->GetClass()->IsChildOf(WidgetClass))
	{
		Widget = WidgetToPatch;
		UFigmaImportSubsystem::TryRenameWidget(WidgetName, Widget);
	}
	else
	{
		const FName ObjectName = UFigmaImportSubsystem::MakeWidgetObjectName(WidgetBlueprint->WidgetTree, WidgetClass, WidgetName);
		Widget = NewObject<UWidget>(WidgetBlueprint->WidgetTree, WidgetClass, ObjectName);
	}

	Insert(WidgetBlueprint->WidgetTree, WidgetToPatch, Widget);
	Setup();
}

bool UGenericLeafWidgetBuilder::TryInsertOrReplace(const TObjectPtr<UWidget>& PrePatchWidget, const TObjectPtr<UWidget>& PostPatchWidget)
{
	UE_LOG_Figma2UMG(Warning, TEXT("[UGenericLeafWidgetBuilder::TryInsertOrReplace] Node %s is a %s and can't insert child widgets."), *Node->GetNodeName(), LexToString(WidgetType));
	return false;
}

void UGenericLeafWidgetBuilder::PatchWidgetProperties()
{
	ApplyListViewDefaults(Widget, ListEntryWidgetBlueprintBuilder, DesignerPreviewEntryCount);
}

void UGenericLeafWidgetBuilder::SetWidget(const TObjectPtr<UWidget>& InWidget)
{
	Widget = InWidget;
}

TObjectPtr<UWidget> UGenericLeafWidgetBuilder::GetWidget() const
{
	return Widget;
}

void UGenericLeafWidgetBuilder::ResetWidget()
{
	Widget = nullptr;
}

void UGenericLeafWidgetBuilder::Setup() const
{
	ApplyTextIfSupported(Node, Widget);
	ApplyListViewDefaults(Widget, ListEntryWidgetBlueprintBuilder, DesignerPreviewEntryCount);

	if (USpacer* Spacer = Cast<USpacer>(Widget))
	{
		Spacer->SetSize(Figma2UMGLayout::RoundLayoutVector(Node->GetAbsoluteSize(IsTopWidgetForNode())));
	}

	if (UCheckBox* CheckBox = Cast<UCheckBox>(Widget))
	{
		FSlateBrush CheckedBrush;
		bool bHasCheckedBrush = Figma2UMGBrush::MakeTextureBrush(Node, CheckedVisualTexture2DBuilder.Get(), CheckedVisualTexture.Get(), CheckedBrush);
		if (!bHasCheckedBrush)
		{
			bHasCheckedBrush = Figma2UMGBrush::MakeTextureBrush(Node, VisualTexture2DBuilder.Get(), VisualTexture.Get(), CheckedBrush);
		}

		FSlateBrush UncheckedBrush;
		bool bHasUncheckedBrush = Figma2UMGBrush::MakeTextureBrush(Node, UncheckedVisualTexture2DBuilder.Get(), UncheckedVisualTexture.Get(), UncheckedBrush);
		if (!bHasUncheckedBrush)
		{
			bHasUncheckedBrush = Figma2UMGBrush::MakeTextureBrush(Node, VisualTexture2DBuilder.Get(), VisualTexture.Get(), UncheckedBrush);
		}

		if (bHasCheckedBrush && !bHasUncheckedBrush)
		{
			UncheckedBrush = CheckedBrush;
			bHasUncheckedBrush = true;
		}
		else if (!bHasCheckedBrush && bHasUncheckedBrush)
		{
			CheckedBrush = UncheckedBrush;
			bHasCheckedBrush = true;
		}

		if (bHasCheckedBrush && bHasUncheckedBrush)
		{
			FCheckBoxStyle Style = CheckBox->GetWidgetStyle();
			Style.SetUncheckedImage(UncheckedBrush);
			Style.SetUncheckedHoveredImage(UncheckedBrush);
			Style.SetUncheckedPressedImage(UncheckedBrush);
			Style.SetCheckedImage(CheckedBrush);
			Style.SetCheckedHoveredImage(CheckedBrush);
			Style.SetCheckedPressedImage(CheckedBrush);
			Style.SetUndeterminedImage(CheckedBrush);
			Style.SetUndeterminedHoveredImage(CheckedBrush);
			Style.SetUndeterminedPressedImage(CheckedBrush);
			Style.SetPadding(FMargin(0.0f));
			CheckBox->SetWidgetStyle(Style);
		}
	}
}

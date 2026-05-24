#pragma once

#include "CoreMinimal.h"
#include "WidgetBuilder.h"

#include "ProgressBarWidgetBuilder.generated.h"

class UProgressBar;
class UFigmaNode;

UCLASS()
class FIGMA2UMG_API UProgressBarWidgetBuilder : public UObject, public IWidgetBuilder
{
public:
	GENERATED_BODY()

	virtual void PatchAndInsertWidget(TObjectPtr<UWidgetBlueprint> WidgetBlueprint, const TObjectPtr<UWidget>& WidgetToPatch) override;
	virtual bool TryInsertOrReplace(const TObjectPtr<UWidget>& PrePatchWidget, const TObjectPtr<UWidget>& PostPatchWidget) override;

	virtual void SetWidget(const TObjectPtr<UWidget>& InWidget) override;
	virtual TObjectPtr<UWidget> GetWidget() const override;
	virtual void ResetWidget() override;

protected:
	void Setup() const;
	const UFigmaNode* FindFillNode() const;
	FLinearColor GetTrackColor() const;
	FLinearColor GetFillColor() const;
	float GetInitialPercent() const;

	UPROPERTY()
	TObjectPtr<UProgressBar> Widget = nullptr;
};

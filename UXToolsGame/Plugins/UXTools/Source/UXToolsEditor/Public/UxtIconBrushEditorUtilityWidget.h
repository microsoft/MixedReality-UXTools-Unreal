// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"

#include "UxtIconBrushEditorUtilityWidget.generated.h"

UCLASS(ClassGroup = "UXToolsEditor")
class UUxtIconBrushEditorUtilityWidget : public UEditorUtilityWidget
{
	GENERATED_BODY()

public:
	/** Sets the unique identifier for the tab this EditorUtilityWidget was spawned into. */
	void SetTabID(FName ID) { TabID = ID; }

	/** Sets the property handle which will be queried and altered for FUxtIconBrush properties. */
	void SetPropertyHandle(TSharedRef<IPropertyHandle> InPropertyHandle)
	{
		PropertyHandle = InPropertyHandle;
		OnPropertyHandleChanged();
	}

	/** Returns true if a non-null PropertyHandle is set. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Icon Brush Editor Utility Widget")
	bool HasValidPropertyHandle() const { return PropertyHandle.IsValid(); }

protected:
	/** Event which triggers when the PropertyHandle is changed. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Uxt Icon Brush Editor Utility Widget")
	void OnPropertyHandleChanged();

	/** Gets the font associated with the UUxtIconBrush. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Icon Brush Editor Utility Widget")
	UFont* GetIconBrushFont() const;

	/** Sets the font associated with the UUxtIconBrush. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Icon Brush Editor Utility Widget")
	bool SetIconBrushFont(const UFont* Font);

	/** Gets the string associated with the UUxtIconBrush. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Icon Brush Editor Utility Widget")
	bool GetIconBrushString(FString& IconString) const;

	/** Sets the string associated with the UUxtIconBrush. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Icon Brush Editor Utility Widget")
	bool SetIconBrushString(const FString& IconString);

	/** Gets unique identifier for the tab this EditorUtilityWidget was spawned into. */
	UPROPERTY(BlueprintReadOnly, Category = "Uxt Icon Brush Editor Utility Widget")
	FName TabID;

private:
	TWeakPtr<IPropertyHandle> PropertyHandle;
};

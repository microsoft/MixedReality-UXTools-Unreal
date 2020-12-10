// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "GameFramework/Actor.h"

#include "UxtTextRenderActor.generated.h"

class UUxtTextRenderComponent;

/**
 * A text render actor which automatically wraps the UUxtTextRenderComponent.
 */
UCLASS(ClassGroup = "UXTools", ComponentWrapperClass, hideCategories = (Collision, Attachment, Actor))
class UXTOOLS_API AUxtTextRenderActor : public AActor
{
	GENERATED_UCLASS_BODY()

private:
	/** Component to render a text in 3d with a font */
	UPROPERTY(
		VisibleAnywhere, BlueprintReadOnly, Category = "Uxt Text Render",
		meta = (ExposeFunctionCategories = "Rendering|Components|TextRender", AllowPrivateAccess = "true"))
	UUxtTextRenderComponent* TextRender;

public:
	/** Returns TextRender subobject **/
	class UUxtTextRenderComponent* GetTextRender() const { return TextRender; }
};

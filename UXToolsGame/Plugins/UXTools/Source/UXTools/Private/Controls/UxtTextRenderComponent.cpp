// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtTextRenderComponent.h"

#include "Controls/UxtTextRenderActor.h"
#include "Engine/Font.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AUxtTextRenderActor::AUxtTextRenderActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	TextRender = CreateDefaultSubobject<UUxtTextRenderComponent>(TEXT("NewTextRenderComponent"));
	RootComponent = TextRender;
}

UUxtTextRenderComponent::UUxtTextRenderComponent()
{
	// Apply default assets.
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(TEXT("/UXTools/Fonts/M_DefaultFont"));
	check(MaterialFinder.Object);
	SetTextMaterial(MaterialFinder.Object);

	static ConstructorHelpers::FObjectFinder<UFont> FontFinder(TEXT("/UXTools/Fonts/Font_SegoeUI_Semibold_42"));
	check(FontFinder.Object);
	SetFont(FontFinder.Object);

	// Apply default text properties.
	SetWorldSize(1);
	SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
	SetText(INVTEXT("UX Tools Text"));
}

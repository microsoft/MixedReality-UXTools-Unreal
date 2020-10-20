// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtBasePressableButtonActor.h"

#include "Controls/UxtPressableButtonComponent.h"

AUxtBasePressableButtonActor::AUxtBasePressableButtonActor()
{
	ButtonComponent = CreateDefaultSubobject<UUxtPressableButtonComponent>(TEXT("UxtPressableButton"));
	RootComponent = ButtonComponent;
}

TScriptInterface<IUxtPokeTarget> AUxtBasePressableButtonActor::GetPokeTarget_Implementation()
{
	return GetButtonComponent();
}

TScriptInterface<IUxtFarTarget> AUxtBasePressableButtonActor::GetFarTarget_Implementation()
{
	return GetButtonComponent();
}

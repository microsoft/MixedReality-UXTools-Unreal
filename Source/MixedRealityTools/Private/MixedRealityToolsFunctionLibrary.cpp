// Fill out your copyright notice in the Description page of Project Settings.


#include "MixedRealityToolsFunctionLibrary.h"
#include "AudioDevice.h"


bool UMixedRealityToolsFunctionLibrary::IsInVR()
{
	return FAudioDevice::CanUseVRAudioDevice();
}
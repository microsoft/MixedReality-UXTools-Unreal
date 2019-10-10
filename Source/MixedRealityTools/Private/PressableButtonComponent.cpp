// Fill out your copyright notice in the Description page of Project Settings.

#include "PressableButtonComponent.h"
#include <GameFramework/Actor.h>
#include <DrawDebugHelpers.h>

using namespace Microsoft::MixedReality::HandUtils;
using namespace DirectX;


// Sets default values for this component's properties
UPressableButtonComponent::UPressableButtonComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = ETickingGroup::TG_PostPhysics;

	Width = 10;
	Height = 10;
	MaxPushDistance = 10;
	PressedDistance = 5;
	ReleasedDistance = 2;
}

static XMVECTOR ToXM(const FVector& vectorUE)
{
	return XMLoadFloat3((const XMFLOAT3*)&vectorUE);
}

static XMVECTOR ToXM(const FQuat& quaternion)
{
	return XMLoadFloat4A((const XMFLOAT4A*)&quaternion);
}

static FVector ToUE(XMVECTOR vectorXM)
{
	FVector vectorUE;
	XMStoreFloat3((XMFLOAT3*)&vectorUE, vectorXM);
	return vectorUE;
}

static FVector ToUEPosition(XMVECTOR vectorXM)
{
	return ToUE(XMVectorSwizzle<2, 0, 1, 3>(vectorXM) * g_XMNegateX);
}

static XMVECTOR ToMRPosition(const FVector& vectorUE)
{
	auto vectorXM = ToXM(vectorUE);
	return XMVectorSwizzle<1, 2, 0, 3>(vectorXM) * g_XMNegateZ;
}

static FQuat ToUERotation(XMVECTOR quaternionXM)
{
	FQuat quaternionUE;
	XMStoreFloat4A((XMFLOAT4A*)&quaternionUE, XMVectorSwizzle<2, 0, 1, 3>(quaternionXM) * g_XMNegateW);
	return quaternionUE;
}

static XMVECTOR ToMRRotation(const FQuat& quatUE)
{
	auto quatXM = ToXM(quatUE);
	return XMVectorSwizzle<1, 2, 0, 3>(quatXM) * g_XMNegateW;
}

void UPressableButtonComponent::FButtonHandler::OnButtonPressed(PressableButton& button, PointerId pointerId, DirectX::FXMVECTOR touchPoint)
{
	PressableButtonComponent.PressedEvent.Broadcast(PressableButtonComponent);
}

// Called when the game starts
void UPressableButtonComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* owner = GetOwner();
	FTransform transform = owner->GetActorTransform();
	XMVECTOR restPosition = ToMRPosition(transform.GetTranslation());
	XMVECTOR orientation = ToMRRotation(transform.GetRotation());
	Button = new PressableButton(restPosition, orientation, Width, Height, MaxPushDistance, PressedDistance, ReleasedDistance);
	Button->m_recoverySpeed = 50;

	ButtonHandler = new FButtonHandler(*this);
	Button->Subscribe(ButtonHandler);
}


void UPressableButtonComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Button->Unsubscribe(ButtonHandler);
	delete ButtonHandler;
	ButtonHandler = nullptr;
	delete Button;
	Button = nullptr;

	Super::EndPlay(EndPlayReason);
}

// Called every frame
void UPressableButtonComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (Pointer)
	{
		AActor* Owner = GetOwner();
		/*FVector NewLocation = Owner->GetActorLocation();
		NewLocation.Y = Pointer->GetActorLocation().Y;*/

		TouchPointer touchPointer;
		touchPointer.m_position = ToMRPosition(Pointer->GetActorLocation());
		touchPointer.m_id = (PointerId)Pointer;

		Button->Update(DeltaTime, { &touchPointer, 1 });

		FVector NewLocation = ToUEPosition(Button->GetCurrentPosition());
		Owner->SetActorLocation(NewLocation, false, nullptr, ETeleportType::TeleportPhysics);
	}

	// Debug display
	{
		FVector Position = ToUEPosition(Button->GetCurrentPosition());
		FQuat Orientation = ToUERotation(Button->GetOrientation());
		FPlane Plane(Position, -Orientation.GetForwardVector());
		FVector2D Extents(Width, Height);
		DrawDebugSolidPlane(GetWorld(), Plane, Position, Extents, FColor::Green);
	}
}


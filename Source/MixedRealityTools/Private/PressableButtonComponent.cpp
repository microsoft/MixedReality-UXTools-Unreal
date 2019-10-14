// Fill out your copyright notice in the Description page of Project Settings.

#include "PressableButtonComponent.h"
#include "PressableButton.h"
#include <GameFramework/Actor.h>
#include <DrawDebugHelpers.h>

namespace HandUtils = Microsoft::MixedReality::HandUtils;
using namespace DirectX;


// Sets default values for this component's properties
UPressableButtonComponent::UPressableButtonComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = ETickingGroup::TG_PostPhysics;

	Extents.Set(10, 10, 10);
	PressedFraction = 0.5f;
	ReleasedFraction = 0.2f;
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
	XMStoreFloat4A((XMFLOAT4A*)&quaternionUE, XMVectorSwizzle<2, 0, 1, 3>(quaternionXM) * g_XMNegateY * g_XMNegateZ);
	return quaternionUE;
}

static XMVECTOR ToMRRotation(const FQuat& quatUE)
{
	auto quatXM = ToXM(quatUE);
	return XMVectorSwizzle<1, 2, 0, 3>(quatXM) * g_XMNegateX * g_XMNegateY;
}

struct FButtonHandler : public Microsoft::MixedReality::HandUtils::IButtonHandler
{
	FButtonHandler(UPressableButtonComponent& PressableButtonComponent) : PressableButtonComponent(PressableButtonComponent) {}

	virtual void OnButtonPressed(
		Microsoft::MixedReality::HandUtils::PressableButton& button,
		Microsoft::MixedReality::HandUtils::PointerId pointerId,
		DirectX::FXMVECTOR touchPoint) override;

	virtual void OnButtonReleased(
		Microsoft::MixedReality::HandUtils::PressableButton& button,
		Microsoft::MixedReality::HandUtils::PointerId pointerId) override;

	UPressableButtonComponent& PressableButtonComponent;
};

void FButtonHandler::OnButtonPressed(HandUtils::PressableButton& button, HandUtils::PointerId pointerId, DirectX::FXMVECTOR touchPoint)
{
	PressableButtonComponent.ButtonPressed.Broadcast(&PressableButtonComponent);
}

void FButtonHandler::OnButtonReleased(HandUtils::PressableButton& button, HandUtils::PointerId pointerId)
{
	PressableButtonComponent.ButtonReleased.Broadcast(&PressableButtonComponent);
}

// Called when the game starts
void UPressableButtonComponent::BeginPlay()
{
	Super::BeginPlay();

	const FTransform& Transform = GetComponentTransform();
	const auto WorldDimensions = 2 * Extents * Transform.GetScale3D();
	XMVECTOR Orientation = ToMRRotation(Transform.GetRotation());
	const auto RestPosition = Transform.GetTranslation();

	Button = new HandUtils::PressableButton(ToMRPosition(RestPosition), Orientation, WorldDimensions.Y, WorldDimensions.Z, WorldDimensions.X, PressedFraction * WorldDimensions.X, ReleasedFraction * WorldDimensions.X);
	Button->m_recoverySpeed = 50;

	ButtonHandler = new FButtonHandler(*this);
	Button->Subscribe(ButtonHandler);

	if (Visuals)
	{
		VisualsPositionLocal = Visuals->GetActorLocation() - RestPosition;
	}
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

    std::vector<HandUtils::TouchPointer> touchPointers;
    touchPointers.reserve(GetActivePointers().Num());
    for (const TWeakObjectPtr<USceneComponent>& wPointer : GetActivePointers())
    {
        if (USceneComponent *pointer = wPointer.Get())
        {
            HandUtils::TouchPointer touchPointer;
            touchPointer.m_position = ToMRPosition(pointer->GetComponentLocation());
            touchPointer.m_id = (HandUtils::PointerId)pointer;
            touchPointers.emplace_back(touchPointer);
        }
    }

	if (Pointer)
	{
        HandUtils::TouchPointer touchPointer;
		touchPointer.m_position = ToMRPosition(Pointer->GetActorLocation());
		touchPointer.m_id = (HandUtils::PointerId)Pointer;
        touchPointers.emplace_back(touchPointer);
	}

	// Update button logic with all known pointers
	Button->Update(DeltaTime, touchPointers);

	if (Visuals)
	{
		// Update visuals position
		FVector NewLocation = ToUEPosition(Button->GetCurrentPosition()) + VisualsPositionLocal;
		Visuals->SetActorLocation(NewLocation);
	}
}

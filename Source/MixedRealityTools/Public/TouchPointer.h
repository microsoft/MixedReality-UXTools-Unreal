// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "TouchPointer.generated.h"


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MIXEDREALITYTOOLS_API UTouchPointer : public USceneComponent
{
    GENERATED_BODY()

public:
    // Sets default values for this component's properties
    UTouchPointer();

    UPROPERTY(BlueprintSetter = "SetTouchRadius")
    float TouchRadius;
    UFUNCTION(BlueprintCallable)
    void SetTouchRadius(float radius);

protected:

	/// Start touching the component.
	/// Returns false if the component is not a valid touch target.
	bool TryStartTouching(UActorComponent *comp);
	/// Stop touching the component.
	/// Returns false if the component was not touched.
	bool StopTouching(UActorComponent *comp);
	/// Stop touching all current components.
	void StopAllTouching();

	UFUNCTION()
	void OnActorBeginOverlap(AActor* OverlappedActor, AActor* OtherActor);
	UFUNCTION()
	void OnActorEndOverlap(AActor* OverlappedActor, AActor* OtherActor);

	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Activate(bool bReset) override;
	virtual void Deactivate() override;

private:

	bool ImplementsTargetInterface(const UActorComponent *comp) const;

protected:

	TSet<TWeakObjectPtr<UActorComponent>> TouchedTargets;

private:

	// Delegates for handling overlap events
	FScriptDelegate m_beginOverlapDelegate;
	FScriptDelegate m_endOverlapDelegate;

	USphereComponent* m_touchSphere;

};

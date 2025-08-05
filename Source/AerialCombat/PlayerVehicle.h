// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"

#include "PlayerVehicle.generated.h"

UCLASS()
class AERIALCOMBAT_API APlayerVehicle : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	APlayerVehicle();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle")
	float AscentAcceleration = 200.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle")
	float MaxAscentVelocity= 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle")
	float DescentAcceleration = -100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle")
	float MaxDescentVelocity = -150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle")
	float MovementAcceleration = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle")
	float MaxMovementVelocity = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle")
	float DecelerateLerpFactor = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle")
	float TurningTorque = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle")
	float MaxTurningSpeed = 40.0f;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle | HoverWobble")
	float WobbleAmplitude = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle | HoverWobble")
	float WobbleDecayConst = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle | HoverWobble")
	float WobbleFrequency = 2.5f;


	bool bShouldHover = false;
	bool bMoving = false;
	bool bTurning = false;

	float GameGravity = 980.0f;

	float HoverTime = 0.0f;
	float MaxVelAchieved = 0.0f;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


	// Component References
	UStaticMeshComponent* MeshComp;
	class USpringArmComponent* SpringArmComp;

private:

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputMappingContext* InputMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* LookInputAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* AscendInputAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* DescendInputAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* MoveForwardInputAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* MoveBackwardInputAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* TurnLeftInputAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* TurnRightInputAction;


public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// **
	
	// Input Functions
	UFUNCTION()
	void Look(const FInputActionValue& Value);

	UFUNCTION()
	void Ascend(const FInputActionValue& Value);

	UFUNCTION()
	void StopAscending(const FInputActionValue& Value);

	UFUNCTION()
	void Descend(const FInputActionValue& Value);

	UFUNCTION()
	void StopDescending(const FInputActionValue& Value);

	UFUNCTION()
	void MoveForward(const FInputActionValue& Value);

	UFUNCTION()
	void MoveBackward(const FInputActionValue& Value);

	UFUNCTION()
	void StopMoving(const FInputActionValue& Value);

	UFUNCTION()
	void TurnLeft(const FInputActionValue& Value);

	UFUNCTION()
	void TurnRight(const FInputActionValue& Value);

	UFUNCTION()
	void StopTurning(const FInputActionValue& Value);


	// Hovering
	UFUNCTION()
	void Hover(float DeltaTime);

	// Decelerating Forward/Backward Movement and Rotation
	UFUNCTION()
	void Decelerate(float DeltaTime);
};

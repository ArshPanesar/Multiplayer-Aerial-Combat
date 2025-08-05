// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerVehicle.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/InputComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

// Sets default values
APlayerVehicle::APlayerVehicle()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void APlayerVehicle::BeginPlay()
{
	Super::BeginPlay();
	
	// Setup Enhanced Input
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(InputMappingContext, 0);
		}
	}

	// Setup Component Refs
	MeshComp = FindComponentByClass<UStaticMeshComponent>();
	check(MeshComp != nullptr);

	SpringArmComp = FindComponentByClass<USpringArmComponent>();
	check(SpringArmComp != nullptr);
	SpringArmComp->bUsePawnControlRotation = true; // Rotate with Mouse

	bShouldHover = false;

	HoverTime = 0.0f;

	bReplicates = true;

	/*GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow,
		FString::Printf(TEXT("Role: %d"), (int)GetLocalRole()));*/

	//GameGravity = GetWorld()->GetGravityZ();
}

// Called every frame
void APlayerVehicle::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Decelerate(DeltaTime);
	Hover(DeltaTime);
}

// Called to bind functionality to input
void APlayerVehicle::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Mouse Look Around
		EnhancedInputComponent->BindAction(LookInputAction, ETriggerEvent::Triggered, this, &APlayerVehicle::Look);
		
		// Ascending
		EnhancedInputComponent->BindAction(AscendInputAction, ETriggerEvent::Triggered, this, &APlayerVehicle::Ascend);
		EnhancedInputComponent->BindAction(AscendInputAction, ETriggerEvent::Completed, this, &APlayerVehicle::StopAscending);

		// Descending
		EnhancedInputComponent->BindAction(DescendInputAction, ETriggerEvent::Triggered, this, &APlayerVehicle::Descend);
		EnhancedInputComponent->BindAction(DescendInputAction, ETriggerEvent::Completed, this, &APlayerVehicle::StopDescending);

		// Moving Forward/Backward
		EnhancedInputComponent->BindAction(MoveForwardInputAction, ETriggerEvent::Triggered, this, &APlayerVehicle::MoveForward);
		EnhancedInputComponent->BindAction(MoveBackwardInputAction, ETriggerEvent::Triggered, this, &APlayerVehicle::MoveBackward);
		EnhancedInputComponent->BindAction(MoveForwardInputAction, ETriggerEvent::Completed, this, &APlayerVehicle::StopMoving);
		EnhancedInputComponent->BindAction(MoveBackwardInputAction, ETriggerEvent::Completed, this, &APlayerVehicle::StopMoving);
		
		// Turning
		EnhancedInputComponent->BindAction(TurnLeftInputAction, ETriggerEvent::Triggered, this, &APlayerVehicle::TurnLeft);
		EnhancedInputComponent->BindAction(TurnRightInputAction, ETriggerEvent::Triggered, this, &APlayerVehicle::TurnRight);
		EnhancedInputComponent->BindAction(TurnLeftInputAction, ETriggerEvent::Completed, this, &APlayerVehicle::StopTurning);
		EnhancedInputComponent->BindAction(TurnRightInputAction, ETriggerEvent::Completed, this, &APlayerVehicle::StopTurning);
	}
}

void APlayerVehicle::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxisValue = Value.Get<FVector2D>();

	if (Controller)
	{
		AddControllerYawInput(LookAxisValue.X);
		AddControllerPitchInput(LookAxisValue.Y);
	}
}

void APlayerVehicle::Ascend(const FInputActionValue& Value)
{
	if (GetVelocity().Z > MaxAscentVelocity)
	{
		MeshComp->SetPhysicsLinearVelocity(FVector(GetVelocity().X, GetVelocity().Y, MaxAscentVelocity));
	}
	else
	{
		MeshComp->AddForce(FVector(0.0f, 0.0f, GameGravity + AscentAcceleration), NAME_None, true);
	}
	bShouldHover = false;

	//GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red,
	//	FString::Printf(TEXT("ZVel: %f , Height: %f"), GetVelocity().Z, GetActorLocation().Z));
}

void APlayerVehicle::StopAscending(const FInputActionValue& Value)
{
	bShouldHover = true;
	MaxVelAchieved = GetVelocity().Z;
	HoverTime = 0.0f;
}

void APlayerVehicle::Descend(const FInputActionValue& Value)
{
	if (GetVelocity().Z < MaxDescentVelocity)
	{
		MeshComp->SetPhysicsLinearVelocity(FVector(GetVelocity().X, GetVelocity().Y, MaxDescentVelocity));
	}
	else
	{
		MeshComp->AddForce(FVector(0.0f, 0.0f, GameGravity + DescentAcceleration), NAME_None, true);
	}
	bShouldHover = false;

	/*GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Blue,
		FString::Printf(TEXT("ZVel: %f , Height: %f"), GetVelocity().Z, GetActorLocation().Z));*/

}

void APlayerVehicle::StopDescending(const FInputActionValue& Value)
{
	bShouldHover = true;
	MaxVelAchieved = GetVelocity().Z;
	HoverTime = 0.0f;
}

void APlayerVehicle::MoveForward(const FInputActionValue& Value)
{
	// Clamp Velocity
	FVector Vel(GetVelocity());
	Vel.Z = 0.0f;
	if (Vel.SquaredLength() > MaxMovementVelocity * MaxMovementVelocity)
	{
		Vel = MaxMovementVelocity * GetActorForwardVector();
		MeshComp->SetPhysicsLinearVelocity(FVector(Vel.X, Vel.Y, GetVelocity().Z));
	}
	else
	{
		// Apply Acceleration
		FVector Acc = MovementAcceleration * GetActorForwardVector();
		MeshComp->AddForce(Acc, NAME_None, true);
	}

	bMoving = true;
	/*GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Blue,
		FString::Printf(TEXT("VelSq: %f"), GetVelocity().SquaredLength()));*/
}

void APlayerVehicle::MoveBackward(const FInputActionValue& Value)
{
	// Clamp Velocity
	FVector Vel(GetVelocity());
	Vel.Z = 0.0f;
	if (Vel.SquaredLength() > MaxMovementVelocity * MaxMovementVelocity)
	{
		Vel = -MaxMovementVelocity * GetActorForwardVector();
		MeshComp->SetPhysicsLinearVelocity(FVector(Vel.X, Vel.Y, GetVelocity().Z));
	}
	else
	{
		// Apply Acceleration
		FVector Acc = -MovementAcceleration * GetActorForwardVector();
		MeshComp->AddForce(Acc, NAME_None, true);
	}

	bMoving = true;
	/*GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red,
		FString::Printf(TEXT("VelSq: %f"), GetVelocity().SquaredLength()));*/
}

void APlayerVehicle::StopMoving(const FInputActionValue& Value)
{
	bMoving = false;
}

void APlayerVehicle::TurnLeft(const FInputActionValue& Value)
{
	if (MeshComp->GetPhysicsAngularVelocityInDegrees().SquaredLength() > MaxTurningSpeed * MaxTurningSpeed)
	{
		MeshComp->SetPhysicsAngularVelocityInDegrees(FVector(0.0f, 0.0f, -TurningTorque));
	}
	else
	{
		MeshComp->AddTorqueInDegrees(FVector(0.0f, 0.0f, -TurningTorque), NAME_None, true);
	}

	bTurning = true;
}

void APlayerVehicle::TurnRight(const FInputActionValue& Value)
{
	if (MeshComp->GetPhysicsAngularVelocityInDegrees().Z > MaxTurningSpeed)
	{
		MeshComp->SetPhysicsAngularVelocityInDegrees(FVector(0.0f, 0.0f, TurningTorque));
	}
	else
	{
		MeshComp->AddTorqueInDegrees(FVector(0.0f, 0.0f, TurningTorque), NAME_None, true);
	}

	bTurning = true;
}

void APlayerVehicle::StopTurning(const FInputActionValue& Value)
{
	bTurning = false;
}

void APlayerVehicle::Hover(float DeltaTime)
{
	if (bShouldHover)
	{
		HoverTime += DeltaTime;

		FVector Vel = GetVelocity();

		// Damped Sin Wave
		Vel.Z = WobbleAmplitude * FMath::Exp(-WobbleDecayConst * HoverTime) * FMath::Cos(WobbleFrequency * HoverTime) * MaxVelAchieved;

		MeshComp->SetPhysicsLinearVelocity(Vel);
		MeshComp->AddForce(FVector(0.0f, 0.0f, GameGravity), NAME_None, true);

		/*GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Yellow,
			FString::Printf(TEXT("ZVel: %f"), VelZ));*/

		/*GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Yellow, 
		FString::Printf(TEXT("ZVel: %f , Height: %f"), GetVelocity().Z, GetActorLocation().Z));*/
	}
}

void APlayerVehicle::Decelerate(float DeltaTime)
{
	if (!bMoving)
	{
		FVector Vel = GetVelocity();
		Vel.X = FMath::Lerp(Vel.X, 0.0f, DeltaTime * DecelerateLerpFactor);
		Vel.Y = FMath::Lerp(Vel.Y, 0.0f, DeltaTime * DecelerateLerpFactor);
		MeshComp->SetPhysicsLinearVelocity(Vel);
	}

	if (!bTurning)
	{
		FVector Vel = MeshComp->GetPhysicsAngularVelocityInDegrees();
		Vel.Z = FMath::Lerp(Vel.Z, 0.0f, DeltaTime * DecelerateLerpFactor);
		MeshComp->SetPhysicsAngularVelocityInDegrees(Vel);
	}
}

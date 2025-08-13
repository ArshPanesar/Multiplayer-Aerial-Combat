// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatVehicle.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/InputComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include <Net/UnrealNetwork.h>

// Sets default values
ACombatVehicle::ACombatVehicle() : NetClientPredStats()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Setup Health
	CurrentHealth = MaxHealth;

	// Initialize Fire Rate
	FireRate = 0.25f;
	bIsShooting = false;
}

// Called when the game starts or when spawned
void ACombatVehicle::BeginPlay()
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

	// Network Check
	bReplicates = true;
	bIsClient = (GetNetMode() == ENetMode::NM_Client);
}

// Called every frame
void ACombatVehicle::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Perform Physics Locally
	if (IsLocallyControlled())
	{
		Hover(DeltaTime);
		Decelerate(DeltaTime);

		// Enqueue the Resultant Move of this Tick
		NetClientPredStats.AddMove(CurrTickClientMove);
		if (!NetClientPredStats.IsMoveQueueEmpty())
		{
			RPC_Server_UpdateMovement(NetClientPredStats);
			NetClientPredStats.UpdateClient();
		}

		// Prepare for the Next Tick
		CurrTickClientMove = FNetClientMove();
	}
}

// Called to bind functionality to input
void ACombatVehicle::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Mouse Look Around
		EnhancedInputComponent->BindAction(LookInputAction, ETriggerEvent::Triggered, this, &ACombatVehicle::Look);

		// Ascending
		EnhancedInputComponent->BindAction(AscendInputAction, ETriggerEvent::Triggered, this, &ACombatVehicle::Ascend);
		EnhancedInputComponent->BindAction(AscendInputAction, ETriggerEvent::Completed, this, &ACombatVehicle::StopAscending);

		// Descending
		EnhancedInputComponent->BindAction(DescendInputAction, ETriggerEvent::Triggered, this, &ACombatVehicle::Descend);
		EnhancedInputComponent->BindAction(DescendInputAction, ETriggerEvent::Completed, this, &ACombatVehicle::StopDescending);

		// Moving Forward/Backward
		EnhancedInputComponent->BindAction(MoveForwardInputAction, ETriggerEvent::Triggered, this, &ACombatVehicle::MoveForward);
		EnhancedInputComponent->BindAction(MoveBackwardInputAction, ETriggerEvent::Triggered, this, &ACombatVehicle::MoveBackward);
		EnhancedInputComponent->BindAction(MoveForwardInputAction, ETriggerEvent::Completed, this, &ACombatVehicle::StopMoving);
		EnhancedInputComponent->BindAction(MoveBackwardInputAction, ETriggerEvent::Completed, this, &ACombatVehicle::StopMoving);

		// Turning
		EnhancedInputComponent->BindAction(TurnLeftInputAction, ETriggerEvent::Triggered, this, &ACombatVehicle::TurnLeft);
		EnhancedInputComponent->BindAction(TurnRightInputAction, ETriggerEvent::Triggered, this, &ACombatVehicle::TurnRight);
		EnhancedInputComponent->BindAction(TurnLeftInputAction, ETriggerEvent::Completed, this, &ACombatVehicle::StopTurning);
		EnhancedInputComponent->BindAction(TurnRightInputAction, ETriggerEvent::Completed, this, &ACombatVehicle::StopTurning);

		// Shooting
		EnhancedInputComponent->BindAction(FireInputAction, ETriggerEvent::Triggered, this, &ACombatVehicle::StartShooting);

	}
}


void ACombatVehicle::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxisValue = Value.Get<FVector2D>();

	if (Controller)
	{
		AddControllerYawInput(LookAxisValue.X);
		AddControllerPitchInput(LookAxisValue.Y);
	}
}

void ACombatVehicle::Ascend(const FInputActionValue& Value)
{
	if (GetVelocity().Z > MaxAscentVelocity)
	{
		CurrTickClientMove.Velocity.Z = MaxAscentVelocity;
		CurrTickClientMove.SetVelocityComp.Z = 1;
	}
	else
	{
		CurrTickClientMove.Force += FVector(0.0f, 0.0f, GameGravity + AscentAcceleration);
	}
	bShouldHover = false;

	//GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red,
	//	FString::Printf(TEXT("ZVel: %f , Height: %f"), GetVelocity().Z, GetActorLocation().Z));
}

void ACombatVehicle::StopAscending(const FInputActionValue& Value)
{
	bShouldHover = true;
	MaxVelAchieved = GetVelocity().Z;
	HoverTime = 0.0f;
}

void ACombatVehicle::Descend(const FInputActionValue& Value)
{
	if (GetVelocity().Z < MaxDescentVelocity)
	{
		CurrTickClientMove.Velocity.Z = MaxDescentVelocity;
		CurrTickClientMove.SetVelocityComp.Z = 1;
	}
	else
	{
		CurrTickClientMove.Force += FVector(0.0f, 0.0f, GameGravity + DescentAcceleration);
	}
	bShouldHover = false;


	/*GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Blue,
		FString::Printf(TEXT("ZVel: %f , Height: %f"), GetVelocity().Z, GetActorLocation().Z));*/

}

void ACombatVehicle::StopDescending(const FInputActionValue& Value)
{
	bShouldHover = true;
	MaxVelAchieved = GetVelocity().Z;
	HoverTime = 0.0f;
}

void ACombatVehicle::MoveForward(const FInputActionValue& Value)
{
	// Clamp Velocity
	FVector Vel(GetVelocity());
	Vel.Z = 0.0f;
	if (Vel.SquaredLength() > MaxMovementVelocity * MaxMovementVelocity && Vel.Dot(GetActorForwardVector()) > 0.0f)
	{
		Vel = MaxMovementVelocity * GetActorForwardVector();
		CurrTickClientMove.Velocity.X = Vel.X;
		CurrTickClientMove.Velocity.Y = Vel.Y;
		CurrTickClientMove.SetVelocityComp.X = 1;
		CurrTickClientMove.SetVelocityComp.Y = 1;
	}
	else
	{
		// Apply Acceleration
		FVector Acc = MovementAcceleration * GetActorForwardVector();
		CurrTickClientMove.Force += Acc;
	}

	bMoving = true;

	/*GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Blue,
		FString::Printf(TEXT("VelSq: %f"), GetVelocity().SquaredLength()));*/
}

void ACombatVehicle::MoveBackward(const FInputActionValue& Value)
{
	// Clamp Velocity
	FVector Vel(GetVelocity());
	Vel.Z = 0.0f;
	if (Vel.SquaredLength() > MaxMovementVelocity * MaxMovementVelocity && Vel.Dot(-GetActorForwardVector()) > 0.0f)
	{
		Vel = -MaxMovementVelocity * GetActorForwardVector();
		CurrTickClientMove.Velocity.X = Vel.X;
		CurrTickClientMove.Velocity.Y = Vel.Y;
		CurrTickClientMove.SetVelocityComp.X = 1;
		CurrTickClientMove.SetVelocityComp.Y = 1;
	}
	else
	{
		// Apply Acceleration
		FVector Acc = -MovementAcceleration * GetActorForwardVector();
		CurrTickClientMove.Force += Acc;
	}

	bMoving = true;

	/*GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red,
		FString::Printf(TEXT("VelSq: %f"), GetVelocity().SquaredLength()));*/
}

void ACombatVehicle::StopMoving(const FInputActionValue& Value)
{
	bMoving = false;
}

void ACombatVehicle::TurnLeft(const FInputActionValue& Value)
{
	if (MeshComp->GetPhysicsAngularVelocityInDegrees().SquaredLength() > MaxTurningSpeed * MaxTurningSpeed)
	{
		CurrTickClientMove.AngVelocity = FVector(0.0f, 0.0f, -MaxTurningSpeed);
		CurrTickClientMove.bSetAngVelocity = true;
	}
	else
	{
		CurrTickClientMove.Torque += FVector(0.0f, 0.0f, -TurningTorque);
	}

	bTurning = true;
}

void ACombatVehicle::TurnRight(const FInputActionValue& Value)
{
	if (MeshComp->GetPhysicsAngularVelocityInDegrees().Z > MaxTurningSpeed)
	{
		CurrTickClientMove.AngVelocity = FVector(0.0f, 0.0f, MaxTurningSpeed);
		CurrTickClientMove.bSetAngVelocity = true;
	}
	else
	{
		CurrTickClientMove.Torque += FVector(0.0f, 0.0f, TurningTorque);
	}

	bTurning = true;
}

void ACombatVehicle::StopTurning(const FInputActionValue& Value)
{
	bTurning = false;
}

void ACombatVehicle::Hover(float DeltaTime)
{
	if (bShouldHover)
	{
		HoverTime += DeltaTime;

		FVector Vel = GetVelocity();

		// Damped Sin Wave
		Vel.Z = WobbleAmplitude * FMath::Exp(-WobbleDecayConst * HoverTime) * FMath::Cos(WobbleFrequency * HoverTime) * MaxVelAchieved;

		CurrTickClientMove.Velocity.Z = Vel.Z;
		CurrTickClientMove.SetVelocityComp.Z = 1;
		CurrTickClientMove.Force += FVector(0.0f, 0.0f, GameGravity);
	}
}

void ACombatVehicle::Decelerate(float DeltaTime)
{
	if (!bMoving)
	{
		FVector Acc = -GetVelocity() * DecelerateFactor;
		CurrTickClientMove.Force += FVector(Acc.X, Acc.Y, 0.0f);
	}

	if (!bTurning)
	{
		FVector Torque = -MeshComp->GetPhysicsAngularVelocityInDegrees() * DecelerateFactor;
		CurrTickClientMove.Torque += Torque;
	}
}

void ACombatVehicle::OnHealthUpdate()
{
	// Client-side Actions
	if (IsLocallyControlled())
	{
		FString HealthMessage = FString::Printf(TEXT("You now have %f health remaining."), CurrentHealth);
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, HealthMessage);

		if (CurrentHealth <= 0)
		{
			FString DeathMessage = FString::Printf(TEXT("You have been killed."));
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, DeathMessage);
		}
	}

	// Server-side Actions
	if (HasAuthority())
	{
		FString HealthMessage = FString::Printf(TEXT("%s now has %f health remaining."), *GetFName().ToString(), CurrentHealth);
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, HealthMessage);
	}
}

void ACombatVehicle::SetCurrentHealth(float HealthValue)
{
	if (GetLocalRole() == ROLE_Authority)
	{
		CurrentHealth = FMath::Clamp(HealthValue, 0.0f, MaxHealth);
		OnHealthUpdate();
	}
}

float ACombatVehicle::TakeDamage(float DamageTaken, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float DamageApplied = CurrentHealth - DamageTaken;
	SetCurrentHealth(DamageApplied);
	return DamageApplied;
}

void ACombatVehicle::StartShooting()
{
	if (!bIsShooting)
	{
		bIsShooting = true;
		UWorld* World = GetWorld();
		World->GetTimerManager().SetTimer(FiringTimer, this, &ACombatVehicle::StopShooting, FireRate, false);
		RPC_Server_HandleShooting();
	}
}

void ACombatVehicle::StopShooting()
{
	bIsShooting = false;
}

void ACombatVehicle::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACombatVehicle, CurrentHealth);
}

void ACombatVehicle::OnRep_CurrentHealth()
{
	OnHealthUpdate();
}

void ACombatVehicle::RPC_Server_HandleShooting_Implementation()
{
	FVector SpawnLocation = GetActorLocation() + (GetActorRotation().Vector() * 100.0f) + (GetActorUpVector() * 50.0f);
	FRotator SpawnRotation = GetActorRotation();

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Instigator = GetInstigator();
	SpawnParameters.Owner = this;

	AVehicleProjectile* SpawnedProjectile = GetWorld()->SpawnActor<AVehicleProjectile>(VehicleProjectile, SpawnLocation, SpawnRotation, SpawnParameters);
}

void ACombatVehicle::RPC_Server_UpdateMovement_Implementation(FNetClientPredStats NetClientPredStatsParam)
{
	// Extract First Move to be Processed
	FNetClientMove Move = NetClientPredStatsParam.ExtractMove();

	/*GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red,
		FString::Printf(TEXT("[SERVER] XYVel: %d"), (int)Move.bSetVelocity));*/

	// Apply the Move on the Remote Actor
	FVector Vel;
	Vel.X = (Move.SetVelocityComp.X > 0) ? Move.Velocity.X : MeshComp->GetPhysicsLinearVelocity().X;
	Vel.Y = (Move.SetVelocityComp.Y > 0) ? Move.Velocity.Y : MeshComp->GetPhysicsLinearVelocity().Y;
	Vel.Z = (Move.SetVelocityComp.Z > 0) ? Move.Velocity.Z : MeshComp->GetPhysicsLinearVelocity().Z;
	MeshComp->SetPhysicsLinearVelocity(Vel);
	MeshComp->AddForce(Move.Force, NAME_None, true);

	if (Move.bSetAngVelocity)
		MeshComp->SetPhysicsAngularVelocityInDegrees(Move.AngVelocity);
	MeshComp->AddTorqueInDegrees(Move.Torque, NAME_None, true);
}

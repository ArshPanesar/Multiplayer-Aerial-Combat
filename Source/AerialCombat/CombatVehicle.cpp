// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatVehicle.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/InputComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemComponent.h"

#include <Net/UnrealNetwork.h>
#include <Kismet/GameplayStatics.h>
#include "Components/DecalComponent.h"

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
	if (APlayerController* PlayerCont = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerCont->GetLocalPlayer()))
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

	// Setup Refs to Cameras
	VehicleCameraComp = Cast<UCameraComponent>(GetDefaultSubobjectByName("Camera"));
	TurretCameraComp = Cast<UCameraComponent>(GetDefaultSubobjectByName("TurretCamera"));
	check(TurretCameraComp != nullptr);
	check(VehicleCameraComp != nullptr);

	TurretMeshComp = Cast<UStaticMeshComponent>(GetDefaultSubobjectByName("Turret"));
	if (TurretMeshComp == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Turret not Found!"));
	}

	// Setup Jet Flame Mesh Refs (Hardcoded Names)
	JetFlameCenterMeshComp = Cast<UStaticMeshComponent>(GetDefaultSubobjectByName("JetFlameV2_Center"));
	JetFlameRightMeshComp = Cast<UStaticMeshComponent>(GetDefaultSubobjectByName("JetFlameV2_RimRight"));
	JetFlameLeftMeshComp = Cast<UStaticMeshComponent>(GetDefaultSubobjectByName("JetFlameV2_RimLeft"));

	check(JetFlameCenterMeshComp != nullptr);
	check(JetFlameRightMeshComp != nullptr);
	check(JetFlameLeftMeshComp != nullptr);

	JetFlameCenterScale = JetFlameCenterMeshComp->GetRelativeScale3D();
	JetFlameCenterPosition = JetFlameCenterMeshComp->GetRelativeLocation();

	JetFlameRightScale = JetFlameRightMeshComp->GetRelativeScale3D();
	JetFlameRightPosition = JetFlameRightMeshComp->GetRelativeLocation();

	JetFlameLeftScale = JetFlameLeftMeshComp->GetRelativeScale3D();
	JetFlameLeftPosition = JetFlameLeftMeshComp->GetRelativeLocation();

	// Setup Thrust Flame Particles
	ThrusterFlameCenterNS = Cast<UNiagaraComponent>(GetDefaultSubobjectByName("NS_ThrustFlame_Center"));
	ThrusterFlameRightNS = Cast<UNiagaraComponent>(GetDefaultSubobjectByName("NS_ThrustFlame_Right"));
	ThrusterFlameLeftNS = Cast<UNiagaraComponent>(GetDefaultSubobjectByName("NS_ThrustFlame_Left"));

	check(ThrusterFlameCenterNS != nullptr);
	check(ThrusterFlameRightNS != nullptr);
	check(ThrusterFlameLeftNS != nullptr);

	BrakeFlameCenterNS = Cast<UNiagaraComponent>(GetDefaultSubobjectByName("NS_BrakeFlame_Center"));
	BrakeFlameRightNS = Cast<UNiagaraComponent>(GetDefaultSubobjectByName("NS_BrakeFlame_Right"));
	BrakeFlameLeftNS = Cast<UNiagaraComponent>(GetDefaultSubobjectByName("NS_BrakeFlame_Left"));

	check(BrakeFlameCenterNS != nullptr);
	check(BrakeFlameRightNS != nullptr);
	check(BrakeFlameLeftNS != nullptr);

	// Set Turning Flame Particles
	TurnLeftNS = Cast<UNiagaraComponent>(GetDefaultSubobjectByName("NS_TurnLeft"));
	TurnRightNS = Cast<UNiagaraComponent>(GetDefaultSubobjectByName("NS_TurnRight"));

	check(TurnLeftNS != nullptr);
	check(TurnRightNS != nullptr);

	// Set Speed Trail Particles
	SpeedTrailLeftNS = Cast<UNiagaraComponent>(GetDefaultSubobjectByName("NS_LeftSpeedTrail"));
	SpeedTrailRightNS = Cast<UNiagaraComponent>(GetDefaultSubobjectByName("NS_RightSpeedTrail"));

	check(SpeedTrailLeftNS != nullptr);
	check(SpeedTrailRightNS != nullptr);

	// Hovering
	bShouldHover = false;

	HoverTime = 0.0f;

	// Override the Light Ridge Material Instance
	int32 LightRidgeMatIndex = 3; // HARDCODED MATERIAL INDEX
	LightRidgeMaterial = UMaterialInstanceDynamic::Create(MeshComp->GetMaterial(LightRidgeMatIndex), this);
	if (LightRidgeMaterial)
	{
		MeshComp->SetMaterial(LightRidgeMatIndex, LightRidgeMaterial);
		CurrLightRidgeColor = LightRidgeColorStart;
	}

	// Initialize Camera Post-Process Materials
	UMaterialInterface* SpeedLinesMaterialInterface = Cast<UMaterialInterface>(StaticLoadObject(UMaterial::StaticClass(), nullptr, TEXT("/Game/Models/PostProcess/M_PP_SpeedLines.M_PP_SpeedLines")));
	SpeedLinesMaterial = UMaterialInstanceDynamic::Create(SpeedLinesMaterialInterface, this);
	if (SpeedLinesMaterial)
	{
		SpeedLinesMaterial->SetScalarParameterValue("Alpha", 0.0f);

		// Add to Camera Post-Processing
		VehicleCameraComp->PostProcessSettings.AddBlendable(SpeedLinesMaterial, 1.0f);
	}
	UMaterialInterface* HitEffectMaterialInterface = Cast<UMaterialInterface>(StaticLoadObject(UMaterial::StaticClass(), nullptr, TEXT("/Game/Models/PostProcess/M_PP_HitEffect.M_PP_HitEffect")));
	HitEffectMaterial = UMaterialInstanceDynamic::Create(HitEffectMaterialInterface, this);
	if (HitEffectMaterial)
	{
		HitEffectMaterial->SetScalarParameterValue("Alpha", 0.0f);

		// Add to Camera Post-Processing
		VehicleCameraComp->PostProcessSettings.AddBlendable(HitEffectMaterial, 1.0f);
	}

	// Load Decal Material
	DecalMaterial = Cast<UMaterialInterface>(StaticLoadObject(UMaterial::StaticClass(), nullptr, TEXT("/Game/Models/Decals/M_ProjectileDecal.M_ProjectileDecal")));

	// Initialize Health for UI
	UI_OnHealthUpdate(CurrentHealth);
	bDeathQueued = false;

	// Network Check
	bReplicates = true;
	bIsClient = (GetNetMode() == ENetMode::NM_Client);
	
	// Disable Gravity for Simulated Proxies
	if (GetLocalRole() == ROLE_SimulatedProxy)
	{
		MeshComp->SetEnableGravity(false);
	}
}

// Called every frame
void ACombatVehicle::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Reconcile for Autonomous and Simulated Proxies
	if (!HasAuthority())
	{
		FVector ReconciledLocation = FMath::VInterpTo(GetActorLocation(), ServerStats.Location, DeltaTime, 4.0f);
		FRotator ReconciledRotation = FMath::RInterpTo(GetActorRotation(), ServerStats.Rotation, DeltaTime, 4.0f);

		// Simulated Proxies will Always Reconcile
		if (GetLocalRole() == ROLE_SimulatedProxy)
		{
			SetActorLocationAndRotation(ReconciledLocation, ReconciledRotation);
		}

		// Autonomous Proxy will Reconcile with Server if needed
		if (bShouldReconcileMovement)
		{
			FVector ReconciledVelocity = FMath::VInterpTo(GetVelocity(), ServerStats.Velocity, DeltaTime, 4.0f);

			SetActorLocationAndRotation(ReconciledLocation, ReconciledRotation);
			if (IsLocallyControlled()) // Simulated Proxies don't need to worry about Velocity
			{
				MeshComp->SetPhysicsLinearVelocity(ReconciledVelocity);
			}

			// Check if Reconciled (Using Location only)
			FVector LocError = ReconciledLocation - ServerStats.Location;
			bShouldReconcileMovement = LocError.SizeSquared() > MaxNetPredictionError * MaxNetPredictionError;
		}
	}

	// Perform Physics Locally
	if (IsLocallyControlled())
	{
		UpdateBoostMode(DeltaTime);
		UpdateLightRidgeColor(DeltaTime);
		UpdateJetFlameVisuals(DeltaTime);
		SetThrustFlameVisuals();
		SetTurningFlameVisuals();
		SetSpeedTrailVisuals();
		UpdateHitEffect(DeltaTime);

		UpdateTurretOrientation();

		// Apply Move
		CurrTickClientMove.Timestamp = GetWorld()->GetTimeSeconds();
		if (!HasAuthority())
		{
			UpdateMovement(CurrTickClientMove);
		}
		
		// Enqueue the Resultant Move of this Tick
		NetClientPredStats.AddMove(CurrTickClientMove);
		
		// Ask Server to Authorize Move
		RPC_Server_UpdateMovement(CurrTickClientMove);

		// Prepare for the Next Tick
		CurrTickClientMove = FNetClientMove();

		// Update Visuals on Remote Clients
		RPC_Server_UpdateVisuals(CurrTickClientVisuals);
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

		// Locking In
		EnhancedInputComponent->BindAction(LockInInputAction, ETriggerEvent::Started, this, &ACombatVehicle::ToggleLockIn);
		
		// Shooting
		EnhancedInputComponent->BindAction(FireInputAction, ETriggerEvent::Triggered, this, &ACombatVehicle::StartShooting);

		// Boost Mode
		EnhancedInputComponent->BindAction(BoostInputAction, ETriggerEvent::Triggered, this, &ACombatVehicle::ActivateBoost);
	}
}

void ACombatVehicle::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	SetOwner(NewController);

	AACPlayerState* PS = GetPlayerState<AACPlayerState>();
	if (PS)
	{
		AbilitySystemComp = Cast<UCVAbilitySystemComponent>(PS->AbilitySystemComponent);
		AbilitySystemComp->InitAbilityActorInfo(PS, this);
	}
}

void ACombatVehicle::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	AACPlayerState* PS = GetPlayerState<AACPlayerState>();
	if (PS)
	{
		// Set the ASC for clients. Server does this in PossessedBy.
		AbilitySystemComp = Cast<UCVAbilitySystemComponent>(PS->AbilitySystemComponent);

		// Init ASC Actor Info for clients. Server will init its ASC when it possesses a new Actor.
		AbilitySystemComp->InitAbilityActorInfo(PS, this);
	}
}

UCVAbilitySystemComponent* ACombatVehicle::GetAbilitySystemComponent() const
{
	return AbilitySystemComp;
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
	CurrTickClientMove.InputVertical = 1.0f;

	bAscending = true;
	bShouldHover = false;
}

void ACombatVehicle::StopAscending(const FInputActionValue& Value)
{
	bShouldHover = true;
	bAscending = false;
	MaxVelAchieved = GetVelocity().Z;
	HoverTime = 0.0f;
}

void ACombatVehicle::Descend(const FInputActionValue& Value)
{
	CurrTickClientMove.InputVertical = -1.0f;
	
	bShouldHover = false;
	bDescending = true;
}

void ACombatVehicle::StopDescending(const FInputActionValue& Value)
{
	bShouldHover = true;
	bDescending = false;
	MaxVelAchieved = GetVelocity().Z;
	HoverTime = 0.0f;
}

void ACombatVehicle::MoveForward(const FInputActionValue& Value)
{
	CurrTickClientMove.InputForward = 1.0f;

	if (bBoostActive)
	{
		CurrTickClientMove.InputBoost = 1.0f;
	}

	bMoving = true;
	bMoveDirectionForward = true;
}

void ACombatVehicle::MoveBackward(const FInputActionValue& Value)
{
	CurrTickClientMove.InputForward = -1.0f;

	bMoving = true;
	bMoveDirectionForward = false;
}

void ACombatVehicle::StopMoving(const FInputActionValue& Value)
{
	bMoving = false;
	bBoostActive = false;

	CurrTickClientMove.InputForward = 0.0f;
	CurrTickClientMove.InputBoost = 0.0f;
}

void ACombatVehicle::TurnLeft(const FInputActionValue& Value)
{
	CurrTickClientMove.InputSteering = -1.0f;

	bTurning = true;
	bTurnDirectionRight = false;
}

void ACombatVehicle::TurnRight(const FInputActionValue& Value)
{
	CurrTickClientMove.InputSteering = 1.0f;

	bTurning = true;
	bTurnDirectionRight = true;
}

void ACombatVehicle::StopTurning(const FInputActionValue& Value)
{
	CurrTickClientMove.InputSteering = 0.0f;

	bTurning = false;
}

void ACombatVehicle::ActivateBoost(const FInputActionValue& Value)
{
	if (bMoving && bMoveDirectionForward)
	{
		CurrTickClientMove.InputBoost = 1.0f;
		bBoostActive = true;
	}
}

void ACombatVehicle::UpdateMovement(FNetClientMove& Move)
{
	//// Global Control Parameters for Movement
	//static float MaxVerticalVelocityAchieved = 0.0f;

	// Read the Inputs and generate the required Forces/Velocities
	FVector ApplyVelocity = FVector();
	FVector ApplyForce = FVector();
	FVector3d SetVelocityComp = FVector3d();

	// Ascending/Descending
	bShouldHover = true;
	if (FMath::Abs(Move.InputVertical) > 0.0f)
	{
		float MaxVerticalVelocity = Move.InputVertical > 0.0f ? MaxAscentVelocity : MaxDescentVelocity;
		float VerticalAcc = Move.InputVertical > 0.0f ? AscentAcceleration : DescentAcceleration;

		if (GetVelocity().Z > MaxVerticalVelocity)
		{
			ApplyVelocity.Z = MaxVerticalVelocity;
			SetVelocityComp.Z = 1;
		}
		else
		{
			ApplyForce += FVector(0.0f, 0.0f, GameGravity + VerticalAcc);
		}

		MaxVelAchieved = GetVelocity().Z;
		HoverTime = 0.0f;
		bShouldHover = false;
	}
	
	// Forward Movement (with Boost)
	bBoostActive = Move.InputBoost > 0.0f;
	bMoving = false;

	if (Move.InputForward > 0.0f)
	{
		float UseMaxVel = bBoostActive ? BoostModeMaxVelocity : MaxMovementVelocity;
		float UseAcc = bBoostActive ? BoostModeAcceleration : MovementAcceleration;
		if (GetVelocity().SquaredLength() > UseMaxVel * UseMaxVel && GetVelocity().Dot(GetActorForwardVector()) > 0.0f)
		{
			FVector Vel = UseMaxVel * GetActorForwardVector();
			ApplyVelocity.X = Vel.X;
			ApplyVelocity.Y = Vel.Y;
			SetVelocityComp.X = 1;
			SetVelocityComp.Y = 1;
		}
		else
		{
			// Apply Acceleration
			FVector Acc = UseAcc * GetActorForwardVector();
			ApplyForce += Acc;
		}

		bMoving = true;
	}
	else if (Move.InputForward < 0.0f) // Backward Movement
	{
		FVector Vel(GetVelocity());
		Vel.Z = 0.0f;

		if (Vel.SquaredLength() > MaxMovementVelocity * MaxMovementVelocity && Vel.Dot(-GetActorForwardVector()) > 0.0f)
		{
			Vel = -MaxMovementVelocity * GetActorForwardVector();
			ApplyVelocity.X = Vel.X;
			ApplyVelocity.Y = Vel.Y;
			SetVelocityComp.X = 1;
			SetVelocityComp.Y = 1;
		}
		else
		{
			// Apply Acceleration
			FVector Acc = -MovementAcceleration * GetActorForwardVector();
			ApplyForce += Acc;
		}

		bMoving = true;
	}
	
	// Steering
	FVector Torque = FVector();
	FVector AngVelocity = FVector();
	bool bSetAngVelocity = false;

	bTurning = false;

	if (FMath::Abs(Move.InputSteering) > 0.0f)
	{
		float UseMaxTurningSpeed = Move.InputSteering > 0.0f ? MaxTurningSpeed : -MaxTurningSpeed;
		float UseTurningTorque = Move.InputSteering > 0.0f ? TurningTorque : -TurningTorque;

		if (MeshComp->GetPhysicsAngularVelocityInDegrees().SquaredLength() > MaxTurningSpeed * MaxTurningSpeed)
		{
			AngVelocity = FVector(0.0f, 0.0f, UseMaxTurningSpeed);
			bSetAngVelocity = true;
		}
		else
		{
			Torque += FVector(0.0f, 0.0f, UseTurningTorque);
		}

		bTurning = true;
	}
	
	// Update Vehicle Rotation for Boost Mode
	FRotator Rotation = Move.BoostRotation;
	
	// Update Hovering Physics
	float VelZ = ApplyVelocity.Z;
	ComputeHoverPhysics(GetWorld()->GetDeltaSeconds(), VelZ, ApplyForce);
	if (bShouldHover)
	{
		ApplyVelocity.Z = VelZ;
		SetVelocityComp.Z = 1;
	}
	
	// Update Deceleration when not moving
	ComputeDeceleration(GetWorld()->GetDeltaSeconds(), ApplyForce, Torque);


	// Apply the Move on the Actor
	FVector Vel;
	Vel.X = (SetVelocityComp.X > 0) ? ApplyVelocity.X : MeshComp->GetPhysicsLinearVelocity().X;
	Vel.Y = (SetVelocityComp.Y > 0) ? ApplyVelocity.Y : MeshComp->GetPhysicsLinearVelocity().Y;
	Vel.Z = (SetVelocityComp.Z > 0) ? ApplyVelocity.Z : MeshComp->GetPhysicsLinearVelocity().Z;
	MeshComp->SetPhysicsLinearVelocity(Vel);
	MeshComp->AddForce(ApplyForce, NAME_None, true);

	SetActorRotation(Rotation);

	if (bSetAngVelocity)
		MeshComp->SetPhysicsAngularVelocityInDegrees(AngVelocity);
	MeshComp->AddTorqueInDegrees(Torque, NAME_None, true);
}

void ACombatVehicle::ComputeHoverPhysics(float DeltaTime, float& VelZ, FVector& Force)
{
	if (bShouldHover)
	{
		HoverTime += DeltaTime;

		// Update Output Parameters
		//
		// Damped Cosine Wave
		VelZ = WobbleAmplitude * FMath::Exp(-WobbleDecayConst * HoverTime) * FMath::Cos(WobbleFrequency * HoverTime) * MaxVelAchieved;
		// Pushback against Gravity
		Force += FVector(0.0f, 0.0f, GameGravity);
	}
}

void ACombatVehicle::ComputeDeceleration(float DeltaTime, FVector& Force, FVector& Torque)
{
	if (!bMoving)
	{
		FVector Acc = -GetVelocity() * DecelerateFactor;
		Force += FVector(Acc.X, Acc.Y, 0.0f);
	}

	if (!bTurning)
	{
		FVector AngAcc = -MeshComp->GetPhysicsAngularVelocityInDegrees() * DecelerateFactor;
		Torque += AngAcc;
	}
}

void ACombatVehicle::UpdateBoostMode(float DeltaTime)
{
	bool bReturnZeroPitch = true;
	bool bReturnZeroRoll = true;
	if (bBoostActive)
	{
		// Change Pitch
		bReturnZeroPitch = !(bAscending || bDescending);
		BoostModeZeroPitchTimer = (bReturnZeroPitch) ? BoostModeZeroPitchTimer : 0.0f;
		
		FRotator Rotation = GetActorRotation();
		if (bAscending)
		{
			// Pitch Up
			Rotation.Pitch = (Rotation.Pitch < BoostPitchAngleDegrees) ? Rotation.Pitch + BoostRotationSpeed * DeltaTime : BoostPitchAngleDegrees;
		}
		else if (bDescending)
		{
			// Pitch Down
			Rotation.Pitch = (Rotation.Pitch > -BoostPitchAngleDegrees) ? Rotation.Pitch - BoostRotationSpeed * DeltaTime : -BoostPitchAngleDegrees;
		}
		BoostModeLastPitchAchieved = Rotation.Pitch;

		// Change Roll
		bReturnZeroRoll = !bTurning;
		BoostModeZeroRollTimer = (bReturnZeroRoll) ? BoostModeZeroRollTimer : 0.0f;
		if (bTurning)
		{
			if (bTurnDirectionRight)
			{
				Rotation.Roll = (Rotation.Roll < BoostRollAngleDegrees) ? Rotation.Roll + BoostRotationSpeed * DeltaTime : BoostRollAngleDegrees;
			}
			else
			{
				Rotation.Roll = (Rotation.Roll > -BoostRollAngleDegrees) ? Rotation.Roll - BoostRotationSpeed * DeltaTime : -BoostRollAngleDegrees;
			}
			BoostModeLastRollAchieved = Rotation.Roll;
		}
		SetActorRotation(Rotation);

		// Apply Post Process Material
		SpeedLinesFadeTimer += DeltaTime * SpeedLinesFadeFactor;
		SpeedLinesFadeTimer = (SpeedLinesFadeTimer > 1.0f) ? 1.0f : SpeedLinesFadeTimer;
		SpeedLinesMaterial->SetScalarParameterValue("Alpha", SpeedLinesFadeTimer);
	}
	else
	{
		// Stop Post Process Material
		SpeedLinesFadeTimer -= DeltaTime * SpeedLinesFadeFactor;
		SpeedLinesFadeTimer = (SpeedLinesFadeTimer < 0.0f) ? 0.0f : SpeedLinesFadeTimer;
		SpeedLinesMaterial->SetScalarParameterValue("Alpha", SpeedLinesFadeTimer);
	}

	FRotator Rotation = GetActorRotation();
	if (bReturnZeroPitch)
	{
		// Return to Zero Pitch
		BoostModeZeroPitchTimer += DeltaTime * BoostReturnZeroPitchSpeed;
		BoostModeZeroPitchTimer = (BoostModeZeroPitchTimer > 1.0f) ? 1.0f : BoostModeZeroPitchTimer;

		Rotation.Pitch = FMath::Lerp(BoostModeLastPitchAchieved, 0.0f, BoostModeZeroPitchTimer);
	}
	if (bReturnZeroRoll)
	{
		// Return to Zero Roll
		BoostModeZeroRollTimer += DeltaTime * BoostReturnZeroPitchSpeed;
		BoostModeZeroRollTimer = (BoostModeZeroRollTimer > 1.0f) ? 1.0f : BoostModeZeroRollTimer;

		Rotation.Roll = FMath::Lerp(BoostModeLastRollAchieved, 0.0f, BoostModeZeroRollTimer);
	}
	SetActorRotation(Rotation);

	// Replicate Boost Mode Movement and Visuals
	CurrTickClientMove.BoostRotation = Rotation;
	CurrTickClientVisuals.bBoostModeActive = bBoostActive;
}

void ACombatVehicle::UpdateLightRidgeColor(float DeltaTime)
{
	// Lerp between Light Ridge Colors
	LightRidgeLerpTimer += DeltaTime;

	float LerpFactor = FMath::Abs(FMath::Sin(LightRidgeLerpTimer));

	if (!bIsLockedIn)
	{
		// Fade back to Starting Color if recently Locked In
		if (LightRidgeLockInTimer > 0.0f)
		{
			CurrLightRidgeColor = FMath::Lerp(LightRidgeColorStart, LightRidgeLockInColor, LightRidgeLockInTimer); // Lerp Backwards
			LightRidgeLerpTimer = 0.0f;
			LightRidgeLockInTimer -= DeltaTime * LightRidgeLockInFadeFactor;
		}
		else
		{
			// Proceed with Normal Color Cycle
			CurrLightRidgeColor = FMath::Lerp(LightRidgeColorStart, LightRidgeColorEnd, LerpFactor);
			ActiveLightRidgeColor = CurrLightRidgeColor;
			LightRidgeLockInTimer = 0.0f;
		}
	}
	else
	{
		// Fade to the Specified Lock In Color
		LightRidgeLockInTimer = (LightRidgeLockInTimer < 1.0f) ? LightRidgeLockInTimer + DeltaTime * LightRidgeLockInFadeFactor : 1.0f;
		CurrLightRidgeColor = FMath::Lerp(ActiveLightRidgeColor, LightRidgeLockInColor, LightRidgeLockInTimer);
	}
	
	LightRidgeMaterial->SetVectorParameterValue("LightColor", CurrLightRidgeColor);

	// Sync on all Instances
	CurrTickClientVisuals.CurrLightRidgeColor = CurrLightRidgeColor;
}

void ACombatVehicle::UpdateJetFlameVisuals(float DeltaTime)
{
	float VelZ = GetVelocity().Z;

	float ScaleMultiplier = 1.0f;
	if (VelZ > 0.0f)
	{
		ScaleMultiplier += (VelZ / MaxAscentVelocity);
	}
	else
	{
		ScaleMultiplier -= (VelZ / MaxDescentVelocity);
	}

	FVector NewScale = JetFlameCenterScale;
	NewScale.Z *= ScaleMultiplier;
	JetFlameCenterMeshComp->SetRelativeScale3D(NewScale);
	JetFlameCenterMeshComp->SetRelativeLocation(JetFlameCenterPosition);

	CurrTickClientVisuals.JetFlameCenterScale = NewScale;

	NewScale = JetFlameRightScale;
	NewScale.Z *= ScaleMultiplier;
	JetFlameRightMeshComp->SetRelativeScale3D(NewScale);
	JetFlameRightMeshComp->SetRelativeLocation(JetFlameRightPosition);

	CurrTickClientVisuals.JetFlameRightScale = NewScale;

	NewScale = JetFlameLeftScale;
	NewScale.Z *= ScaleMultiplier;
	JetFlameLeftMeshComp->SetRelativeScale3D(NewScale);
	JetFlameLeftMeshComp->SetRelativeLocation(JetFlameLeftPosition);

	CurrTickClientVisuals.JetFlameLeftScale = NewScale;
}

void ACombatVehicle::SetThrustFlameVisuals()
{
	if (bMoving)
	{
		if (bMoveDirectionForward)
		{
			if (!ThrusterFlameCenterNS->IsActive()) // Just need to check one NS
			{
				ThrusterFlameCenterNS->Activate(true);
				ThrusterFlameRightNS->Activate(true);
				ThrusterFlameLeftNS->Activate(true);
			}
		}
		else
		{
			if (!BrakeFlameCenterNS->IsActive())
			{
				BrakeFlameCenterNS->Activate(true);
				BrakeFlameRightNS->Activate(true);
				BrakeFlameLeftNS->Activate(true);
			}
		}

		if (bBoostActive)
		{
			if (!bSetThrustToBoostMode)
			{
				ThrusterFlameCenterNS->SetNiagaraVariableFloat(TEXT("User.SpawnRate"), NSThrustBoostSpawnRate);
				ThrusterFlameRightNS->SetNiagaraVariableFloat(TEXT("User.SpawnRate"), NSThrustBoostSpawnRate);
				ThrusterFlameLeftNS->SetNiagaraVariableFloat(TEXT("User.SpawnRate"), NSThrustBoostSpawnRate);

				ThrusterFlameCenterNS->SetNiagaraVariableLinearColor(TEXT("User.ThrustColor"), NSThrustBoostColor);
				ThrusterFlameRightNS->SetNiagaraVariableLinearColor(TEXT("User.ThrustColor"), NSThrustBoostColor);
				ThrusterFlameLeftNS->SetNiagaraVariableLinearColor(TEXT("User.ThrustColor"), NSThrustBoostColor);
				
				ThrusterFlameCenterNS->SetNiagaraVariableFloat(TEXT("User.MaxLifeTime"), NSThrustBoostLifeTime);
				ThrusterFlameRightNS->SetNiagaraVariableFloat(TEXT("User.MaxLifeTime"), NSThrustBoostLifeTime);
				ThrusterFlameLeftNS->SetNiagaraVariableFloat(TEXT("User.MaxLifeTime"), NSThrustBoostLifeTime);


				bSetThrustToBoostMode = true;
			}
		}
		else if (bSetThrustToBoostMode)
		{
			ThrusterFlameCenterNS->SetNiagaraVariableFloat(TEXT("User.SpawnRate"), NSThrustNormalSpawnRate);
			ThrusterFlameRightNS->SetNiagaraVariableFloat(TEXT("User.SpawnRate"), NSThrustNormalSpawnRate);
			ThrusterFlameLeftNS->SetNiagaraVariableFloat(TEXT("User.SpawnRate"), NSThrustNormalSpawnRate);

			ThrusterFlameCenterNS->SetNiagaraVariableLinearColor(TEXT("User.ThrustColor"), NSThrustNormalColor);
			ThrusterFlameRightNS->SetNiagaraVariableLinearColor(TEXT("User.ThrustColor"), NSThrustNormalColor);
			ThrusterFlameLeftNS->SetNiagaraVariableLinearColor(TEXT("User.ThrustColor"), NSThrustNormalColor);

			ThrusterFlameCenterNS->SetNiagaraVariableFloat(TEXT("User.MaxLifeTime"), NSThrustNormalLifeTime);
			ThrusterFlameRightNS->SetNiagaraVariableFloat(TEXT("User.MaxLifeTime"), NSThrustNormalLifeTime);
			ThrusterFlameLeftNS->SetNiagaraVariableFloat(TEXT("User.MaxLifeTime"), NSThrustNormalLifeTime);

			bSetThrustToBoostMode = false;
		}
	}
	else
	{
		if (ThrusterFlameCenterNS->IsActive())
		{
			ThrusterFlameCenterNS->Deactivate();
			ThrusterFlameRightNS->Deactivate();
			ThrusterFlameLeftNS->Deactivate();
		}
		if (BrakeFlameCenterNS->IsActive())
		{
			BrakeFlameCenterNS->Deactivate();
			BrakeFlameRightNS->Deactivate();
			BrakeFlameLeftNS->Deactivate();
		}
	}

	// Send to Network
	CurrTickClientVisuals.bActivateThrustOrBrakes = bMoving;
	CurrTickClientVisuals.bMoveDirectionForward = bMoveDirectionForward;
}

void ACombatVehicle::SetTurningFlameVisuals()
{
	if (bTurning)
	{
		if (bTurnDirectionRight)
		{
			if (!TurnRightNS->IsActive())
				TurnRightNS->Activate(true);
		}
		else
		{
			if (!TurnLeftNS->IsActive())
				TurnLeftNS->Activate(true);
		}
	}
	else
	{
		if (TurnLeftNS->IsActive())
			TurnLeftNS->Deactivate();
		if (TurnRightNS->IsActive())
			TurnRightNS->Deactivate();
	}

	CurrTickClientVisuals.bActivateTurningFlames = bTurning;
	CurrTickClientVisuals.bTurnDirectionRight = bTurnDirectionRight;
}

void ACombatVehicle::SetSpeedTrailVisuals()
{
	if (bBoostActive)
	{
		if (!SpeedTrailLeftNS->IsActive())
		{
			SpeedTrailLeftNS->Activate();
		}
		if (!SpeedTrailRightNS->IsActive())
		{
			SpeedTrailRightNS->Activate();
		}
		bSpeedTrailTimerSet = false;
	}
	else
	{
		if (!bSpeedTrailTimerSet)
		{
			GetWorldTimerManager().SetTimer(SpeedTrailTimer, this, &ACombatVehicle::StopSpeedTrailVisuals, SpeedTrailStopTime);
			bSpeedTrailTimerSet = true;
		}
	}
}

void ACombatVehicle::StopSpeedTrailVisuals()
{
	if (SpeedTrailLeftNS->IsActive())
	{
		SpeedTrailLeftNS->Deactivate();
	}
	if (SpeedTrailRightNS->IsActive())
	{
		SpeedTrailRightNS->Deactivate();
	}
}

void ACombatVehicle::UpdateHitEffect(float DeltaTime)
{
	if (HitEffectFadeTimer > 0.0f)
	{
		HitEffectFadeTimer -= DeltaTime * HitEffectFadeFactor;
		HitEffectFadeTimer = (SpeedLinesFadeTimer < 0.0f) ? 0.0f : HitEffectFadeTimer;
		HitEffectMaterial->SetScalarParameterValue("Alpha", HitEffectFadeTimer);
	}
}

void ACombatVehicle::UpdateTurretOrientation()
{
	if (bIsLockedIn)
	{
		// Make Turret Follow LockedIn Camera Rotation
		FRotator CorrectedTurretRotation = TurretCameraComp->GetComponentRotation();

		CorrectedTurretRotation.Add(0.0f, -90.0f, 0.0f); // Correction for Turret Mesh (Offset by 90 degrees)
		CorrectedTurretRotation.Roll = -CorrectedTurretRotation.Pitch;
		CorrectedTurretRotation.Pitch = 0.0f;
		TurretMeshComp->SetWorldRotation(CorrectedTurretRotation);

		CurrTickClientVisuals.TurretRotation = CorrectedTurretRotation;
	}
	CurrTickClientVisuals.bIsLockedIn = bIsLockedIn;
}

void ACombatVehicle::OnHealthUpdate()
{
	// Don't Accept Any Updates after Death is Queued
	if (bDeathQueued)
		return;

	// Client-side Actions
	if (IsLocallyControlled())
	{
		UI_OnHealthUpdate(CurrentHealth);
	}

	// Server-side Actions
	if (HasAuthority())
	{
		if (CurrentHealth <= 0)
		{
			// Let the Blueprint Handle Death Scenario
			BP_PlayerDeath();
			bDeathQueued = true;

			// Update Leaderboard Stats
			if (AACPlayerState* OwnPlayerState = Cast<AACPlayerState>(GetPlayerState()))
			{
				OwnPlayerState->NumDeaths += 1;
			}
			if (LastShotBy)
			{
				if (AACPlayerState* OtherPlayerState = Cast<AACPlayerState>(LastShotBy->GetPlayerState()))
				{
					OtherPlayerState->NumKills += 1;
				}
			}
		}
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
	LastShotBy = Cast<ACombatVehicle>(DamageCauser);
	
	SetCurrentHealth(DamageApplied);

	return DamageApplied;
}

void ACombatVehicle::ToggleLockIn()
{
	if (!bIsLockedIn)
	{
		TurretCameraComp->SetActive(true);
		VehicleCameraComp->SetActive(false);
		TurretMeshComp->SetVisibility(false);

		// Limit Turret Camera Rotation
		APlayerController* PlayerController = Cast<APlayerController>(GetController());
		PlayerController->PlayerCameraManager->ViewPitchMin = TurretCameraPitchLimits.X;
		PlayerController->PlayerCameraManager->ViewPitchMax = TurretCameraPitchLimits.Y;

		// Blueprint Implemented
		UI_SetLockedIn(true);

		bIsLockedIn = true;
	}
	else
	{
		TurretCameraComp->SetActive(false);
		VehicleCameraComp->SetActive(true);
		TurretMeshComp->SetVisibility(true);

		// Free Normal Camera Rotation
		APlayerController* PlayerController = Cast<APlayerController>(GetController());
		PlayerController->PlayerCameraManager->ViewPitchMin = NormalCameraPitchLimits.X;
		PlayerController->PlayerCameraManager->ViewPitchMax = NormalCameraPitchLimits.Y;

		// Blueprint Implemented
		UI_SetLockedIn(false);

		bIsLockedIn = false;
	}
}

void ACombatVehicle::StartShooting()
{
	if (!bIsShooting && bIsLockedIn)
	{
		bIsShooting = true;

		GetWorld()->GetTimerManager().SetTimer(FiringTimer, this, &ACombatVehicle::StopShooting, FireRate, false);
		
		// Ask Server to Spawn the Projectile
		if (AbilitySystemComp)
		{
			bool bResult = AbilitySystemComp->TryActivateAbilityByClass(UShootingGameplayAbility::StaticClass());
		}
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
	DOREPLIFETIME(ACombatVehicle, ServerStats);
}

void ACombatVehicle::OnRep_CurrentHealth()
{
	OnHealthUpdate();
}

void ACombatVehicle::OnRep_ServerStats()
{
	// Handles Autonomous Proxy (Owning Client)
	//

	if (IsLocallyControlled() && GetLocalRole() == ROLE_AutonomousProxy)
	{
		// Client needs to Reconcile its Movement with the Server's
		//

		// Check if Location Error is Large Enough
		FVector LocError = GetActorLocation() - ServerStats.Location;

		// Reset Movement according to Server Authorized Parameters if Error exceeds Threshold
		bShouldReconcileMovement = LocError.SizeSquared() > MaxNetPredictionError * MaxNetPredictionError;
		
		// Remove Acknowledged Moves
		NetClientPredStats.RemoveAcknowledgedMoves(ServerStats.Timestamp);
	}
}

void ACombatVehicle::RPC_Multicast_UpdateVisuals_Implementation(FNetClientVisuals NewVisuals)
{
	// Don't Run on Local Client
	if (IsLocallyControlled())
		return;

	// Update Light Ridge
	LightRidgeMaterial->SetVectorParameterValue("LightColor", NewVisuals.CurrLightRidgeColor);
	
	// Update Jet Flame
	JetFlameCenterMeshComp->SetRelativeScale3D(NewVisuals.JetFlameCenterScale);
	JetFlameCenterMeshComp->SetRelativeLocation(JetFlameCenterPosition);

	JetFlameRightMeshComp->SetRelativeScale3D(NewVisuals.JetFlameRightScale);
	JetFlameRightMeshComp->SetRelativeLocation(JetFlameRightPosition);

	JetFlameLeftMeshComp->SetRelativeScale3D(NewVisuals.JetFlameLeftScale);
	JetFlameLeftMeshComp->SetRelativeLocation(JetFlameLeftPosition);

	// Update Thrust/Brake Flames
	bMoving = NewVisuals.bActivateThrustOrBrakes;
	bMoveDirectionForward = NewVisuals.bMoveDirectionForward;
	SetThrustFlameVisuals();

	// Update Turning Flames
	bTurning = NewVisuals.bActivateTurningFlames;
	bTurnDirectionRight = NewVisuals.bTurnDirectionRight;
	SetTurningFlameVisuals();

	// Update Turret Rotation
	if (NewVisuals.bIsLockedIn)
	{
		TurretMeshComp->SetWorldRotation(NewVisuals.TurretRotation);
	}

	// Update Speed Trails
	bBoostActive = NewVisuals.bBoostModeActive;
	SetSpeedTrailVisuals();
}

void ACombatVehicle::RPC_Server_UpdateVisuals_Implementation(FNetClientVisuals NewVisuals)
{
	RPC_Multicast_UpdateVisuals(NewVisuals);
}

void ACombatVehicle::RPC_Server_UpdateMovement_Implementation(FNetClientMove ClientMove)
{
	// Apply the Move on the Remote Actor
	UpdateMovement(ClientMove);

	// Update Server Stats (Replicated Property)
	ServerStats.Location = GetActorLocation();
	ServerStats.Rotation = GetActorRotation();
	ServerStats.Velocity = GetVelocity();
	ServerStats.Timestamp = ClientMove.Timestamp;
}

void ACombatVehicle::RPC_Multicast_SpawnDecal_Implementation(FVector Location, FRotator Rotation, FVector DecalTexSize)
{
	UDecalComponent* DecalComp = UGameplayStatics::SpawnDecalAttached(DecalMaterial, DecalTexSize, MeshComp, NAME_None, Location,
		Rotation, EAttachLocation::KeepWorldPosition, 5.0f);
	DecalComp->SetFadeScreenSize(0.0f);

	if (IsLocallyControlled())
	{
		// Display Hit Effect
		HitEffectFadeTimer = 1.0f;
		HitEffectMaterial->SetScalarParameterValue("Alpha", HitEffectFadeTimer);
	}
}

void ACombatVehicle::RPC_Server_SpawnDecal_Implementation(FVector Location, FRotator Rotation, FVector DecalTexSize)
{
	RPC_Multicast_SpawnDecal(Location, Rotation, DecalTexSize);
}
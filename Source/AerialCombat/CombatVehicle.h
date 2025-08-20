// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"

// Projectile
#include "VehicleProjectile.h"

// Niagara System
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

#include "CombatVehicle.generated.h"

// Network
USTRUCT()
struct FNetClientMove // Per Tick
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Velocity = FVector();

	UPROPERTY()
	FVector Force = FVector();

	UPROPERTY()
	FVector Torque = FVector();

	UPROPERTY()
	FVector AngVelocity = FVector();

	UPROPERTY()
	FVector3d SetVelocityComp = FVector3d();

	UPROPERTY()
	bool bSetAngVelocity = false;
};

USTRUCT()
struct FNetClientPredStats
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FNetClientMove> MoveQueue;

	UPROPERTY()
	int MaxMovesInQueue = 16;

public:
	FNetClientPredStats() : MoveQueue()
	{
		MoveQueue.Reserve(MaxMovesInQueue);
	}

	bool IsMoveQueueEmpty()
	{
		return (MoveQueue.Num() == 0);
	}

	int GetNumOfMoves()
	{
		return MoveQueue.Num();
	}

	// Enqueue a Move
	void AddMove(FNetClientMove& Move)
	{
		if (MoveQueue.Num() < MaxMovesInQueue)
		{
			MoveQueue.Add(Move);
		}
	}

	// Dequeue Moves
	FNetClientMove ExtractMove()
	{
		// Avoid Out-of-Bounds Error
		if (IsMoveQueueEmpty())
			return FNetClientMove();

		FNetClientMove Move = MoveQueue[0];
		MoveQueue.RemoveAt(0);
		return Move;
	}

	// Update on Client-Side for Next Tick
	// Must be called after Server RPC
	void UpdateClient()
	{
		// Assume First Move Processed by Server
		ExtractMove();
	}
};


USTRUCT()
struct FNetClientVisuals
{
	GENERATED_BODY()

	UPROPERTY()
	FLinearColor CurrLightRidgeColor;

	UPROPERTY()
	FVector JetFlameCenterScale;
	
	UPROPERTY()
	FVector JetFlameRightScale;
	
	UPROPERTY()
	FVector JetFlameLeftScale;

	UPROPERTY()
	bool bActivateThrustOrBrakes = false;

	UPROPERTY()
	bool bMoveDirectionForward = false; // To be used with `bActivateThrustOrBrakes`

	UPROPERTY()
	bool bActivateTurningFlames = false;

	UPROPERTY()
	bool bTurnDirectionRight = false; // To be used with `bActivateTurningFlames`
};


UCLASS()
class AERIALCOMBAT_API ACombatVehicle : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ACombatVehicle();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle")
	float AscentAcceleration = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle")
	float MaxAscentVelocity = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle")
	float DescentAcceleration = -100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle")
	float MaxDescentVelocity = -150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle")
	float MovementAcceleration = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle")
	float MaxMovementVelocity = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle")
	float DecelerateFactor = 2.5f;

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

	// Health
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle | Health")
	float MaxHealth = 100.0f;



	// Light Ridge Colors
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle | Visuals")
	FLinearColor LightRidgeColorStart = FColor::Blue;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle | Visuals")
	FLinearColor LightRidgeColorEnd = FColor::Green;

	FLinearColor CurrLightRidgeColor;


	UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth)
	float CurrentHealth = 100.0f;

	// Shooting Projectiles
	UPROPERTY(EditDefaultsOnly, Category = "Vehicle | Shooting")
	TSubclassOf<AVehicleProjectile> VehicleProjectile;

	// Delay between shots in seconds. Used to control fire rate for your test projectile, 
	// but also to prevent an overflow of server functions from binding SpawnProjectile directly to input.
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay")
	float FireRate;
	
	// Timer to control FireRate
	FTimerHandle FiringTimer;

	// If true, you are in the process of firing projectiles
	bool bIsShooting;

	
	
	bool bShouldHover = false;

	bool bMoving = false;
	bool bMoveDirectionForward = false; // To be used with `bMoving`

	bool bTurning = false;
	bool bTurnDirectionRight = false; // To be used with `bTurning`

	bool bIsClient = false;

	float GameGravity = 980.0f;

	float HoverTime = 0.0f;
	float MaxVelAchieved = 0.0f;

	// Network
	FNetClientPredStats NetClientPredStats;
	FNetClientMove CurrTickClientMove;
	FNetClientVisuals CurrTickClientVisuals;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Component References
	class UStaticMeshComponent* MeshComp;
	class USpringArmComponent* SpringArmComp;

	// Jet Flame Visuals
	class UStaticMeshComponent* JetFlameCenterMeshComp;
	class UStaticMeshComponent* JetFlameRightMeshComp;
	class UStaticMeshComponent* JetFlameLeftMeshComp;
	FVector JetFlameCenterScale;
	FVector JetFlameCenterPosition;
	FVector JetFlameRightScale;
	FVector JetFlameRightPosition;
	FVector JetFlameLeftScale;
	FVector JetFlameLeftPosition;

	// Thrust and Brake Flame Visuals
	UNiagaraComponent* ThrusterFlameCenterNS;
	UNiagaraComponent* ThrusterFlameLeftNS;
	UNiagaraComponent* ThrusterFlameRightNS;

	UNiagaraComponent* BrakeFlameCenterNS;
	UNiagaraComponent* BrakeFlameLeftNS;
	UNiagaraComponent* BrakeFlameRightNS;
	
	// Turning Flame Visuals
	UNiagaraComponent* TurnLeftNS;
	UNiagaraComponent* TurnRightNS;

	// Light Ridge
	UMaterialInstanceDynamic* LightRidgeMaterial;
	float LightRidgeLerpTimer = 0.0f;

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

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* FireInputAction;


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


	// Vehicle Visuals
	void UpdateLightRidgeColor(float DeltaTime);
	void UpdateJetFlameVisuals(float DeltaTime);
	void SetThrustFlameVisuals();
	void SetTurningFlameVisuals();

	// Health
	//
	void OnHealthUpdate();
	
	// Getters for Health variables
	UFUNCTION(BlueprintPure, Category = "Vehicle | Health")
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Vehicle | Health")
	FORCEINLINE float GetCurrentHealth() const { return CurrentHealth; }

	// Setter for Current Health. Clamps the value between 0 and MaxHealth and calls OnHealthUpdate. 
	// Should only be called on the server.
	UFUNCTION(BlueprintCallable, Category = "Vehicle | Health")
	void SetCurrentHealth(float HealthValue);

	// Event for taking damage. Overridden from APawn.
	UFUNCTION(BlueprintCallable, Category = "Vehicle | Health")
	float TakeDamage(float DamageTaken, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;


	// Shooting
	//
	// Function for beginning weapon fire.
	UFUNCTION()
	void StartShooting();

	// Function for ending weapon fire. Once this is called, the player can use StartFire again
	UFUNCTION()
	void StopShooting();


	// Replication
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Update Health
	UFUNCTION()
	void OnRep_CurrentHealth();

	// RPC Calls

	// Notify Server About Movement
	UFUNCTION(Server, Unreliable)
	void RPC_Server_UpdateMovement(FNetClientPredStats NetClientPredStatsParam);

	// Notify Server About Shooting
	UFUNCTION(Server, Unreliable)
	void RPC_Server_HandleShooting();

	// Notify Everyone About Visuals
	// Must be Called by CLIENT
	UFUNCTION(Server, Unreliable)
	void RPC_Server_UpdateVisuals(FNetClientVisuals NewVisuals);

	// Notify Everyone About Visuals
	// Must be Called by SERVER
	UFUNCTION(NetMulticast, Unreliable)
	void RPC_Multicast_UpdateVisuals(FNetClientVisuals NewVisuals);
};

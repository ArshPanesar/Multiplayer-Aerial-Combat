// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VehicleProjectile.generated.h"

UCLASS()
class AERIALCOMBAT_API AVehicleProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AVehicleProjectile();

	// Damage type and damage that will be done by this projectile
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage")
	TSubclassOf<class UDamageType> DamageType;

	// Damage dealt by this projectile.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage")
	float Damage;

	UPROPERTY(ReplicatedUsing = OnRep_MoveDirection)
	FVector MoveDirection;

	UPROPERTY(ReplicatedUsing = OnRep_StartPosition)
	FVector StartPosition;
	
	UPROPERTY(ReplicatedUsing = OnRep_AddVelocity)
	FVector AddVelocity;

	static FVector CurrentSpawnLocation; // To be Updated by Player
	static FVector CurrentInstigatorVelocity; // To be Updated by Player
	static FVector CurrentSpawnDirection; // To be Updated by Player

	float RepCount = 0;
	float RequiredRepCount = 3;

	float InitialSpeed;

	bool bInitOnRemote = false;
	bool bReplicationComplete = false;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Components
	class USphereComponent* SphereComp;
	class UStaticMeshComponent* StaticMesh;
	class UProjectileMovementComponent* ProjectileMovementComponent;


	// Actor Destroyed
	virtual void Destroyed() override;

	// Delegate called when SphereComponent begins Overlapping something
	UFUNCTION(Category = "Projectile")
	void OnProjectileBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void SetDirection(FVector Direction, FVector AddVel = FVector());


	// Replication
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION()
	void OnRep_MoveDirection();

	UFUNCTION()
	void OnRep_StartPosition();

	UFUNCTION()
	void OnRep_AddVelocity();
};

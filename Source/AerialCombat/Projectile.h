// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Components/SphereComponent.h"
#include "ACPlayerController.h"
#include <GameFramework/ProjectileMovementComponent.h>

#include "Projectile.generated.h"

UCLASS()
class AERIALCOMBAT_API AProjectile : public AActor
{
	GENERATED_BODY()

protected:

	/** This projectile's ID. Used to link fake and authoritative projectiles. Only valid on the owning client (i.e. the
	 * client with the fake projectile). NULL_PROJECTILE_ID on other machines. Set before BeginPlay. */
	UPROPERTY(Replicated)
	uint32 ProjectileId;

	/** Whether this projectile is a fake client-side projectile. Set before BeginPlay. */
	bool bIsFakeProjectile;

	/** The fake projectile that is representing this actor on the owning client, if this actor is the authoritative
	 * server-side projectile. */
	UPROPERTY()
	TObjectPtr<AProjectile> LinkedFakeProjectile;

	/** The authoritative server-side projectile that is representing this actor on the server and remote clients, if
	 * this actor is the fake client-side projectile. */
	UPROPERTY()
	TObjectPtr<AProjectile> LinkedAuthProjectile;

public:	

	// Damage type and damage that will be done by this projectile
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage")
	TSubclassOf<class UDamageType> DamageType;

	// Damage dealt by this projectile.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage")
	float Damage;

	USphereComponent* SphereCollision;
	UProjectileMovementComponent* ProjectileMovement;
	UStaticMeshComponent* StaticMesh;

	// Sets default values for this actor's properties
	AProjectile();

	/** Default constructor. */
	AProjectile(const FObjectInitializer& ObjectInitializer);

	/** Initialize this projectile's ID. */
	FORCEINLINE void InitProjectileId(uint32 InProjectileId) { ProjectileId = InProjectileId; }

	/** Initialize this projectile as the fake projectile. */
	void InitFakeProjectile(AACPlayerController* OwningPlayer, uint32 InProjectileId);

	// Delegate called when SphereComponent begins Overlapping something
	UFUNCTION(Category = "Projectile")
	void OnProjectileBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/** Link this authoritative projectile with its corresponding fake projectile. */
	void LinkFakeProjectile(AProjectile* InFakeProjectile);


public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};

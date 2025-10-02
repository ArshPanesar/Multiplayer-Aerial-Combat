// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectile.h"
#include <Net/UnrealNetwork.h>
#include <Kismet/GameplayStatics.h>
#include "Components/DecalComponent.h"

#include "CombatVehicle.h"

// Sets default values
AProjectile::AProjectile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	bReplicates = true;

	ProjectileId = NULL_PROJECTILE_ID;

	// Damage done to Vehicles
	DamageType = UDamageType::StaticClass();
	Damage = 5.0f;
}

AProjectile::AProjectile(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;

	ProjectileId = NULL_PROJECTILE_ID;

	// Damage done to Vehicles
	DamageType = UDamageType::StaticClass();
	Damage = 5.0f;
}

void AProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	constexpr bool bUsePushModel = true;

	FDoRepLifetimeParams OwnerOnlyParams{ COND_OwnerOnly, REPNOTIFY_Always, bUsePushModel };
	DOREPLIFETIME_WITH_PARAMS_FAST(AProjectile, ProjectileId, OwnerOnlyParams);
}

void AProjectile::InitFakeProjectile(AACPlayerController* OwningPlayer, uint32 InProjectileId)
{
	if (InProjectileId == NULL_PROJECTILE_ID)
	{
		return;
	}

	bIsFakeProjectile = true;
	if (OwningPlayer)
	{
		OwningPlayer->FakeProjectiles.Add(InProjectileId, this);
	}
}

// Called when the game starts or when spawned
void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	AACPlayerController* PlayerCont = GetInstigator() ? GetInstigatorController<AACPlayerController>() : nullptr;
	if (!PlayerCont)
	{
		Destroy();
		return;
	}

	SphereCollision = FindComponentByClass<USphereComponent>();
	StaticMesh = FindComponentByClass<UStaticMeshComponent>();
	ProjectileMovement = FindComponentByClass<UProjectileMovementComponent>();

	// Initialize the replicated authoritative projectile once it's replicated to the owning client.
	if (!HasAuthority() && (ProjectileId != NULL_PROJECTILE_ID))
	{
		// Link to the associated fake projectile.
		if (ensureAlwaysMsgf(PlayerCont->FakeProjectiles.Contains(ProjectileId), TEXT("Client-side authoritative projectile (%s) failed to find corresponding fake projectile with ID (%i)."), *GetNameSafe(this), ProjectileId))
		{
			LinkFakeProjectile(PlayerCont->FakeProjectiles[ProjectileId]);
			PlayerCont->FakeProjectiles.Remove(ProjectileId);
		}

		// Don't be Visible on the Client-Side, we will only render the Fake Projectile!
		StaticMesh->SetVisibility(false);
	}

	if (HasAuthority())
	{
		SphereCollision->OnComponentBeginOverlap.AddDynamic(this, &AProjectile::OnProjectileBeginOverlap);
		SphereCollision->OnComponentHit.AddDynamic(this, &AProjectile::OnProjectileHit);
	}
}

void AProjectile::OnProjectileBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Hit)
{
	// Ignore Collision if Owner
	if (OtherActor == GetInstigator())
		return;
	if (OtherActor)
	{
		// Apply Damage
		UGameplayStatics::ApplyPointDamage(OtherActor, Damage, FVector(), Hit, GetInstigator()->Controller, this, DamageType);
		
		// Spawn Decal
		if (ACombatVehicle* CV = Cast<ACombatVehicle>(OtherActor))
		{
			CV->RPC_Server_SpawnDecal(Hit.Location, (-Hit.ImpactNormal).Rotation(), DecalSize);
		}

		// Queue for Destroy
		Destroy();
	}
}

void AProjectile::OnProjectileHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Queue for Destroy
	Destroy();
}

void AProjectile::LinkFakeProjectile(AProjectile* InFakeProjectile)
{
	LinkedFakeProjectile = InFakeProjectile;
	InFakeProjectile->LinkedAuthProjectile = this;
}

// Called every frame
void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


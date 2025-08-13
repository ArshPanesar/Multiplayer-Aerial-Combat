// Fill out your copyright notice in the Description page of Project Settings.


#include "VehicleProjectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Particles/ParticleSystem.h"
#include <Kismet/GameplayStatics.h>

// Sets default values
AVehicleProjectile::AVehicleProjectile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Replication
	bReplicates = true;

	// Damage done to Vehicles
	DamageType = UDamageType::StaticClass();
	Damage = 5.0f;
}

// Called when the game starts or when spawned
void AVehicleProjectile::BeginPlay()
{
	Super::BeginPlay();

	// Setup Components
	SphereComp = FindComponentByClass<USphereComponent>();
	check(SphereComp != nullptr);
	
	StaticMesh = FindComponentByClass<UStaticMeshComponent>();
	check(StaticMesh != nullptr);

	// Register the Projectile Impact function on a Hit
	if (HasAuthority())
	{
		SphereComp->OnComponentBeginOverlap.AddDynamic(this, &AVehicleProjectile::OnProjectileBeginOverlap);
	}
}

void AVehicleProjectile::Destroyed()
{
	// Do Nothing for now
}

void AVehicleProjectile::OnProjectileBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Hit)
{
	// Ignore Collision if Owner
	if (OtherActor == GetOwner())
		return;
	if (OtherActor)
	{
		UGameplayStatics::ApplyPointDamage(OtherActor, Damage, FVector(), Hit, GetInstigator()->Controller, this, DamageType);
		Destroy();
	}
}

// Called every frame
void AVehicleProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


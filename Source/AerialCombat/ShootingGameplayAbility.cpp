// Fill out your copyright notice in the Description page of Project Settings.


#include "ShootingGameplayAbility.h"

UShootingGameplayAbility::UShootingGameplayAbility()
{
	ProjectileClass = LoadClass<AProjectile>(nullptr, TEXT("/Game/Blueprints/PredictedProjectile/BP_Projectile.BP_Projectile_C"));
	
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ClientOrServer;

	/*ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;*/
}

void UShootingGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	AController* Cont = Cast<APawn>(GetAvatarActorFromActorInfo())->GetController();

	FVector SpawnPoint = FVector();
	FRotator SpawnRotation = FRotator();
	
	Cont->GetPlayerViewPoint(SpawnPoint, SpawnRotation);
	
	FVector Up = FRotationMatrix(SpawnRotation).GetUnitAxis(EAxis::Z);
	SpawnPoint = SpawnPoint - Up * 15.0f;
	
	UAbilityTask_SpawnPredProjectile* Task = UAbilityTask_SpawnPredProjectile::SpawnPredProjectile(this, ProjectileClass, SpawnPoint, SpawnRotation);
	if (Task)
	{
		Task->Success.AddDynamic(this, &UShootingGameplayAbility::StopShootingAbility);
		Task->FailedToSpawn.AddDynamic(this, &UShootingGameplayAbility::FailedShootingAbility);
		
		Task->ReadyForActivation();
	}
}

void UShootingGameplayAbility::StopShootingAbility(AProjectile* SpawnedProjectile)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UShootingGameplayAbility::FailedShootingAbility(AProjectile* SpawnedProjectile)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilityTask_SpawnPredProjectile.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/PlayerState.h"
#include "Projectile.h"
#include "CVAbilitySystemComponent.h"

FActorSpawnParameters UAbilityTask_SpawnPredProjectile::GenerateSpawnParams() const
{
	FActorSpawnParameters Params;
	
	// Instigator will be the Player's Pawn (AvatarActor)
	Params.Instigator = Ability->GetCurrentActorInfo()->AvatarActor.IsValid() ? Cast<APawn>(Ability->GetCurrentActorInfo()->AvatarActor) : nullptr;
	// Owner is the Player State
	Params.Owner = AbilitySystemComponent->GetOwnerActor();
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	return Params;
}

FActorSpawnParameters UAbilityTask_SpawnPredProjectile::GenerateSpawnParamsForFake(const uint32 ProjectileId) const
{
	FActorSpawnParameters Params = GenerateSpawnParams();
	AACPlayerController* PlayerCont = Cast<AACPlayerController>(Ability->GetCurrentActorInfo()->PlayerController);
	Params.CustomPreSpawnInitalization = [ProjectileId, PlayerCont](AActor* Actor)
	{
		// Initialize ID
		if (AProjectile* Projectile = Cast<AProjectile>(Actor))
		{
			Projectile->InitProjectileId(ProjectileId);
			Projectile->InitFakeProjectile(PlayerCont, ProjectileId);
		}
	};
	return Params;
}

FActorSpawnParameters UAbilityTask_SpawnPredProjectile::GenerateSpawnParamsForAuth(const uint32 ProjectileId) const
{
	FActorSpawnParameters Params = GenerateSpawnParams();
	Params.CustomPreSpawnInitalization = [ProjectileId](AActor* Actor)
	{
		// Initialize ID
		if (AProjectile* Projectile = Cast<AProjectile>(Actor))
		{
			Projectile->InitProjectileId(ProjectileId);
		}
	};
	return Params;
}

void UAbilityTask_SpawnPredProjectile::SpawnDelayedFakeProjectile()
{
	if (Ability && Ability->GetCurrentActorInfo() && DelayedProjectileInfo.PlayerCont.IsValid())
	{
		if (AProjectile* NewProjectile = GetWorld()->SpawnActor<AProjectile>(DelayedProjectileInfo.ProjectileClass, DelayedProjectileInfo.SpawnLocation, 
			DelayedProjectileInfo.SpawnRotation, GenerateSpawnParamsForFake(DelayedProjectileInfo.ProjectileId)))
		{
			// Send Spawn Data to Server so that the Server can actually spawn an Authoritative Version
			NewProjectile->ProjectileMovement->Velocity += GetAvatarActor()->GetVelocity();
			SendSpawnDataToServer(SpawnLocation, SpawnRotation, DelayedProjectileInfo.ProjectileId);

			SpawnedFakeProj = NewProjectile;

			if (ShouldBroadcastAbilityTaskDelegates())
			{
				Success.Broadcast(NewProjectile);
			}

			// We don't end the task here because we need to keep listening for possible rejection.

			return;
		}
	}

	// Cancel the task on the server if the client failed.
	CancelServerSpawn();

	// Failed to spawn. Ability should usually be cancelled at this point.
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		FailedToSpawn.Broadcast(nullptr);
	}

	EndTask();
}

void UAbilityTask_SpawnPredProjectile::OnTaskRejected()
{
	AACPlayerController* PlayerCont = (Ability && Ability->GetCurrentActorInfo()) ? Cast<AACPlayerController>(Ability->GetCurrentActorInfo()->PlayerController) : nullptr;

	// If we've spawned a fake projectile on the client, destroy it.
	if (SpawnedFakeProj.IsValid())
	{
		// The fake projectile will still be lingering on the PC's list of unlinked projectiles; we need to remove it.
		if (PlayerCont)
		{
			if (const uint32* Key = PlayerCont->FakeProjectiles.FindKey(SpawnedFakeProj.Get()))
			{
				PlayerCont->FakeProjectiles.Remove(*Key);
			}
		}

		SpawnedFakeProj.Get()->Destroy();
	}

	// If we didn't spawn the fake projectile yet (because we're waiting for a delayed spawn), cancel it.
	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
}

void UAbilityTask_SpawnPredProjectile::SendSpawnDataToServer(const FVector& InLocation, const FRotator& InRotation, uint32 InProjectileId)
{
	const bool bGenerateNewKey = !AbilitySystemComponent->ScopedPredictionKey.IsValidForMorePrediction();

	FScopedPredictionWindow ScopedPrediction(AbilitySystemComponent.Get(), bGenerateNewKey);
	FGameplayAbilityTargetDataHandle Handle = FGameplayAbilityTargetData_ProjectileSpawnInfo::MakeProjectileSpawnInfoTargetData(InLocation, InRotation, InProjectileId);
	
	AbilitySystemComponent->CallServerSetReplicatedTargetData(GetAbilitySpecHandle(), GetActivationPredictionKey(), Handle, FGameplayTag(), AbilitySystemComponent->ScopedPredictionKey);
}

void UAbilityTask_SpawnPredProjectile::OnSpawnDataReplicated(const FGameplayAbilityTargetDataHandle& Data, FGameplayTag Activation)
{
	// Copy the target data before we consume it.
	const FGameplayAbilityTargetData* TargetData = Data.Get(0);

	// Consume the client's data. Ensures each server task only spawns one projectile for each client task.
	if (!Cast<UCVAbilitySystemComponent>(AbilitySystemComponent)->TryConsumeClientReplicatedTargetData(GetAbilitySpecHandle(), GetActivationPredictionKey()))
	{
		return;
	}

	if (TargetData)
	{
		if (const FGameplayAbilityTargetData_ProjectileSpawnInfo* SpawnInfo = static_cast<const FGameplayAbilityTargetData_ProjectileSpawnInfo*>(TargetData))
		{
			AACPlayerController* PlayerCont = Ability->GetCurrentActorInfo()->PlayerController.IsValid() ? Cast<AACPlayerController>(Ability->GetCurrentActorInfo()->PlayerController.Get()) : nullptr;
			const float ForwardPredictionTime = PlayerCont->GetForwardPredictionTime();

			if (AProjectile* NewProjectile = GetWorld()->SpawnActor<AProjectile>(Projectile, SpawnInfo->SpawnLocation, SpawnInfo->SpawnRotation, GenerateSpawnParamsForAuth(SpawnInfo->ProjectileId)))
			{
				// Note that there will be a discrepancy between the server's perceived ping and the client's.
				//
				if (NewProjectile->ProjectileMovement)
				{
					// Tick the actor (e.g. animations, VFX).
					if (NewProjectile->PrimaryActorTick.IsTickFunctionEnabled())
					{
						NewProjectile->TickActor(ForwardPredictionTime * NewProjectile->CustomTimeDilation, LEVELTICK_All, NewProjectile->PrimaryActorTick);
					}

					// Tick the movement component (to actually move the projectile).
					NewProjectile->ProjectileMovement->TickComponent(ForwardPredictionTime * NewProjectile->CustomTimeDilation, LEVELTICK_All, nullptr);
					if (NewProjectile->GetLifeSpan() > 0.0f)
					{
						/* Since we're fast-forwarding this actor, we need to subtract the forward prediction time from
						 * its lifespan. Clamp at 0.2 so we have enough time to replicate. */
						NewProjectile->SetLifeSpan(FMath::Max(0.2f, NewProjectile->GetLifeSpan() - ForwardPredictionTime));
					}
				}

				if (ShouldBroadcastAbilityTaskDelegates())
				{
					Success.Broadcast(NewProjectile);
				}

				EndTask();

				return;
			}
		}
	}

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		FailedToSpawn.Broadcast(nullptr);
	}

	EndTask();
}

void UAbilityTask_SpawnPredProjectile::OnSpawnDataCancelled()
{
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		FailedToSpawn.Broadcast(nullptr);
	}

	EndTask();
}

void UAbilityTask_SpawnPredProjectile::CancelServerSpawn()
{
	const bool bGenerateNewKey = !AbilitySystemComponent->ScopedPredictionKey.IsValidForMorePrediction();
	FScopedPredictionWindow ScopedPrediction(AbilitySystemComponent.Get(), bGenerateNewKey);
	AbilitySystemComponent->ServerSetReplicatedTargetDataCancelled(GetAbilitySpecHandle(), GetActivationPredictionKey(), AbilitySystemComponent->ScopedPredictionKey);
}

UAbilityTask_SpawnPredProjectile* UAbilityTask_SpawnPredProjectile::SpawnPredProjectile(UGameplayAbility* OwningAbility, TSubclassOf<AProjectile> ProjectileClass, FVector SpawnLocation, FRotator SpawnRotation)
{
    if (!ensureAlwaysMsgf(OwningAbility->GetNetExecutionPolicy() == EGameplayAbilityNetExecutionPolicy::LocalPredicted, 
        TEXT("SpawnPredictedProjectile ability task activated in ability (%s), which does not have a net execution policy of Local Predicted. SpawnPredictedProjectile should only be used in predicted abilities. Use AbilityTask_SpawnActor otherwise."), *GetNameSafe(OwningAbility)))
    {
        return nullptr;
    }

	if (!ensureAlwaysMsgf(IsValid(ProjectileClass), 
		TEXT("SpawnPredictedProjectile ability task activated in ability (%s) without a valid projectile class set."), *GetNameSafe(OwningAbility)))
	{
		return nullptr;
	}

	// Instantiate the Ability Task
	UAbilityTask_SpawnPredProjectile* Task = NewAbilityTask<UAbilityTask_SpawnPredProjectile>(OwningAbility);
	Task->Projectile = ProjectileClass;
	Task->SpawnLocation = SpawnLocation;
	Task->SpawnRotation = SpawnRotation;

	return Task;
}

void UAbilityTask_SpawnPredProjectile::Activate()
{
	Super::Activate();

	// On the client, listen for if this task is rejected. If it is, we need to destroy our fake projectile.
	if (IsPredictingClient())
	{
		GetActivationPredictionKey().NewRejectedDelegate().BindUObject(this, &UAbilityTask_SpawnPredProjectile::OnTaskRejected);
	}

	// Activate Task
	if (Ability && Ability->GetCurrentActorInfo())
	{
		if (AACPlayerController* PlayerCont = Ability->GetCurrentActorInfo()->PlayerController.IsValid() ? Cast<AACPlayerController>(Ability->GetCurrentActorInfo()->PlayerController.Get()) : nullptr)
		{
			// Figure out current Instance Context
			const float ForwardPredictionTime = PlayerCont->GetForwardPredictionTime();
			const bool bIsNetAuthority = Ability->GetCurrentActorInfo()->IsNetAuthority();
			const bool bShouldUseServerInfo = IsLocallyControlled();

			// Perform on Server
			if (bIsNetAuthority && !bShouldUseServerInfo)
			{
				const FGameplayAbilitySpecHandle& SpecHandle = GetAbilitySpecHandle();
				const FPredictionKey& ActivationPredictionKey = GetActivationPredictionKey();

				AbilitySystemComponent->AbilityTargetDataSetDelegate(SpecHandle, ActivationPredictionKey).AddUObject(this, &UAbilityTask_SpawnPredProjectile::OnSpawnDataReplicated);
				AbilitySystemComponent->AbilityTargetDataCancelledDelegate(SpecHandle, ActivationPredictionKey).AddUObject(this, &UAbilityTask_SpawnPredProjectile::OnSpawnDataCancelled);

				// Check if the client already sent the data.
				AbilitySystemComponent->CallReplicatedTargetDataDelegatesIfSet(SpecHandle, ActivationPredictionKey);

				// Kill the ability if we never receive the data.
				SetWaitingOnRemotePlayerData();
				
				return;
			}

			// Perform on Local Client
			if (!bIsNetAuthority)
			{
				/* On clients, if our ping is too high to forward-predict, delay spawning the projectile so we don't
				 * forward-predict further than MaxPredictionPing. */
				float SleepTime = PlayerCont->GetProjectileSleepTime();
				if (SleepTime > 0.0f)
				{
					if (!GetWorld()->GetTimerManager().IsTimerActive(SpawnDelayedFakeProjHandle))
					{
						// Set a timer to spawn the predicted projectile after a delay.
						DelayedProjectileInfo.ProjectileClass = Projectile;
						DelayedProjectileInfo.SpawnLocation = SpawnLocation;
						DelayedProjectileInfo.SpawnRotation = SpawnRotation;
						DelayedProjectileInfo.PlayerCont = PlayerCont;
						DelayedProjectileInfo.ProjectileId = PlayerCont->GenerateNewFakeProjectileID();
						GetWorld()->GetTimerManager().SetTimer(SpawnDelayedFakeProjHandle, this, &UAbilityTask_SpawnPredProjectile::SpawnDelayedFakeProjectile, SleepTime, false);
					}


					return;
				}

				// If our ping is low enough to forward-predict (or we're on LAN), immediately spawn and initialize the fake projectile.
				const uint32 FakeProjectileId = PlayerCont->GenerateNewFakeProjectileID();
				if (AProjectile* NewProjectile = GetWorld()->SpawnActor<AProjectile>(Projectile, SpawnLocation, SpawnRotation, GenerateSpawnParamsForFake(FakeProjectileId)))
				{
					// Send Spawn Data to Server so that the Server can actually spawn an Authoritative Version
					NewProjectile->ProjectileMovement->Velocity += GetAvatarActor()->GetVelocity();
					SendSpawnDataToServer(SpawnLocation, SpawnRotation, FakeProjectileId);

					// Cache the projectile in case the server rejects this task, and we have to destroy it.
					SpawnedFakeProj = NewProjectile;

					if (ShouldBroadcastAbilityTaskDelegates())
					{
						Success.Broadcast(NewProjectile);
					}

					// We don't end the task here because we need to keep listening for possible rejection.

					return;
				}
			}
			// Perform on Authority (Listen Server)
			/* On listen servers or in standalone, spawn the authoritative projectile. No prediction is needed in this
			 * case. Remote servers don't spawn authoritative actors until OnTargetDataReplicated. */
			else if (bIsNetAuthority && bShouldUseServerInfo)
			{
				if (AProjectile* NewProjectile = GetWorld()->SpawnActor<AProjectile>(Projectile, SpawnLocation, SpawnRotation, GenerateSpawnParams()))
				{
					NewProjectile->ProjectileMovement->Velocity += GetAvatarActor()->GetVelocity();

					if (ShouldBroadcastAbilityTaskDelegates())
					{
						Success.Broadcast(NewProjectile);
					}

					EndTask();

					return;
				}
			}
		}
	}

	// Cancel the task on the server if the client failed.
	CancelServerSpawn();

	// Failed to spawn. Ability should usually be cancelled at this point.
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		FailedToSpawn.Broadcast(nullptr);
	}

	EndTask();
}

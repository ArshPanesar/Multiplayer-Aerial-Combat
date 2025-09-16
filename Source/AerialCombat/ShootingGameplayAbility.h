// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Abilities/GameplayAbility.h"
//#include "Abilities/GameplayAbilitySpec.h"

#include "Projectile.h"
#include "AbilityTask_SpawnPredProjectile.h"

#include "ShootingGameplayAbility.generated.h"

struct FCVGameplayAbilitySpec : public FGameplayAbilitySpec
{
    FCVGameplayAbilitySpec() = default;

    FCVGameplayAbilitySpec(TSubclassOf<UGameplayAbility> InAbilityClass, int32 InLevel = 1, int32 InInputID = INDEX_NONE, UObject* InSourceObject = nullptr)
        : FGameplayAbilitySpec(InAbilityClass, InLevel, InInputID, InSourceObject)
    {
    }

    float ProjectileSpawnOffsetDown = 10.0f;
};

/**
 * 
 */
UCLASS()
class AERIALCOMBAT_API UShootingGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

private:
    TSubclassOf<AProjectile> ProjectileClass;

public:
    UShootingGameplayAbility();

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, 
        const FGameplayAbilityActivationInfo ActivationInfo,const FGameplayEventData* TriggerEventData) override;

    UFUNCTION()
    void StopShootingAbility(AProjectile* SpawnedProjectile);


    UFUNCTION()
    void FailedShootingAbility(AProjectile* SpawnedProjectile);
};

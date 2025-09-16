// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"

#include "ACPlayerController.h"

#include "AbilityTask_SpawnPredProjectile.generated.h"

// Based on https://sreitich.github.io/projectile-prediction-2/

/**
 * Cached spawn information for predicted projectiles that are spawned after a delay. Projectile spawning is delayed
 * when the client's ping is higher than the maximum forward prediction ping to prevent projectiles from being
 * forward-predicted too far.
 */
USTRUCT()
struct FDelayedProjectileInfo
{
    GENERATED_BODY()

    UPROPERTY()
    TSubclassOf<AProjectile> ProjectileClass;

    UPROPERTY()
    FVector SpawnLocation;

    UPROPERTY()
    FRotator SpawnRotation;

    UPROPERTY()
    TWeakObjectPtr<AACPlayerController> PlayerCont;

    UPROPERTY()
    uint32 ProjectileId;

    FDelayedProjectileInfo() :
        ProjectileClass(nullptr),
        SpawnLocation(ForceInit),
        SpawnRotation(ForceInit),
        PlayerCont(nullptr),
        ProjectileId(0)
    {
    }
};

/**
 * Target data for spawning projectiles. Used to send spawn information from the client to the server.
 */
USTRUCT()
struct FGameplayAbilityTargetData_ProjectileSpawnInfo : public FGameplayAbilityTargetData
{
    GENERATED_BODY()

    /** Location to spawn projectile at. */
    UPROPERTY()
    FVector SpawnLocation;

    /** Rotation with which to spawn projectile. */
    UPROPERTY()
    FRotator SpawnRotation;

    /** The projectile's ID. Used to link fake and authoritative projectiles. */
    UPROPERTY()
    uint32 ProjectileId;

    FGameplayAbilityTargetData_ProjectileSpawnInfo() :
        SpawnLocation(ForceInit),
        SpawnRotation(ForceInit),
        ProjectileId(0)
    {
    }

    virtual UScriptStruct* GetScriptStruct() const override
    {
        return FGameplayAbilityTargetData_ProjectileSpawnInfo::StaticStruct();
    }

    virtual FString ToString() const override
    {
        return FString::Printf(TEXT("FGameplayAbilityTargetData_ProjectileSpawnInfo: (%i)"), ProjectileId);
    }

    static FGameplayAbilityTargetDataHandle MakeProjectileSpawnInfoTargetData(const FVector& SpawnLocation, const FRotator& SpawnRotation, const uint32 ProjectileId)
    {
        // Allocate and Initialize our Target Data
        FGameplayAbilityTargetData_ProjectileSpawnInfo* TargetData = new FGameplayAbilityTargetData_ProjectileSpawnInfo();
        TargetData->SpawnLocation = SpawnLocation;
        TargetData->SpawnRotation = SpawnRotation;
        TargetData->ProjectileId = ProjectileId;

        // Provide a Handle to the Heap Allocated Data
        FGameplayAbilityTargetDataHandle Handle;
        Handle.Data.Add(TSharedPtr<FGameplayAbilityTargetData_ProjectileSpawnInfo>(TargetData));
        
        return Handle;
    }

    bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
    {
        Ar << SpawnLocation;
        Ar << SpawnRotation;
        Ar << ProjectileId;

        bOutSuccess = true;
        return true;
    }
};

template<>
struct TStructOpsTypeTraits<FGameplayAbilityTargetData_ProjectileSpawnInfo> : public TStructOpsTypeTraitsBase2<FGameplayAbilityTargetData_ProjectileSpawnInfo>
{
    enum
    {
        WithNetSerializer = true
    };
};


// Delegate for Task to Continue Execution
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSpawnPredictedProjectileDelegate, AProjectile*, SpawnedProjectile);

/**
 * 
 */
UCLASS()
class AERIALCOMBAT_API UAbilityTask_SpawnPredProjectile : public UAbilityTask
{
	GENERATED_BODY()
	
public:
    /**
    * Called locally when the projectile is spawned.
    *
    * On clients, this will return the fake projectile actor. On the server, this will return the authoritative
    * projectile actor.
    */
    UPROPERTY(BlueprintAssignable)
    FSpawnPredictedProjectileDelegate Success;

    /** Called on the client and server if either failed to spawn their projectile actor, usually because the ability's
     * prediction key was rejected. The ability should likely be cancelled (on both machines) at this point. */
    UPROPERTY(BlueprintAssignable)
    FSpawnPredictedProjectileDelegate FailedToSpawn;

protected:
    // Projectile Class to Spawn
    UPROPERTY()
    TSubclassOf<AProjectile> Projectile;

    FVector SpawnLocation;
    FRotator SpawnRotation;

    //
    // Projectile Spawning
    //

    /** Helper to make spawn parameters for a projectile. */
    FActorSpawnParameters GenerateSpawnParams() const;

    /** Helper to make spawn parameters for a fake projectile. */
    FActorSpawnParameters GenerateSpawnParamsForFake(const uint32 ProjectileId) const;

    /** Helper to make spawn parameters for an authoritative projectile. */
    FActorSpawnParameters GenerateSpawnParamsForAuth(const uint32 ProjectileId) const;

    /** The fake projectile spawned by this task on the client. Used to destroy the spawned projectile if this task is
     * rejected. */
    TWeakObjectPtr<AProjectile> SpawnedFakeProj;


    // 
    // Delayed Projectile Spawning
    //

    /** Handle for spawning a fake projectile after a delay, when the client's ping is higher than the
    * forward-prediction limit. */
    FTimerHandle SpawnDelayedFakeProjHandle;

    /** Cached spawn info for spawning a fake projectile after a delay. */
    UPROPERTY()
    FDelayedProjectileInfo DelayedProjectileInfo;

    /** Spawns a fake projectile using the DelayedProjectileInfo. */
    void SpawnDelayedFakeProjectile();
    
    /** Destroys the client's fake projectile if this task is rejected (e.g. the server rejects the ability
    * activation). */
    UFUNCTION()
    void OnTaskRejected();

    //
    // Spawning Projectile on Server
    //

    /** Replicates the client's spawn data to the server, so the server can spawn the authoritative projectile. */
    void SendSpawnDataToServer(const FVector& InLocation, const FRotator& InRotation, uint32 InProjectileId);

    /** Spawns the authoritative projectile on the server when the spawn data is received from the client. */
    void OnSpawnDataReplicated(const FGameplayAbilityTargetDataHandle& Data, FGameplayTag Activation);

    /** Cancels this task on the server if the client failed to spawn their version of the projectile. */
    void OnSpawnDataCancelled();

    /** Sends the task cancellation to the server if the client failed. */
    void CancelServerSpawn();

public:
    // Spawn a Predicated Projectile
    UFUNCTION(BlueprintCallable, Meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "True"), Category = "Ability|Tasks")
    static UAbilityTask_SpawnPredProjectile* SpawnPredProjectile(UGameplayAbility* OwningAbility, TSubclassOf<AProjectile> ProjectileClass, FVector SpawnLocation, FRotator SpawnRotation);

    // Spawns Two Projectiles: A "Fake" One on the Client, and the "Real" one on the Authoritative Server
    virtual void Activate() override;
};

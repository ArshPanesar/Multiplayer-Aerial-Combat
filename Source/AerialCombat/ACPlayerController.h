// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
//#include "Projectile.h"
#include "ACPlayerController.generated.h"

// We need IDs to Identify a linked Fake and Real Projectile
#define NULL_PROJECTILE_ID 0

class AProjectile;

/**
 * 
 */
UCLASS(Config = Game)
class AERIALCOMBAT_API AACPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
    // Constructor
    AACPlayerController();

    // Runs on Client-Side
    virtual void AcknowledgePossession(APawn* P) override;

    //
    // Network Prediction
    //

    // A "fudge factor" used to "fudge" the server's estimate of a client's ping to get a more accurate guess of their exact ping
    // We use this since Unreal increases the actual latency value due to processing time!
    UPROPERTY(BlueprintReadOnly, Config, Category = Network)
    float PredictionLatencyReduction;

    /* How much (from 0 - 1) to favor the client when determining the real position of predicted projectiles.
     * A greater value will spawn authoritative projectiles closer to where the client wants; a smaller value will spawn
     * authoritative projectiles closer to where the server wants (forwarding them less). */
    UPROPERTY(BlueprintReadOnly, Config, Category = Network)
    float ClientBiasPct;

    /* Max amount of ping to predict ahead for. If the client's ping exceeds this, we'll delay spawning the projectile
     * so it doesn't spawn further ahead than this. */
    UPROPERTY(BlueprintReadOnly, Config, Category = Network)
    float MaxPredictionPing;

    /** The amount of time, in seconds, to tick or simulate to make up for network lag. (1/2 player's ping) - prediction
     * latency reduction. */
    float GetForwardPredictionTime() const;

    /** How long to wait before spawning the projectile if the client's ping exceeds MaxPredictionPing, so we don't
     * forward-predict too far. */
    float GetProjectileSleepTime() const;


    //
    // Tracking Projectile IDs
    //

    /** List of this client's fake projectiles (client-side predicted projectiles) that haven't been linked to an
    * authoritative projectile yet. */
    UPROPERTY()
    TMap<uint32, TObjectPtr<AProjectile>> FakeProjectiles;

    /** Generates a unique ID for a new fake projectile. */
    uint32 GenerateNewFakeProjectileID();



private:

    /** Internal counter for projectile IDs. Starts at 1 because 0 is reserved for non-predicted projectiles. */
    uint32 FakeProjectileIDCounter;
};

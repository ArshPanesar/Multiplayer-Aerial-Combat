// Fill out your copyright notice in the Description page of Project Settings.


#include "ACPlayerController.h"
#include "GameFramework/PlayerState.h"
#include "CombatVehicle.h"
#include "AbilitySystemComponent.h"

AACPlayerController::AACPlayerController()
{
    // Initialize GAS Prediction Variables
    PredictionLatencyReduction = 20.0f;
    ClientBiasPct = 0.5f;
    MaxPredictionPing = 150.0f;
}

void AACPlayerController::AcknowledgePossession(APawn* P)
{
    Super::AcknowledgePossession(P);
}

float AACPlayerController::GetForwardPredictionTime() const
{
    // Divide by 1000 to convert ping from MS to S.
    return (PlayerState && (GetNetMode() != NM_Standalone)) ? (0.001f * ClientBiasPct * FMath::Clamp(PlayerState->ExactPing - (IsLocalController() ? 0.0f : PredictionLatencyReduction), 0.0f, MaxPredictionPing)) : 0.f;
}

float AACPlayerController::GetProjectileSleepTime() const
{
    // At high latencies, projectiles won't be spawned until they can be forward-predicted at the maximum prediction ping.
    return 0.001f * FMath::Max(0.0f, PlayerState->ExactPing - PredictionLatencyReduction - MaxPredictionPing);
}

uint32 AACPlayerController::GenerateNewFakeProjectileID()
{
    const uint32 NextID = FakeProjectileIDCounter;
    
    // Increment Next ID
    FakeProjectileIDCounter = FakeProjectileIDCounter < UINT32_MAX ? FakeProjectileIDCounter + 1 : 1;

    checkf(!FakeProjectiles.Contains(NextID), TEXT("Generated invalid projectile ID! ID (%i) already used. Fake projectile map size: (%i)."), NextID, FakeProjectiles.Num());
    return NextID;
}

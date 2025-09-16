// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "CVAbilitySystemComponent.generated.h"

/**
 * 
 */
UCLASS()
class AERIALCOMBAT_API UCVAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()


public:

	/** Consumes cached TargetData from client (only TargetData) and returns whether any data was actually consumed. */
	bool TryConsumeClientReplicatedTargetData(FGameplayAbilitySpecHandle AbilityHandle, FPredictionKey AbilityOriginalPredictionKey);
};

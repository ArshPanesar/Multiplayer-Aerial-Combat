// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"

#include "AbilitySystemInterface.h"
#include "CVAbilitySystemComponent.h"

#include "ShootingGameplayAbility.h"

#include "ACPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class AERIALCOMBAT_API AACPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	// Default Constructor
	AACPlayerState();

	virtual void BeginPlay() override;


	// ASC
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	UCVAbilitySystemComponent* AbilitySystemComponent;
};

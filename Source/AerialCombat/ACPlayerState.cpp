// Fill out your copyright notice in the Description page of Project Settings.


#include "ACPlayerState.h"

AACPlayerState::AACPlayerState()
{
	NumKills = 0;
	NumDeaths = 0;

	AbilitySystemComponent = CreateDefaultSubobject<UCVAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
}

void AACPlayerState::BeginPlay()
{
	Super::BeginPlay();

    if (HasAuthority() && AbilitySystemComponent)
    {
		FCVGameplayAbilitySpec Spec(UShootingGameplayAbility::StaticClass());
		Spec.ProjectileSpawnOffsetDown = 15.0f;
		AbilitySystemComponent->GiveAbility(Spec);
    }
}

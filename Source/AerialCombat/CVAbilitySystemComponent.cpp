// Fill out your copyright notice in the Description page of Project Settings.


#include "CVAbilitySystemComponent.h"

bool UCVAbilitySystemComponent::TryConsumeClientReplicatedTargetData(FGameplayAbilitySpecHandle AbilityHandle, FPredictionKey AbilityOriginalPredictionKey)
{
    TSharedPtr<FAbilityReplicatedDataCache> CachedData = AbilityTargetDataMap.Find(FGameplayAbilitySpecHandleAndPredictionKey(AbilityHandle, AbilityOriginalPredictionKey));
    if (CachedData.IsValid())
    {
        const bool bConsumed = CachedData->TargetData.Num() > 0;
        CachedData->TargetData.Clear();
        CachedData->bTargetConfirmed = false;
        CachedData->bTargetCancelled = false;
        return bConsumed;
    }
    return false;
}

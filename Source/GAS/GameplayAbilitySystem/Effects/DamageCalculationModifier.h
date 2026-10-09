// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "DamageCalculationModifier.generated.h"

/**
 * ダメージの計算ロジック
 * 「このダメージは物理か魔法か」を判定（Data.DamageType.〇〇）し、計算を変える。
 */
UCLASS()
class GAS_API UDamageCalculationModifier : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()
	
public:
	UDamageCalculationModifier();
	
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
	
private:
	// 属性のキャプチャ定義
	FGameplayEffectAttributeCaptureDefinition ArmorDef;
	FGameplayEffectAttributeCaptureDefinition PhysicalPowerDef;
	FGameplayEffectAttributeCaptureDefinition MagicalPowerDef;
	FGameplayEffectAttributeCaptureDefinition AdditionalMagicalDamageDef;
	FGameplayEffectAttributeCaptureDefinition AdditionalPhysicalDamageDef;
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CharacterClassDataAsset.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType)
struct FPrimaryStatValues
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) float Strength = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) float Knowledge = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) float Willpower = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) float Agility = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) float Vigor= 0.f;
};

UCLASS(BlueprintType)
class GAS_API UCharacterClassDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= "Class")
	FText ClassName;
	
	// このクラスの初期ステータス
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= "Class")
	FPrimaryStatValues BaseStats;
};

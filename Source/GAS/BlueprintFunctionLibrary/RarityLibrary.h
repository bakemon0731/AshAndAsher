// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GAS/Enum/Rarity.h"
#include "GAS/EquipmentSystem/DataAsset/EquipmentDataAsset.h"
#include "RarityLibrary.generated.h"

/**
 * 
 */
UCLASS()
class GAS_API URarityLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	// レアリティから対応する色を取得する
	UFUNCTION(BlueprintCallable,BlueprintPure,Category = "Rarity")
	static FLinearColor GetRarityColor(EItemRarity Rarity);
	
	//レアリティのEnumから対応するテキストを取得する
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Rarity")
	static FText GetRarityText(EItemRarity Rarity);
	
	// Enumから対応する装備スロットの表示名を取得
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Equipment")
	static FText GetEquipmentSlotText(EEquipmentSlot Slot);
};

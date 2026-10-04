// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayEffect.h"
#include "GAS/Enum/Rarity.h"
#include "EquipmentDataAsset.generated.h"

/**
 * 装備のデータアセット
 */

// ブループリントで使用可能にするためのマクロ
UENUM(BlueprintType)
enum class EEquipmentSlot : uint8
{
	//頭
	Head        UMETA(DisplayName = "Head"),
	//胴体
	Chest       UMETA(DisplayName = "Chest"),
	//手
	Hands       UMETA(DisplayName = "Hands"),
	//脚
	Legs        UMETA(DisplayName = "Legs"),
	//足
	Feet        UMETA(DisplayName = "Feet"),
	//マント
	Cloak       UMETA(DisplayName = "Cloak"),
	//指輪
	Ring        UMETA(DisplayName = "Ring"),
	//ネックレス
	Pendant		UMETA(DisplayName = "Pendant"),
	//無し
	None        UMETA(DisplayName = "None")
};

UCLASS(BlueprintType)
class GAS_API UEquipmentDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	
	//どの部位の装備か。
	UPROPERTY(EditAnywhere,BlueprintReadOnly, Category = "Equipment")
	EEquipmentSlot Slot = EEquipmentSlot::None;
	
	//名前
	UPROPERTY(EditAnywhere,BlueprintReadOnly, Category = "Equipment")
	FText DisplayName;
	
	//説明
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment", meta = (MultiLine = true))
	FText Description;
	
	//アイコン画像
	UPROPERTY(EditAnywhere,BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<UTexture2D> Icon;
	
	// この装備で適用されるPrimaryステータス。(意思や知識など)
	UPROPERTY(EditAnywhere,BlueprintReadOnly, Category = "Equipment")
	TArray<TSubclassOf<UGameplayEffect>> GrantedEffects;
	
	//グリッドの幅
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment",meta = (ClampMin = "1"))
	int32 GridWidth = 1;
	
	//グリッド高さ
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment",meta = (ClampMin = "1"))
	int32 GridHeight = 1;
	
	//このBPクラスとしてワールドにスポーンさせる
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	TSubclassOf<class AWorldItemActor> WorldItemClass;
	
	// このアイテムのレアリティ
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	EItemRarity Rarity = EItemRarity::Common;
};

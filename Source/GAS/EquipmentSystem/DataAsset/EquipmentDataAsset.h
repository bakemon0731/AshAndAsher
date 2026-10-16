// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayEffect.h"
#include "GAS/Enum/Rarity.h"
#include "GAS/Enum/EquipmentType.h"
#include "EquipmentDataAsset.generated.h"

/**
 * 装備のデータアセット
 */

// ブループリントで使用可能にするためのマクロ
UENUM(BlueprintType)
enum class EEquipmentSlot : uint8
{
	Head        UMETA(DisplayName = "頭"),
	Chest       UMETA(DisplayName = "胴体"),
	Hands       UMETA(DisplayName = "手"),
	Legs        UMETA(DisplayName = "脚"),
	Feet        UMETA(DisplayName = "足"),
	Cloak       UMETA(DisplayName = "マント"),
	Ring        UMETA(DisplayName = "指輪"),
	Pendant		UMETA(DisplayName = "ネックレス"),
	Utility     UMETA(DisplayName = "ユーティリティ"),
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
	
	//付与されるアイテムステータス
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment", meta = (MultiLine = true))
	FText ItemStatsGranted;
	
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
	
	// このアイテムの説明
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment",meta = (MultiLine = true))
	FText Description;
	
	// 移動速度ペナルティ値（正の数で入力。例：10.0）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment",meta = (ClampMin = "0.0"))
	float MoveSpeedPenalty = 0.0f;
	
	//このアイテムを１つのマスに重ねれる最大数（装備品は１のまま）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item",meta = (ClampMin = "1"))
	int32 MaxStackSize = 1;
	
	//このアイテムの種類
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	EItemCategory Category = EItemCategory::Equipment;
	
	//このアイテムを使用した時に自分へ適用するGE（回復など）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Use")
	TArray<TSubclassOf<UGameplayEffect>> UseEffects;
	
	//使用後、次に使えるまでの秒数
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Use", meta=(ClampMin = "0.0"))
	float UseCooldown = 1.0f;
};

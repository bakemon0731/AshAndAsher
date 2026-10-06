#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GAS/EquipmentSystem/DataAsset/EquipmentDataAsset.h"
#include "LootTableDataAsset.generated.h"

USTRUCT(BlueprintType)
struct FLootTableEntry
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UEquipmentDataAsset> Item = nullptr;

	// 抽選時の重み（他のエントリとの相対比率。大きいほど出やすい）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float Weight = 1.0f;

	// このエントリ自体が「出るかどうか」の独立判定（0.0～1.0）。
	// 例：トロールの激レア武器は Weight に関係なく DropChance=0.05 で単独判定したい場合に使う
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DropChance = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "1"))
	int32 MinCount = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "1"))
	int32 MaxCount = 1;
};

UCLASS(BlueprintType)
class GAS_API ULootTableDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 必ず出るアイテム（トロール確定ドロップ素材など）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot")
	TArray<TObjectPtr<UEquipmentDataAsset>> GuaranteedDrops;

	// 重み付き抽選プール（ランダムに N 個選ばれる）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot")
	TArray<FLootTableEntry> WeightedDrops;

	// WeightedDropsから何個選ぶか（ゴブリンなら1、トロールなら2-3など）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0"))
	int32 NumWeightedRolls = 1;

	// 抽選本体：GuaranteedDrops + 重み付き抽選の結果をまとめて返す
	TArray<UEquipmentDataAsset*> RollLoot() const;
};
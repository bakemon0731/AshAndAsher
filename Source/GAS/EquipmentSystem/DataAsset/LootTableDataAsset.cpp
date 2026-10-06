#include "LootTableDataAsset.h"

TArray<UEquipmentDataAsset*> ULootTableDataAsset::RollLoot() const
{
	TArray<UEquipmentDataAsset*> Result;

	//  確定ドロップ
	for (UEquipmentDataAsset* Item : GuaranteedDrops)
	{
		if (Item)
		{
			Result.Add(Item);
		}
	}

	//  重み付き抽選を NumWeightedRolls 回実施
	for (int32 Roll = 0; Roll < NumWeightedRolls; Roll++)
	{
		// まず各エントリの DropChance（単独判定）でプールを絞る
		TArray<const FLootTableEntry*> EligibleEntries;
		float TotalWeight = 0.f;

		for (const FLootTableEntry& Entry : WeightedDrops)
		{
			if (!Entry.Item)
			{
				continue;
			}
			if (FMath::FRand() <= Entry.DropChance)
			{
				EligibleEntries.Add(&Entry);
				TotalWeight += Entry.Weight;
			}
		}

		if (EligibleEntries.Num() == 0 || TotalWeight <= 0.f)
		{
			continue;
		}

		// 重みに応じて1つ選ぶ
		float RandomPoint = FMath::FRandRange(0.f, TotalWeight);
		float Accumulated = 0.f;
		const FLootTableEntry* Chosen = nullptr;

		for (const FLootTableEntry* Entry : EligibleEntries)
		{
			Accumulated += Entry->Weight;
			if (RandomPoint <= Accumulated)
			{
				Chosen = Entry;
				break;
			}
		}

		if (Chosen)
		{
			int32 Count = FMath::RandRange(Chosen->MinCount, Chosen->MaxCount);
			for (int32 i = 0; i < Count; i++)
			{
				Result.Add(Chosen->Item);
			}
		}
	}

	return Result;
}
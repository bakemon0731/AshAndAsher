// Fill out your copyright notice in the Description page of Project Settings.


#include "NexusEnemybase.h"
#include "GAS/GameplayAbilitySystem/Characters/PlayerCharacter/NexusPlayerbase.h"
#include "GAS/EquipmentSystem/DataAsset/LootTableDataAsset.h"
#include "GAS/EquipmentSystem/EquipmentComponent/AC_EquipmentComponent.h"


// Sets default values
ANexusEnemybase::ANexusEnemybase()
{
	PrimaryActorTick.bCanEverTick = true;
	
}

void ANexusEnemybase::BeginPlay()
{
	Super::BeginPlay();
}

//死亡時IsDead変数をTrueにする
inline void ANexusEnemybase::OnDeathTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	// IsDead・HandleDeath・近くのプレイヤー登録は基底が行う
	Super::OnDeathTagChanged(CallbackTag, NewCount); 

	// サーバー権限でのみルート抽選・付与を行う
	if (NewCount > 0 && HasAuthority() && LootTable)
	{
		if (UAC_EquipmentComponent* EquipComp = FindComponentByClass<UAC_EquipmentComponent>())
		{
			for (UEquipmentDataAsset* DropItem : LootTable->RollLoot())
			{
				EquipComp->AddItemToInventory(DropItem);
			}
		}
	}
}

// Called every frame
void ANexusEnemybase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void ANexusEnemybase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

FText ANexusEnemybase::GetActorDisplayName_Implementation() const
{
	// 敵の名前を返す。
	return EnemyDisplayName;
}



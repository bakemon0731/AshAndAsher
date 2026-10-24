// Fill out your copyright notice in the Description page of Project Settings.


#include "GA_EquipWeaponBase.h"
#include "Abilities/GameplayAbility.h"
#include "GAS/Weapon/WeaponComponent/AC_WeaponComponent.h"
#include "GAS/Weapon/WeaponBase/WeaponBase.h"
#include "GAS/EquipmentSystem/EquipmentComponent/AC_EquipmentComponent.h"

void UGA_EquipWeaponBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	// コストやクールダウンの適用チェック
	if (!CommitAbility(Handle,ActorInfo,ActivationInfo))
	{
        // コンポーネントが存在するか確認 
		EndAbility(Handle,ActorInfo,ActivationInfo,true,true);
		return;
	}
	
	// アバターアクターとイベントデータの存在確認
	if (ActorInfo && ActorInfo->AvatarActor.IsValid() && TriggerEventData)
	{
		AActor* AvatarActor = ActorInfo->AvatarActor.Get();
		
		// イベントの Event Magnitude から、武器スロット番号を取得する（0=左・1キー、1=右・2キー）
		const int32 SlotIndex = FMath::RoundToInt(TriggerEventData->EventMagnitude);
		
		if (UAC_EquipmentComponent* EquipComp = AvatarActor->FindComponentByClass<UAC_EquipmentComponent>())
		{
			// スロットの武器を持つ／もう一度で仕舞う。サーバーでのみ動く（EquipWeaponと同じ扱い）
			EquipComp->ToggleWeaponSlot(SlotIndex);
		}
	}
	// アビリティを終了
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UGA_EquipWeaponBase::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

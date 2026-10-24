// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GAS/GamePlayAbilitySystem/Characters/NexusCharacterBase.h"
#include "GAS/Weapon/WeaponBase/WeaponBase.h"
#include "AC_WeaponComponent.generated.h"

// イベントディスパッチャー用のデリゲート宣言
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponChanged,AWeaponBase*,EquippedWeapon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponUnequipped);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponEquipped);

//前方宣言
class UEquipmentDataAsset;

UCLASS(Blueprintable,ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GAS_API UAC_WeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UAC_WeaponComponent();
	
	// イベントディスパッチャー
	UPROPERTY(BlueprintAssignable,BlueprintCallable,Category= "Weapon Events");
	FOnWeaponChanged OnWeaponChanged;
	
	UPROPERTY(BlueprintAssignable,BlueprintCallable,Category= "Weapon Events");
	FOnWeaponUnequipped OnWeaponUnequipped;
	
	UPROPERTY(BlueprintAssignable,BlueprintCallable,Category= "Weapon Events");
	FOnWeaponEquipped OnWeaponEquipped;
	
	UPROPERTY(BlueprintReadWrite,ReplicatedUsing = OnRep_EquippedWeapon,Category= "Weapon")
	AWeaponBase* EquippedWeapon;
	
	//武器を持ている間だけMoveSpeedを下げるGEを適用する。（SetByCaller の Data.WeightPenalty を使う）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<UGameplayEffect> WieldMoveSpeedPenaltyEffect;
	
public:

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;
	
	// 武器を装備する関数
	UFUNCTION(BlueprintCallable, Category= "Weapon")
	void EquipWeapon (TSubclassOf<AWeaponBase> WeaponClass);
	
	// 武器を解除する関数
	UFUNCTION(BlueprintCallable, Category= "Weapon")
	void UnequipWeapon();
	
	// 手に持つための消耗品を出す。サーバー専用
	void EquipHeldItem(TSubclassOf<AWeaponBase> HeldClass);
	
	//手に持っている消耗品を仕舞い、持つ前の武器または素手に戻る。サーバー専用
	void ReturnFromHeldItem();
	
	//アイテムから武器Actorを生成して背中に取り付ける。サーバー専用。
	AWeaponBase* GiveWeaponForItem(UEquipmentDataAsset* Item);
	
	//武器Actorを仕舞う。サーバー専用。
	void RemoveWeaponActor(AWeaponBase* Weapon);
	
	//その武器を持つ。既に持っていれば仕舞う。別のものを持っていれば持ち替える。
	void ToggleWeaponInstance(AWeaponBase* Weapon);
	
protected:
	UPROPERTY(BlueprintReadOnly,Category= "Weapon")
	ANexusCharacterBase* OwningCharacter;
	
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category= "Weapon")
	TArray<TSubclassOf<AWeaponBase>> StartingWeapons;
	
	UPROPERTY(BlueprintReadOnly,Category= "Weapon")
	TArray<AWeaponBase*> StowedWeapons;
	
	UPROPERTY(BlueprintReadWrite,Category= "Weapon")
	bool IsWieldingWeapon;
	
	UPROPERTY(BlueprintReadOnly,Replicated,Category= "Weapon")
	AWeaponBase* PreviouslyEquippedWeapon;
	
	UPROPERTY(EditAnywhere,Category="Weapon")
	FSWeaponConfig UnarmedWeaponConfig;
	
	// 消耗品を持つ直前に持っていた武器のクラス。nullptrなら素手。
	UPROPERTY(BlueprintReadOnly,Category= "Weapon")
	TSubclassOf<AWeaponBase> WeaponBeforeHeld;
	
	FActiveGameplayEffectHandle WieldPenaltyHandle;
protected:

	virtual void BeginPlay() override;
	
	// EquippedWeaponがレプリケートされた時に呼ばれる関数
	UFUNCTION()
	void OnRep_EquippedWeapon(AWeaponBase* OldWeapon);

	// 武器を付与する関数
	UFUNCTION(BlueprintCallable, Category= "Weapon")
	void GiveWeapon(TSubclassOf<AWeaponBase> WeaponClass);
	
	//取り出す武器が背中にある武器と等しいか調べる関数。
	UFUNCTION(BlueprintCallable, Category= "Weapon")
	AWeaponBase* GetStowedWeaponByClass(TSubclassOf<AWeaponBase> WeaponClass);
	
	// 装備武器のプロパティの設定関数。
	UFUNCTION(BlueprintCallable, Category= "Weapon")
	void SetEquippedWeaponProperties();
	
	// 解除武器のプロパティの設定関数。
	UFUNCTION(BlueprintCallable, Category= "Weapon")
	void SetUnarmedWeaponConfig();
	
	//BPでEquipMontageを再生する関数
	UFUNCTION(BlueprintImplementableEvent,Category="Weapon")
	void PlayEquipMontage(FSWeaponConfig WeaponConfig);
	
	//BPでUnequipMontageを再生する関数
	UFUNCTION(BlueprintImplementableEvent,Category="Weapon")
	void PlayUnequipMontage(FSWeaponConfig WeaponConfig);
	
	//カメラ設定をUnarmedに戻す関数
	UFUNCTION(BlueprintCallable,Category= "Weapon")
	void SetUnarmedProperties();
	
	//武器を背中に戻す関数。Montageには頼らない。
	void StowWeaponImmediately(AWeaponBase* Weapon);
	
	// 今持っている物に合わせて、ペナルティを付け替える。サーバー専用。
	void RefreshWieldPenalty();
	
};

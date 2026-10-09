// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "CombatAttributeSet.generated.h"

/**
 * 
 */
UCLASS()
class GAS_API UCombatAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
	
public:
	UCombatAttributeSet();
	
	//Armor Attributes
	UPROPERTY(BlueprintReadOnly, Category = "Armor",ReplicatedUsing=OnRep_Armor)
	FGameplayAttributeData Armor;
	ATTRIBUTE_ACCESSORS_BASIC(UCombatAttributeSet, Armor);//ATTRIBUTE_ACCESSORS_BASIC();<<<各属性に対して、手動でゲッター・セッター関数を書かなくても自動で生成してくれる便利なマクロ
	
	
	UPROPERTY(BlueprintReadOnly, Category = "MaxArmor",ReplicatedUsing=OnRep_MaxArmor)
	FGameplayAttributeData MaxArmor;
	ATTRIBUTE_ACCESSORS_BASIC(UCombatAttributeSet, MaxArmor);

	// Physical Power (物理攻撃力)
	UPROPERTY(BlueprintReadOnly, Category = "PhysicalPower",ReplicatedUsing=OnRep_PhysicalPower)
	FGameplayAttributeData PhysicalPower;
	ATTRIBUTE_ACCESSORS_BASIC(UCombatAttributeSet, PhysicalPower);

	// Magical Power (魔法攻撃力)
	UPROPERTY(BlueprintReadOnly, Category = "MagicalPower",ReplicatedUsing=OnRep_MagicalPower)
	FGameplayAttributeData MagicalPower;
	ATTRIBUTE_ACCESSORS_BASIC(UCombatAttributeSet, MagicalPower);
	
	//AdditionalMagicalDamage(追加魔法ダメージ)
	UPROPERTY(BlueprintReadOnly, Category = "AdditionalMagicalDamage",ReplicatedUsing=OnRep_AdditionalMagicalDamage)
	FGameplayAttributeData AdditionalMagicalDamage;
	ATTRIBUTE_ACCESSORS_BASIC(UCombatAttributeSet, AdditionalMagicalDamage);
	
	//AdditionalPhysicalDamage(追加物理ダメージ)
	UPROPERTY(BlueprintReadOnly, Category = "AdditionalPhysicalDamage",ReplicatedUsing=OnRep_AdditionalPhysicalDamage)
	FGameplayAttributeData AdditionalPhysicalDamage;
	ATTRIBUTE_ACCESSORS_BASIC(UCombatAttributeSet, AdditionalPhysicalDamage);
	
protected:
	UFUNCTION()
	void OnRep_Armor(const FGameplayAttributeData& OldValue) const
	{
		GAMEPLAYATTRIBUTE_REPNOTIFY(UCombatAttributeSet, Armor, OldValue);
		// ↑ Armor が変更されたことをゲーム全体に通知
	}
	
	UFUNCTION()
	void OnRep_MaxArmor(const FGameplayAttributeData& OldValue) const
	{
		GAMEPLAYATTRIBUTE_REPNOTIFY(UCombatAttributeSet, MaxArmor, OldValue);
		// ↑ MaxArmor が変更されたことをゲーム全体に通知
	}

	UFUNCTION()
	void OnRep_PhysicalPower(const FGameplayAttributeData& OldValue) const
	{
		GAMEPLAYATTRIBUTE_REPNOTIFY(UCombatAttributeSet, PhysicalPower, OldValue);
	}

	UFUNCTION()
	void OnRep_MagicalPower(const FGameplayAttributeData& OldValue) const
	{
		GAMEPLAYATTRIBUTE_REPNOTIFY(UCombatAttributeSet, MagicalPower, OldValue);
	}
	
	UFUNCTION()
	void OnRep_AdditionalMagicalDamage(const FGameplayAttributeData& OldValue) const
	{
		GAMEPLAYATTRIBUTE_REPNOTIFY(UCombatAttributeSet, AdditionalMagicalDamage, OldValue);
	}
	
	UFUNCTION()
	void OnRep_AdditionalPhysicalDamage(const FGameplayAttributeData& OldValue) const
	{
		GAMEPLAYATTRIBUTE_REPNOTIFY(UCombatAttributeSet, AdditionalPhysicalDamage, OldValue);
	}
	
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	// ↑ 「このクラスの 属性をネットワーク同期してね」とエンジンに指示する関数
	
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	//↑ 属性が変更される前に呼ばれる関数。属性の値を制限するために使用される。
	
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	// ↑ ゲームプレイエフェクトが実行された後に呼ばれる関数。属性の値を制限するために使用される。
};

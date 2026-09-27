// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "BasicAttributeSet.generated.h"

/**
 * 
 */

UCLASS()
class GAS_API UBasicAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
	
public:	 // <<<ここ以降のメンバーは外部からアクセス可能
	
	UBasicAttributeSet();
	
	//Health Attributes
	UPROPERTY(BlueprintReadOnly, Category = "Health",ReplicatedUsing=OnRep_Health)
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS_BASIC(UBasicAttributeSet, Health);
	//ATTRIBUTE_ACCESSORS_BASIC();<<<各属性に対して、手動でゲッター・セッター関数を書かなくても自動で生成してくれる便利なマクロ
	
	UPROPERTY(BlueprintReadOnly, Category = "MaxHealth",ReplicatedUsing=OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS_BASIC(UBasicAttributeSet, MaxHealth);

	//Stamina Attributes
	UPROPERTY(BlueprintReadOnly, Category = "Stamina",ReplicatedUsing=OnRep_Stamina)
	FGameplayAttributeData Stamina;
	ATTRIBUTE_ACCESSORS_BASIC(UBasicAttributeSet, Stamina);

	UPROPERTY(BlueprintReadOnly, Category = "MaxStamina",ReplicatedUsing=OnRep_MaxStamina)
	FGameplayAttributeData MaxStamina;
	ATTRIBUTE_ACCESSORS_BASIC(UBasicAttributeSet, MaxStamina);
	
	//ダメージ属性（複製されない）
	UPROPERTY(BlueprintReadOnly, Category = "Attribute")
	FGameplayAttributeData Damage;
	ATTRIBUTE_ACCESSORS_BASIC(UBasicAttributeSet, Damage);
	
	//Shield Attributes
	UPROPERTY(BlueprintReadOnly, Category = "Shield",ReplicatedUsing=OnRep_Shield)
	FGameplayAttributeData Shield;
	ATTRIBUTE_ACCESSORS_BASIC(UBasicAttributeSet, Shield);

	UPROPERTY(BlueprintReadOnly, Category = "MaxShield",ReplicatedUsing=OnRep_MaxShield)
	FGameplayAttributeData MaxShield;
	ATTRIBUTE_ACCESSORS_BASIC(UBasicAttributeSet, MaxShield);
	
	// Movement Attributes
	UPROPERTY(BlueprintReadOnly, Category = "Movement",ReplicatedUsing=OnRep_MoveSpeed)
	FGameplayAttributeData MoveSpeed;
	ATTRIBUTE_ACCESSORS_BASIC(UBasicAttributeSet, MoveSpeed);
	
	UPROPERTY(BlueprintReadOnly, Category = "Movement",ReplicatedUsing=OnRep_MoveSpeed)
	FGameplayAttributeData MaxMoveSpeed;
	ATTRIBUTE_ACCESSORS_BASIC(UBasicAttributeSet, MaxMoveSpeed);
	
	//派生ステータス：知識から算出される、魔法の最大記憶容量
	UPROPERTY(BlueprintReadOnly,Category= "Derived",ReplicatedUsing=OnRep_MaxMemoryCapacity);
	FGameplayAttributeData MaxMemoryCapacity;
	ATTRIBUTE_ACCESSORS_BASIC(UBasicAttributeSet, MaxMemoryCapacity);
	
public:
	// ネットワークで同期時に呼ばれる
	
	// Health が変更されたことをゲーム全体に通知
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue) const
	{
		GAMEPLAYATTRIBUTE_REPNOTIFY(UBasicAttributeSet, Health, OldValue);
	}
	
	// MaxHealth が変更されたことをゲーム全体に通知
	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue) const
	{
		GAMEPLAYATTRIBUTE_REPNOTIFY(UBasicAttributeSet, MaxHealth, OldValue);
	}

	// Stamina が変更されたことをゲーム全体に通知
	UFUNCTION()
	void OnRep_Stamina(const FGameplayAttributeData& OldValue) const
	{
		GAMEPLAYATTRIBUTE_REPNOTIFY(UBasicAttributeSet, Stamina, OldValue);
	}

	// MaxStamina が変更されたことをゲーム全体に通知
	UFUNCTION()
	void OnRep_MaxStamina(const FGameplayAttributeData& OldValue) const
	{
		GAMEPLAYATTRIBUTE_REPNOTIFY(UBasicAttributeSet, MaxStamina, OldValue);
	}
	
	// Shield が変更されたことをゲーム全体に通知
	UFUNCTION()
	void OnRep_Shield(const FGameplayAttributeData& OldValue) const
	{
		GAMEPLAYATTRIBUTE_REPNOTIFY(UBasicAttributeSet, Shield, OldValue);
	}
	
	// MaxShield が変更されたことをゲーム全体に通知
	UFUNCTION()
	void OnRep_MaxShield(const FGameplayAttributeData& OldValue) const
	{
		GAMEPLAYATTRIBUTE_REPNOTIFY(UBasicAttributeSet, MaxShield, OldValue);
	}
	
	// MoveSpeed が変更されたことをゲーム全体に通知
	UFUNCTION()
	void OnRep_MoveSpeed(const FGameplayAttributeData& OldValue) const
	{
		GAMEPLAYATTRIBUTE_REPNOTIFY(UBasicAttributeSet, MoveSpeed, OldValue);
	}
	
	// MaxMoveSpeed が変更されたことをゲーム全体に通知
	UFUNCTION()
	void OnRep_MaxMoveSpeed(const FGameplayAttributeData& OldValue) const
	{
		GAMEPLAYATTRIBUTE_REPNOTIFY(UBasicAttributeSet, MaxMoveSpeed, OldValue);
	}
	
	//最大記憶容量が変更されたことをゲーム全体に通知
	UFUNCTION()
	void OnRep_MaxMemoryCapacity(const FGameplayAttributeData& OldValue) const
	{
		GAMEPLAYATTRIBUTE_REPNOTIFY(UBasicAttributeSet,MaxMemoryCapacity,OldValue);
	}
	
	// 「このクラスの Health、MaxHealth、Stamina、MaxStamina をネットワーク同期してね」とエンジンに指示する関数
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	// 属性が変更される前に呼ばれる関数。属性の値を制限するために使用される。
	virtual auto PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) -> void override;
	
	// ゲームプレイエフェクトが実行された後に呼ばれる関数。属性の値を制限するために使用される。
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	
	
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
};

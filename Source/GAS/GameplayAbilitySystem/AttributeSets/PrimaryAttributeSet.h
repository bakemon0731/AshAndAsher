// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "PrimaryAttributeSet.generated.h"

/**
 * ステータス用Attribute
 */
UCLASS()
class GAS_API UPrimaryAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
	
public:
	UPrimaryAttributeSet();
	
	// 筋力：物理ダメージの基礎
	UPROPERTY(BlueprintReadOnly,Category= "Primary",ReplicatedUsing=OnRep_Strength)
	FGameplayAttributeData Strength;
	ATTRIBUTE_ACCESSORS_BASIC(UPrimaryAttributeSet, Strength);
	
	// 知識：魔法の記憶容量
	UPROPERTY(BlueprintReadOnly,Category= "Primary",ReplicatedUsing=OnRep_Knowledge)
	FGameplayAttributeData Knowledge;
	ATTRIBUTE_ACCESSORS_BASIC(UPrimaryAttributeSet, Knowledge);
	
	// 意思：魔法ダメージ
	UPROPERTY(BlueprintReadOnly,Category= "Primary",ReplicatedUsing=OnRep_Willpower)
	FGameplayAttributeData Willpower;
	ATTRIBUTE_ACCESSORS_BASIC(UPrimaryAttributeSet, Willpower);
	
	// 敏捷：移動速度
	UPROPERTY(BlueprintReadOnly,Category= "Primary",ReplicatedUsing=OnRep_Agility)
	FGameplayAttributeData Agility;
	ATTRIBUTE_ACCESSORS_BASIC(UPrimaryAttributeSet, Agility);
	
	// 体力：最大HP
	UPROPERTY(BlueprintReadOnly,Category= "Primary",ReplicatedUsing=OnRep_Vitality);
	FGameplayAttributeData Vitality;
	ATTRIBUTE_ACCESSORS_BASIC(UPrimaryAttributeSet, Vitality);

public:
	
	//筋力が変更されたことをゲーム全体に通知
	UFUNCTION()
	void OnRep_Strength(const FGameplayAttributeData& OldValue) const
	{
		GAMEPLAYATTRIBUTE_REPNOTIFY(UPrimaryAttributeSet,Strength,OldValue);
	}
	
	//知識が変更されたことをゲーム全体に通知
	UFUNCTION()
	void OnRep_Knowledge(const FGameplayAttributeData& OldValue) const
	{
		GAMEPLAYATTRIBUTE_REPNOTIFY(UPrimaryAttributeSet,Knowledge,OldValue);
	}
	
	//意思が変更されたことをゲーム全体に通知
	UFUNCTION()
	void OnRep_Willpower(const FGameplayAttributeData& OldValue) const
	{
		GAMEPLAYATTRIBUTE_REPNOTIFY(UPrimaryAttributeSet,Willpower,OldValue);
	}
	
	//敏捷が変更されたことをゲーム全体に通知
	UFUNCTION()
	void OnRep_Agility(const FGameplayAttributeData& OldValue) const
	{
		GAMEPLAYATTRIBUTE_REPNOTIFY(UPrimaryAttributeSet,Agility,OldValue);
	}
	
	//敏捷が変更されたことをゲーム全体に通知
	UFUNCTION()
	void OnRep_Vitality(const FGameplayAttributeData& OldValue) const
	{
		GAMEPLAYATTRIBUTE_REPNOTIFY(UPrimaryAttributeSet,Vitality,OldValue);
	}
	
	//変数の同期
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override; 
};

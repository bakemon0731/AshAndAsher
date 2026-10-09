// Fill out your copyright notice in the Description page of Project Settings.


#include "AC_StatComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GAS/GameplayAbilitySystem/AttributeSets/PrimaryAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GAS/StatSystem/DataAsset/CharacterClassDataAsset.h"


// Sets default values for this component's properties
UAC_StatComponent::UAC_StatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	
	// AvailableStatPointsをネットワーク同期するために、コンポーネント自体の同期をオンにする
	SetIsReplicatedByDefault(true);
}

void UAC_StatComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	// AvailableStatPoints変数をネットワーク同期（レプリケート）対象として登録する
	DOREPLIFETIME(UAC_StatComponent,AvailableStatsPoints);
}

// Called when the game starts
void UAC_StatComponent::BeginPlay()
{
	Super::BeginPlay();
	
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	if (!ASC)
	{
		return;
	}
}

void UAC_StatComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                      FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
}

void UAC_StatComponent::OnRep_AvailableStatsPoints()
{
	// サーバーからポイントの更新を受け取った際、UI等に変更を通知する
	OnStatPointsChanged.Broadcast();
}

void UAC_StatComponent::RequestAllocateStat(FGameplayTag StatTag)
{
	// クライアントの操作をサーバーに送信して、実際の処理をサーバーに依頼する
	Server_AllocateStat(StatTag);
}

void UAC_StatComponent::InitializeStats()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	if (!ASC)
	{
		return;
	}
	
	if (CharacterClass)
	{
		const FPrimaryStatValues& S = CharacterClass->BaseStats;
		ASC->SetNumericAttributeBase(UPrimaryAttributeSet::GetStrengthAttribute(),  S.Strength);
		ASC->SetNumericAttributeBase(UPrimaryAttributeSet::GetKnowledgeAttribute(), S.Knowledge);
		ASC->SetNumericAttributeBase(UPrimaryAttributeSet::GetWillpowerAttribute(), S.Willpower);
		ASC->SetNumericAttributeBase(UPrimaryAttributeSet::GetAgilityAttribute(),   S.Agility);
		ASC->SetNumericAttributeBase(UPrimaryAttributeSet::GetVitalityAttribute(),  S.Vigor);
	}
	
	//派生ステータス用のInfinite Effectを適用
	for (TSubclassOf<UGameplayEffect> EffectClass : DerivedStatEffects)
	{
		if (!EffectClass)
		{
			continue;
		}
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(EffectClass, 1.f, Context);
		if (Spec.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}
}

// サーバー側で実行される割り振りのメイン処理
void UAC_StatComponent::Server_AllocateStat_Implementation(FGameplayTag StatTag)
{
	//ポイントが残っているかチェック（不正や連打対策）
	if (AvailableStatsPoints <= 0)
	{
		return;
	}
	
	//渡されたタグに対応するGameplayEffect（GE）が登録されているか探す
	TSubclassOf<UGameplayEffect>* FoundEffect = StatAllocationEffect.Find(StatTag);
	
	//GetOwnerからAbilitySystemComponent（ASC）を取得
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	
	// 必要なものが揃っていなければ処理を中断
	if (!FoundEffect || !*FoundEffect || !ASC)
	{
		return;
	}
	
	//GameplayEffectContextの作成
	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(*FoundEffect, 1.0f, Context);
	
	if (GetOwner()->HasAuthority())
	{
		if (Spec.IsValid())
		{
			//SetByCallerの設定
			Spec.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.StatPoint")),1.0f);
		
			//自身にGameplayEffectを適用（ここで実際のステータス値が上昇する）
			ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}
	
	//使用したステータスポイントを1減らす
	AvailableStatsPoints -= 1;
	
	//サーバー側（ホストプレイヤー等）のUIも更新させるためにデリゲートを手動で呼び出す
	OnRep_AvailableStatsPoints();
}



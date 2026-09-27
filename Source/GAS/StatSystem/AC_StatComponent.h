// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "GameplayEffect.h"
#include "AC_StatComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStatPointsChanged);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GAS_API UAC_StatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UAC_StatComponent();
	
	// UIでバインドし、ポイント増減時に画面を更新するためのイベント
	UPROPERTY(BlueprintAssignable, Category = "Stats")
	FOnStatPointsChanged OnStatPointsChanged;
	
	//DeriveEffectを初期化時に一度だけ適用。
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "Stats")
	TArray<TSubclassOf<UGameplayEffect>> DerivedStatEffects;
	
	// コンポーネントのレプリケーション設定
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
protected:
	
	// 未使用のステータスポイント。サーバーからクライアントへ同期される
	UPROPERTY(EditAnywhere,BlueprintReadWrite,ReplicatedUsing=OnRep_AvailableStatsPoints,Category="Stats" )
	int32 AvailableStatsPoints = 0;
	
	// どのタグに対して、どのGameplayEffectを適用するかを定義する
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "Stats")
	TMap<FGameplayTag,TSubclassOf<UGameplayEffect>> StatAllocationEffect;
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	
	//実際の割り振り処理を行うサーバー側の関数（チート防止のため計算はサーバーで行う）
	UFUNCTION(Server, Reliable)
	void Server_AllocateStat(FGameplayTag StatTag);
	
	// AvailableStatPointsがクライアントで更新された際に自動で呼ばれる関数
	UFUNCTION()
	void OnRep_AvailableStatsPoints();

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
	
	//現在の残りステータスポイントを取得する
	UFUNCTION(BlueprintPure, Category = "Stats")
	int32 GetAvailableStatPoints() const { return AvailableStatsPoints; }
	
	//Data.Stat.〇〇タグを送る。
	UFUNCTION(BlueprintCallable, Category = "Stats")
	void RequestAllocateStat(FGameplayTag StatTag);
	
};

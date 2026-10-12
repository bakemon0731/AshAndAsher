// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GAS/Interface/Damageable.h"
#include "GAS/Interface/Interactable.h"
#include "NexusCharacterBase.generated.h"

class UAnimMontage;
class USphereComponent;

UCLASS()
class GAS_API ANexusCharacterBase : public ACharacter, public IAbilitySystemInterface, public IDamageable ,public IInteractable
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	
	ANexusCharacterBase();//キャラクターの初期化
	
	// AbilitySystemComponent<<<ゲーム中にアビリティを使うためのコンポーネント
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability System")
	UAbilitySystemComponent* AbilitySystemComponent;
	
	// キャラクターに付与される基本属性セットを管理するための変数
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability System")
	class UBasicAttributeSet* BasicAttributeSet;
	
	// キャラクターに付与される戦闘用の属性セット（Armor、Strengthなど）を管理するための変数
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability System")
	class UCombatAttributeSet* CombatAttributeSet;
	
	// キャラクターに付与されるステータス属性セットを管理するための変数
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability System")
	class UPrimaryAttributeSet* PrimaryAttributeSet;
	
	//キャラクターにAC_SpellComponentを追加。
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spells")
	TObjectPtr<class UAC_SpellComponent> SpellManagerComponent;
	
	//キャラクターにAC_StatComponentを追加
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	TObjectPtr<class UAC_StatComponent> StatManagerComponent;
	
	//キャラクターにAC_EquipmentComponentを追加
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<class UAC_EquipmentComponent> EquipmentManagerComponent;
	
	// MoveSpeedレーティング300(=100%)のときの実際の走行速度(cm/s)。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float BaseRunSpeed = 500.f;
	
	// 死体を漁れる範囲（敵・プレイヤー共通）
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly, Category = "Loot")
	TObjectPtr<USphereComponent> InteractionSphere;
	
	// 漁る時に死体の装備スロットも見せるか（プレイヤー=true、モンスター=false）
	UPROPERTY(EditAnywhere,BlueprintReadOnly, Category = "Loot")
	bool LootShowsEquipment = false;
	
	// インターフェース関数のオーバーライド宣言
	virtual int32 GetTeamNumber() const override;
	
	virtual UAnimMontage* GetHitReactionMontage_Implementation() const override;

protected://マルチプレイヤーでのデータ同期に必要
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability System")
	EGameplayEffectReplicationMode AscReplicationMode = EGameplayEffectReplicationMode::Mixed;
	
	// キャラクターがゲーム開始時に持つべきアビリティのリストを保存する変数
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability System")
	TArray<TSubclassOf<UGameplayAbility>> StartingAbilities;
	
	//魔法選択メニューのIndexと対応させる配列（順番が重要）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability System|Spells")
	TArray<TSubclassOf<UGameplayAbility>> SpellAbilities;
	
	//魔法選択メニューのIndexと対応させるハンドル保持用配列
	UPROPERTY(BlueprintReadOnly, Category = "Ability System|Spells")
	TArray<FGameplayAbilitySpecHandle> SpellAbilityHandles;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TeamNumber")
	int32 TeamNumber;
	
	// 詳細パネルでキャラクターごとにモンタージュを設定できる変数
	UPROPERTY(EditAnywhere,BlueprintReadOnly, Category = "HitReaction")
	TObjectPtr<UAnimMontage> HitReactionMontage;
	
	//体を曲げるPith量を保存する変数
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pitch")
	float Pitch;
	
	UPROPERTY(BlueprintReadOnly, Category = "Loot")
	bool IsDead = false;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void PossessedBy(AController* NewController) override;
	
	virtual void OnRep_PlayerState() override;
	
	// 死亡タグが変化した時に呼ばれる関数（デフォルトでは何もしない）
	virtual void OnDeathTagChanged(const FGameplayTag CallbackTag, int32 NewCount);
	
	// 死亡時の処理を実装する関数
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Damage")
	void HandleDeath();
	
	UFUNCTION()
	void OnLootSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnLootSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	
	
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual class UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	// アビリティを付与する関数
	UFUNCTION(BlueprintCallable, Category = "AbilitySystem")
	TArray<FGameplayAbilitySpecHandle>GrantAbilities(TArray<TSubclassOf<UGameplayAbility>> AbilitiesToGrant,int32 Level = 1);
	
	// アビリティを削除する関数
	UFUNCTION(BlueprintCallable, Category = "AbilitySystem")
	void RemoveAbilities(TArray<FGameplayAbilitySpecHandle> AbilityHandlesToRemove);

	// アビリティの変更を知らせるゲームプレイイベントを送る関数
	UFUNCTION(BlueprintCallable, Category = "AbilitySystem")
	void SendAbilitiesChangedEvent();
	
	// サーバーRPC呼び出し関数　＝　クライアントからサーバーにイベントを送信する関数
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "AbilitySystem")
	void ServerSendGameplayEventToSelf(FGameplayEventData EventData );
	
	// マルチキャストRPC呼び出し関数　＝　サーバーから全クライアントにイベントを送信する関数
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "AbilitySystem")
	void MultiSendGameplayEventToSelf(AActor*TargetActor, FGameplayEventData EventData);
	
	// Index指定で魔法を発動する関数（ディスパッチャーAbilityから呼ばれる）
	UFUNCTION(BlueprintCallable, Category = "AbilitySystem")
	bool ActivateSpellByIndex(int32 SpellIndex);
	
	//死体かどうかを返す純粋関数
	UFUNCTION(BlueprintPure, Category = "Loot")
	bool IsCorpse() const { return IsDead; }
	
	//自身が死んだかどうか。（生きている間（true）は自身のインベントリを漁れない等）
	UFUNCTION(BlueprintCallable, Category = "Loot")
	void SetDead(bool NewDead){ IsDead = NewDead; }
	
	// Interactableインターフェースをオーバーライド
	virtual FText GetInteractionText_Implementation() const override;
	virtual FText GetActorDisplayName_Implementation() const override;
	virtual FLinearColor GetDisplayNameColor_Implementation() const override;
	virtual void OnInteract_Implementation(AActor* Interactor) override;
};

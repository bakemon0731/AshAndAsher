// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/GamePlayAbilitySystem/Characters/NexusCharacterBase.h"// 親クラスのヘッダー
#include "GAS/Interface/Interactable.h"
#include "NexusEnemybase.generated.h"

UCLASS()
class GAS_API ANexusEnemybase : public ANexusCharacterBase, public IInteractable
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ANexusEnemybase();
	
	// 詳細パネルで設定できる BehaviorTree 変数
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="AI")
	class UBehaviorTree* BehaviorTree;
	
	// 攻撃範囲と防御範囲を直接持たせる(元BPI_EnemyAI)
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="AI")
	float AttackRadius = 150.0f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="AI")
	float DefendRadius = 300.0f;
	
	//死んだ敵のアイテムを漁れる範囲
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<class USphereComponent> InteractionSphere;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="AI|Loot")
	TObjectPtr<class ULootTableDataAsset> LootTable;
	
	//Interactableインターフェース関数のオーバーライド
	virtual FText GetInteractionText_Implementation() const override;
	virtual FText GetActorDisplayName_Implementation() const override;
	virtual FLinearColor GetDisplayNameColor_Implementation() const override;
	virtual void OnInteract_Implementation(AActor* Interactor) override;
	
public:
	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetDead(bool NewDead) { IsDead = NewDead; }

	
protected:
	//敵の名前
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="AI")
	FText EnemyDisplayName;
	
	//この敵が死んだかどうか。（生きている間（true）は敵のインベントリを漁れない等）
	UPROPERTY(BlueprintReadOnly,Category= "AI")
	bool IsDead = false;
	
	// オーバーラップ用の関数
	UFUNCTION()
	void OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	// オーバーラップ用の関数
	UFUNCTION()
	void OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void OnDeathTagChanged(const FGameplayTag CallbackTag, int32 NewCount) override;
	
	
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};


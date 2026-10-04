// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/GameplayAbilitySystem/Characters/NexusCharacterBase.h"
#include "GAS/Interface/PlayerInerface.h"
#include "GAS/EquipmentSystem/WorldItem/WorldItemActor.h"
#include "NexusPlayerbase.generated.h"

//イベントディスパッチャーの宣言
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCastStartedDelegate,float,ChargeTime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCastCanceledDelegate);

UCLASS()
class GAS_API ANexusPlayerbase : public ANexusCharacterBase, public IPlayerInerface 
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ANexusPlayerbase();
	
	// イベントディスパッチャーの変数
	UPROPERTY(BlueprintAssignable, Category = "Ability State")
	FOnCastStartedDelegate OnCastStarted;
	
	UPROPERTY(BlueprintAssignable, Category = "Ability State")
	FOnCastCanceledDelegate OnCastCanceled;
	
public:
	
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void RegisterNearbyInteractable(AActor* Interactable);

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void UnregisterNearbyInteractable(AActor* Interactable);

	UFUNCTION(BlueprintPure, Category = "Interaction")
	AActor* GetClosestInteractable() const;

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void TryInteract();


protected:
	UPROPERTY()
	TArray<TObjectPtr<AActor>> NearbyInteractables;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	//インターフェース関数の実装
	virtual void NotifyStartCast_Implementation(float ChargeTime) override;
	virtual void NotifyCancelCast_Implementation() override;
};

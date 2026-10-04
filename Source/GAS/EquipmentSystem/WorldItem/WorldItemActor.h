// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GAS/EquipmentSystem/EquipmentDataAsset.h"
#include "GAS/Interface/Interactable.h"
#include "WorldItemActor.generated.h"

UCLASS()
class GAS_API AWorldItemActor : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AWorldItemActor();

	//アイテムの見た目
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category= "Item")
	TObjectPtr<class UStaticMeshComponent> MeshComponent;
	
	//アイテム拾える範囲
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category= "Item")
	TObjectPtr<class USphereComponent> InteractionSphere;
	
	//AC_EquipmentComponentのスポーン時にドロップしたアイテムのデータを格納する変数
	UPROPERTY(ReplicatedUsing = OnRep_ItemData, EditAnywhere,BlueprintReadOnly,Category= "Item")
	TObjectPtr<UEquipmentDataAsset> ItemData;
	
public:
	UFUNCTION()
	void OnRep_ItemData();
	
	//BP側でメッシュアイコンを差し替えなどを行うためのイベント
	UFUNCTION(BlueprintImplementableEvent,Category= "Item")
	void OnItemDataUpdated();
	
	// ハイライト表示切り替え用（BP側でメッシュのアウトラインON/OFF等を実装する）
	UFUNCTION(BlueprintImplementableEvent,Category= "Item")
	void SetHighlighted(bool Highlighted);
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	//Interactableインターフェースの関数をオーバーライドで使用可能にする
	virtual FText GetInteractionText_Implementation() const override;
	virtual FText GetActorDisplayName_Implementation() const override;
	virtual FLinearColor GetDisplayNameColor_Implementation() const override;
	virtual void OnInteract_Implementation(AActor* Interactor) override;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	UFUNCTION()
	void OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};

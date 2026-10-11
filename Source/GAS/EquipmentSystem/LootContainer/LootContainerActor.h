// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GAS/Interface/Interactable.h"
#include "GAS/EquipmentSystem/DataAsset/EquipmentDataAsset.h"
#include "LootContainerActor.generated.h"

class UAC_EquipmentComponent;
class UEquipmentDataAsset;

UCLASS()
class GAS_API ALootContainerActor : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ALootContainerActor();

	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Loot")
	TObjectPtr<class UStaticMeshComponent> MeshComponent;
	
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Loot")
	TObjectPtr<class USphereComponent> InteractionSphere;
	
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Loot")
	TObjectPtr<UAC_EquipmentComponent> EquipmentComponent;
	
	// 中身が残っている間の最大存在時間（秒）
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Loot")
	float MaxLifeSeconds = 600.f;
	
	// サーバー専用：アイテムを投入し、表示名を設定する
	UFUNCTION(Blueprintable, Category="Loot")
	void InitializeContents(TArray<UEquipmentDataAsset*>& Items, const FText& InDisplayName);
	
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	//インターフェースオーバーライド
	virtual FText GetInteractionText_Implementation() const override;
	virtual FText GetActorDisplayName_Implementation() const override;
	virtual FLinearColor GetDisplayNameColor_Implementation() const override;
	virtual void OnInteract_Implementation(AActor* Interactor) override;
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
protected:

	UPROPERTY(Replicated)
	FText DisplayName;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	UFUNCTION()
	void OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
	//　中身が変わるたびに呼ばれる関数（空になったら自動消滅させる）
	UFUNCTION()
	void HandleContentsChanged();
	
};

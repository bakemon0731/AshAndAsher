// Fill out your copyright notice in the Description page of Project Settings.


#include "NexusEnemybase.h"
#include "Components/SphereComponent.h"
#include "GAS/GameplayAbilitySystem/Characters/PlayerCharacter/NexusPlayerbase.h"
#include "GAS/EquipmentSystem/DataAsset/LootTableDataAsset.h"
#include "GAS/EquipmentSystem/EquipmentComponent/AC_EquipmentComponent.h"


// Sets default values
ANexusEnemybase::ANexusEnemybase()
{
	PrimaryActorTick.bCanEverTick = true;
	
	// インタラクション判定用スフィアの作成
	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(RootComponent);
	InteractionSphere->SetSphereRadius(150.f);
	InteractionSphere->SetCollisionProfileName(TEXT("Custom"));
}

FText ANexusEnemybase::GetInteractionText_Implementation() const
{
	return IsDead ? FText::FromString(TEXT("漁る")) : FText::GetEmpty();
}

FText ANexusEnemybase::GetActorDisplayName_Implementation() const
{
	// 敵の名前を返す。
	return EnemyDisplayName;
}

FLinearColor ANexusEnemybase::GetDisplayNameColor_Implementation() const
{
	// 敵の名前色は固定。
	return FLinearColor::White;
}

void ANexusEnemybase::OnInteract_Implementation(AActor* Interactor)
{
	if (!IsDead)
	{
		// 生きている間はインタラクトできない
		return;
	}
	
	if (ANexusPlayerbase* PlayerChar = Cast<ANexusPlayerbase>(Interactor))
	{
		PlayerChar->OpenLootScreen(this);
	}
}

//死亡時IsDead変数をTrueにする
inline void ANexusEnemybase::OnDeathTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	Super::OnDeathTagChanged(CallbackTag, NewCount);
	
	if (NewCount > 0)
	{
		IsDead = true;
		
		// サーバー権限でのみルート抽選・付与を行う（クライアントで重複実行させないため）
		if (HasAuthority() && LootTable)
		{
			if (UAC_EquipmentComponent* EquipComp = FindComponentByClass<UAC_EquipmentComponent>())
			{
				TArray<UEquipmentDataAsset*> Drops = LootTable->RollLoot();
				for (UEquipmentDataAsset* DropItem : Drops)
				{
					EquipComp->AddItemToInventory(DropItem);
				}
			}
		}
	}
}

void ANexusEnemybase::OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 生きている間はインタラクト対象外
	if (!IsDead)
	{
		return;
	}
	
	if (ANexusPlayerbase* Character = Cast<ANexusPlayerbase>(OtherActor))
	{
		Character->RegisterNearbyInteractable(this);
	}
}

void ANexusEnemybase::OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (ANexusPlayerbase* Character = Cast<ANexusPlayerbase>(OtherActor))
	{
		Character->UnregisterNearbyInteractable(this);
	}
}

// Called when the game starts or when spawned
void ANexusEnemybase::BeginPlay()
{
	Super::BeginPlay();
	
	// オーバーラップイベントのバインド
	InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &ANexusEnemybase::OnInteractionSphereBeginOverlap);
	InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &ANexusEnemybase::OnInteractionSphereEndOverlap);
}

// Called every frame
void ANexusEnemybase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void ANexusEnemybase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}



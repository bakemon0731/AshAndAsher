// Fill out your copyright notice in the Description page of Project Settings.


#include "NexusPlayerbase.h"
#include "GameFramework/PlayerState.h"
#include "Components/CapsuleComponent.h"
#include "GAS/Interface/Interactable.h"
#include "GAS/EquipmentSystem/EquipmentComponent/AC_EquipmentComponent.h"
#include "GAS/EquipmentSystem/LootContainer/LootContainerActor.h"



// Sets default values
ANexusPlayerbase::ANexusPlayerbase()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

void ANexusPlayerbase::RegisterNearbyInteractable(AActor* Interactable)
{
	if (Interactable && Interactable->Implements<UInteractable>())
	{
		NearbyInteractables.AddUnique(Interactable);
		
	}
}

void ANexusPlayerbase::UnregisterNearbyInteractable(AActor* Interactable)
{
	if (Interactable)
	{
		NearbyInteractables.Remove(Interactable);
	}
}

AActor* ANexusPlayerbase::GetClosestInteractable() const
{
	AActor* Closest = nullptr;
	float ClosestDistSq = TNumericLimits<float>::Max();
	
	for (AActor* Candidate : NearbyInteractables)
	{
		if (!Candidate) continue;
		float DistSq = FVector::DistSquared(GetActorLocation(), Candidate->GetActorLocation());
		if (DistSq < ClosestDistSq)
		{
			ClosestDistSq = DistSq;
			Closest = Candidate;
		}
	}
	return Closest;
}

void ANexusPlayerbase::TryInteract()
{
	if (AActor* Target = GetClosestInteractable())
	{
		IInteractable::Execute_OnInteract(Target, this);
	}
}


// Called when the game starts or when spawned
void ANexusPlayerbase::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ANexusPlayerbase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void ANexusPlayerbase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

// 詠唱開始通知を受け取ったら、UIにCallする。(詠唱開始。)
void ANexusPlayerbase::NotifyStartCast_Implementation(float ChargeTime)
{
	// このキャラクターがローカルPCのプレイヤーによって操作されている場合のみデリゲートを飛ばす
	if (IsLocallyControlled())
	{
		OnCastStarted.Broadcast(ChargeTime);
	}
}

// 詠唱キャンセル通知を受け取ったら、UIにCallする。
void ANexusPlayerbase::NotifyCancelCast_Implementation()
{
	// 同様にローカル制御されているかチェック
	if (IsLocallyControlled())
	{
		OnCastCanceled.Broadcast();
	}
}

void ANexusPlayerbase::OnDeathTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	Super::OnDeathTagChanged(CallbackTag, NewCount);
	
	//コールバック（サーバーのみで処理）
	if (NewCount > 0 && HasAuthority())
	{
		SpawnDeathContainer();
	}
}

void ANexusPlayerbase::SpawnDeathContainer()
{
	UAC_EquipmentComponent* EquipComp = FindComponentByClass<UAC_EquipmentComponent>();
	if (!EquipComp || !DeathContainerClass)
	{
		return;
	}
	
	// インベントリと装備が空になる
	TArray<UEquipmentDataAsset*> Items;
	EquipComp->ExtractAllItems(Items);
	if (Items.Num() == 0)
	{
		// 何も持っていなければコンテナは作らない
		return;
	}
	
	const FVector SpawnLocation = GetActorLocation() - FVector(0.0f, 0.0f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	
	if (ALootContainerActor* Container = GetWorld()->SpawnActor<ALootContainerActor>(DeathContainerClass, SpawnLocation, GetActorRotation(), Params))
	{
		const FString PlayerName = GetPlayerState() ? GetPlayerState()->GetPlayerName() : FString();
		Container->InitializeContents(Items, FText::FromString(PlayerName + TEXT("の遺体")));
	}
}




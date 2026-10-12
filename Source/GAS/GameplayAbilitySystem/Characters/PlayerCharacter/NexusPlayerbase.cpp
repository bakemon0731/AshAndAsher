// Fill out your copyright notice in the Description page of Project Settings.


#include "NexusPlayerbase.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "GAS/Interface/Interactable.h"
#include "GAS/EquipmentSystem/EquipmentComponent/AC_EquipmentComponent.h"



// Sets default values
ANexusPlayerbase::ANexusPlayerbase()
{
	PrimaryActorTick.bCanEverTick = true;
	
	// プレイヤーの死体は装備スロットも見せる
	LootShowsEquipment = true;
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
		if (!IsValid(Candidate)) continue;
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

FText ANexusPlayerbase::GetActorDisplayName_Implementation() const
{
	return FText::FromString(DeadPlayerName + TEXT("の遺体"));
}

void ANexusPlayerbase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANexusPlayerbase, DeadPlayerName);
}

void ANexusPlayerbase::OnDeathTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	// 名前はPlayerStateが外れる前の死亡時点で確定させる（サーバー）
	if (NewCount > 0 && HasAuthority() && GetPlayerState())
	{
		DeadPlayerName = GetPlayerState()->GetPlayerName();
	}
	Super::OnDeathTagChanged(CallbackTag, NewCount);
}





// Fill out your copyright notice in the Description page of Project Settings.


#include "AC_EquipmentComponent.h"
#include "GAS/EquipmentSystem/WorldItem/WorldItemActor.h"
#include "GAS/GameplayAbilitySystem/Characters/NexusCharacterBase.h"
#include "Net/UnrealNetwork.h"
#include "GAS/Weapon/WeaponBase/WeaponBase.h"
#include "GAS/Weapon/WeaponComponent/AC_WeaponComponent.h"


UAC_EquipmentComponent::UAC_EquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	
	// このコンポーネントをネットワーク経由で同期（レプリケーション）するように設定
	SetIsReplicatedByDefault(true);
	
	UtilitySlots.SetNum(NumUtilitySlots);
	
	WeaponSlots.SetNum(NumWeaponSlots);
	WeaponSlotActors.SetNum(NumWeaponSlots);
}

void UAC_EquipmentComponent::BeginPlay()
{
	Super::BeginPlay();
	
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		for (UEquipmentDataAsset* StartingItem : StartingInventoryItems)
		{
			AddItemToInventory(StartingItem);
		}
	}
}

bool UAC_EquipmentComponent::Internal_RemoveUtilitySlot(int32 SlotIndex, FUtilitySlotEntry& OutEntry)
{
	if (!UtilitySlots.IsValidIndex(SlotIndex) || !UtilitySlots[SlotIndex].Item)
	{
		return false;
	}

	OutEntry = UtilitySlots[SlotIndex];
	UtilitySlots[SlotIndex] = FUtilitySlotEntry();

	if (ActiveUtilitySlot == SlotIndex)
	{
		Internal_SetActiveUtilitySlots(INDEX_NONE);
	}
	OnRep_UtilitySlots();
	return true;
}

void UAC_EquipmentComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                           FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
}

// サーバーからクライアントへ同期するプロパティの登録
void UAC_EquipmentComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	
	DOREPLIFETIME(UAC_EquipmentComponent,InventoryItemsInstance);
	DOREPLIFETIME(UAC_EquipmentComponent, EquippedSlotsReplicated);
	DOREPLIFETIME(UAC_EquipmentComponent, UtilitySlots);
	DOREPLIFETIME(UAC_EquipmentComponent, ActiveUtilitySlot);
	DOREPLIFETIME(UAC_EquipmentComponent, WeaponSlots);
}

void UAC_EquipmentComponent::RebuildEquippedSlotsReplicated()
{
	EquippedSlotsReplicated.Empty();
	for (const auto& Pair : EquippedItems)
	{
		FEquippedSlotInfo Info;
		Info.Slot = Pair.Key;
		Info.Item = Pair.Value.Item;
		EquippedSlotsReplicated.Add(Info);
	}
}

bool UAC_EquipmentComponent::IsValidLootTarget(const UAC_EquipmentComponent* Remote) const
{
	const ANexusCharacterBase* TargetChar = Remote ? Cast<ANexusCharacterBase>(Remote->GetOwner()) : nullptr;
	if (!TargetChar || !GetOwner())
	{
		return false;
	}

	// 死体であり、かつ操作者の近くにあること
	return TargetChar->IsCorpse()
		&& FVector::Dist(TargetChar->GetActorLocation(), GetOwner()->GetActorLocation()) <= 500.f;
}

//GAS（Ability System Component）を紐付ける
void UAC_EquipmentComponent::InitializeEquipmentSystem(UAbilitySystemComponent* InASC)
{
	CachedASC = InASC;
	
	//サーバーのみ：武器コンポーネントの持ち物の変化を監視
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		if (UAC_WeaponComponent* WeaponComp = GetOwner()->FindComponentByClass<UAC_WeaponComponent>())
		{
			WeaponComp->OnWeaponChanged.AddDynamic(this,&UAC_EquipmentComponent::HandleWeaponChanged);
		}
	}
}

// 特定の装備スロットに現在何が装備されているかを返す
UEquipmentDataAsset* UAC_EquipmentComponent::GetEquippedItem(EEquipmentSlot Slot) const
{
	for (const FEquippedSlotInfo& Info : EquippedSlotsReplicated)
	{
		if (Info.Slot == Slot)
		{
			return Info.Item;
		}
	}
	// 何も装備されていない場合はnullptrを返す
	return nullptr;
}

//クライアントからの装備要求を行う（ServerRPCを呼び出す）
void UAC_EquipmentComponent::RequestEquipItem(FGuid InstanceID)
{
	Server_EquipItem(InstanceID);
}

//クライアントからの装備解除要求を行う（ServerRPCを呼び出す）
void UAC_EquipmentComponent::RequestUnequipItem(EEquipmentSlot Slot)
{
	Server_UnequipItem(Slot);
}

//装備時にGameplay Effect(意思、知識)をキャラクターに適用
void UAC_EquipmentComponent::ApplyItemEffect(FEquippedItemEntry& Entry)
{
	if (!CachedASC || !Entry.Item)
	{
		return;
	}
	
	// アイテムに設定されたエフェクト（意思、知識）をループ処理し、適用
	for (TSubclassOf<UGameplayEffect> EffectClass : Entry.Item->GrantedEffects)
	{
		if (!EffectClass)
		{
			continue;
		}
		
		//GameplayEffectContextを作成。
		FGameplayEffectContextHandle Context = CachedASC->MakeEffectContext();
		FGameplayEffectSpecHandle Spec = CachedASC->MakeOutgoingSpec(EffectClass, 1.f, Context);
		
		if (Spec.IsValid())
		{
			// 自身にエフェクトを適用し、解除時に必要となるスペックハンドルを保存。
			FActiveGameplayEffectHandle Handle = CachedASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			Entry.AppliedEffectHandles.Add(Handle);
		}
	}
	
	//　Head/Chest/Hands/Legsのみ、移動速度ペナルティを適用
	const bool IsArmorSlot = 
		Entry.Item->Slot == EEquipmentSlot::Head  ||
		Entry.Item->Slot == EEquipmentSlot::Chest ||
		Entry.Item->Slot == EEquipmentSlot::Hands ||
		Entry.Item->Slot == EEquipmentSlot::Legs;
	
	if (IsArmorSlot && Entry.Item->MoveSpeedPenalty > 0.f && MoveSpeedPenaltyEffect)
	{
		FGameplayEffectContextHandle Context = CachedASC->MakeEffectContext();
		FGameplayEffectSpecHandle Spec = CachedASC->MakeOutgoingSpec(MoveSpeedPenaltyEffect, 1.f, Context);
		if (Spec.IsValid())
		{
			//MoveSpeedPenaltyの数値をマイナス値にして渡す。
			Spec.Data->SetSetByCallerMagnitude(
				FGameplayTag::RequestGameplayTag(FName("Data.WeightPenalty")),
				-Entry.Item->MoveSpeedPenalty
				);
			
			FActiveGameplayEffectHandle Handle = CachedASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			// 既存のRemoveItemEffectsで自動的に外れる
			Entry.AppliedEffectHandles.Add(Handle);
		}
	}
	
}

//装備を外した時に、適用されていたGameplayEffect(知識、意思)をキャラクターから削除
void UAC_EquipmentComponent::RemoveItemEffects(FEquippedItemEntry& Entry)
{
	if (!CachedASC)
	{
		return;
	}
	
	//保存しておいたスペックハンドルを使って、適用中のエフェクトを一つずつ解除する
	for (FActiveGameplayEffectHandle Handle : Entry.AppliedEffectHandles)
	{
		CachedASC->RemoveActiveGameplayEffect(Handle);
	}
	
	// エフェクトの履歴をリセットする
	Entry.AppliedEffectHandles.Empty();
}

bool UAC_EquipmentComponent::FindFreeGridSlot(UEquipmentDataAsset* Item, int32& OutX, int32& OutY) const
{
	if (!Item)
	{
		return false;
	}
	
	for (int32 y = 0; y <= InventoryGridHeight - Item->GridHeight; y++)
	{
		for (int32 x = 0; x <= InventoryGridWidth - Item->GridWidth; x++)
		{
			if (CanPlaceItemAt(Item,x,y,FGuid()))
			{
				OutX = x;
				OutY = y;
				return true;
			}
		}
	}
	return false;
}


//Server側で装備処理を行う
void UAC_EquipmentComponent::Server_EquipItem_Implementation(FGuid InstanceID)
{
	// ドラッグされた「その個体」をインベントリ配列内から探す
	int32 InstanceIndex = INDEX_NONE;
	for (int32 i = 0; i < InventoryItemsInstance.Num(); i++)
	{
		if (InventoryItemsInstance[i].InstanceID == InstanceID)
		{
			InstanceIndex = i;
			break;
		}
	}

	if (InstanceIndex == INDEX_NONE)
	{
		return; // 指定インスタンスがインベントリに存在しない
	}

	UEquipmentDataAsset* Item = InventoryItemsInstance[InstanceIndex].Item;
	if (!Item || Item->Slot == EEquipmentSlot::None)
	{
		return;
	}

	// 既にそのスロットに何か装備していれば、先に外してインベントリへ戻す
	if (FEquippedItemEntry* Existing = EquippedItems.Find(Item->Slot))
	{
		RemoveItemEffects(*Existing);

		if (Existing->Item)
		{
			int32 FreeX, FreeY;
			if (FindFreeGridSlot(Existing->Item, FreeX, FreeY))
			{
				FInventoryItemInstance ReturnedInstance;
				ReturnedInstance.Item = Existing->Item;
				ReturnedInstance.GridX = FreeX;
				ReturnedInstance.GridY = FreeY;
				ReturnedInstance.InstanceID = FGuid::NewGuid();
				InventoryItemsInstance.Add(ReturnedInstance); // 末尾に追加されるだけなのでInstanceIndexはズレない
			}
		}

		EquippedItems.Remove(Item->Slot);
	}

	// ドラッグした「その個体」だけをインベントリから正確に削除
	InventoryItemsInstance.RemoveAt(InstanceIndex);

	// 新しいアイテムをスロットに装備し、エフェクトを適用
	FEquippedItemEntry NewEntry;
	NewEntry.Item = Item;
	ApplyItemEffect(NewEntry);
	EquippedItems.Add(Item->Slot, NewEntry);

	// レプリケート用配列を更新
	RebuildEquippedSlotsReplicated();

	OnRep_Inventory();
	OnRep_EquippedSlots();
}

//Server側で装備解除処理を行う
void UAC_EquipmentComponent::Server_UnequipItem_Implementation(EEquipmentSlot Slot)
{
	
	
	// 対象スロットの装備情報を取得
	FEquippedItemEntry* Entry = EquippedItems.Find(Slot);
	if (!Entry || !Entry->Item)
	{
		// 何も装備されていない場合は無視
		return;
	}
	
	int32 FreeX, FreeY;
	if (!FindFreeGridSlot(Entry->Item, FreeX, FreeY))
	{
		return; // インベントリに空きが無いので装備解除できない
	}
	
	// エフェクトを解除する
	RemoveItemEffects(*Entry);
	
	if (FindFreeGridSlot(Entry->Item, FreeX, FreeY))
	{
		FInventoryItemInstance NewInstance;
		NewInstance.Item = Entry->Item;
		NewInstance.GridX = FreeX;
		NewInstance.GridY = FreeY;
		NewInstance.InstanceID = FGuid::NewGuid();
		InventoryItemsInstance.Add(NewInstance);
	}
	// 空きが無い場合、アイテムは消滅してしまう点に注意（Step末尾の補足を参照）
	
	// 装備スロットから削除する
	EquippedItems.Remove(Slot);
	
	// レプリケート用配列を更新
	RebuildEquippedSlotsReplicated();
	
	// サーバー側のUIを更新し、クライアントへ変更を同期する
	OnRep_Inventory();
	OnRep_EquippedSlots();
}

//インベントリの同期がクライアントで完了した際（またはサーバー自身）に呼ばれる
void UAC_EquipmentComponent::OnRep_Inventory()
{
	// UI等に変更を通知して、インベントリ画面や装備画面の表示を更新させる
	OnEquipmentChanged.Broadcast();
}

void UAC_EquipmentComponent::OnRep_EquippedSlots()
{
	OnEquipmentChanged.Broadcast();
}

bool UAC_EquipmentComponent::CanPlaceItemAt(UEquipmentDataAsset* Item, int32 GridX, int32 GridY,
	FGuid IgnoreInstanceID) const
{
	if (!Item)
	{
		return false;
	}
	
	// 範囲外チェック
	if (GridX < 0 || GridY < 0 ||
		GridX + Item->GridWidth > InventoryGridWidth ||
		GridY + Item->GridHeight > InventoryGridHeight)
	{
		return false;
	}
	
	// 重なりチェック（既存アイテムの占有範囲と衝突しないか）
	for (const FInventoryItemInstance& Existing : InventoryItemsInstance)
	{
		if (Existing.InstanceID == IgnoreInstanceID || !Existing.Item )
		{
			continue;
		}
		
		bool bOverlapX = GridX < (Existing.GridX + Existing.Item->GridWidth) && (GridX + Item->GridWidth) > Existing.GridX;
		bool bOverlapY = GridY < (Existing.GridY + Existing.Item->GridHeight) && (GridY + Item->GridHeight) > Existing.GridY;

		if (bOverlapX && bOverlapY)
		{
			return false;
		}
	}
	return true;
}

void UAC_EquipmentComponent::RequestMoveItemInventory(FGuid InstanceID, int32 NewGridX, int32 NewGridY)
{
	Server_MoveItemInInventory(InstanceID,NewGridX,NewGridY);
}

bool UAC_EquipmentComponent::AddItemToInventory(UEquipmentDataAsset* Item,int32 Count )
{
	if (!Item || Count <= 0)
	{
		return false;
	}
	
	// 入りきらない時に戻す用
	const TArray<FInventoryItemInstance> Backup = InventoryItemsInstance;
	const int32 MaxStack = FMath::Max(1,Item->MaxStackSize);
	int32 Remaining = Count;
	
	//既存の同じアイテムのスタックに空きがあれば、先に埋める
	for (FInventoryItemInstance& Inst : InventoryItemsInstance)
	{
		if (Remaining <= 0)
		{
			break;
		}
		if (Inst.Item == Item && Inst.StackCount < MaxStack)
		{
			const int32 AddNum = FMath::Min(MaxStack - Inst.StackCount,Remaining);
			Inst.StackCount += AddNum;
			Remaining -= AddNum;
		}
	}
	
	//残りは新しいスタックとして、空きマスに置く
	while (Remaining >0)
	{
		int32 FreeX, FreeY;
		if (!FindFreeGridSlot(Item, FreeX, FreeY))
		{
			// 全部は入らないので、何も変えずに失敗にする
			InventoryItemsInstance = Backup;
			return false;
		}
		
		FInventoryItemInstance NewInstance;
		NewInstance.Item = Item;
		NewInstance.GridX = FreeX;
		NewInstance.GridY = FreeY;
		NewInstance.InstanceID = FGuid::NewGuid();
		NewInstance.StackCount = FMath::Min(MaxStack, Remaining);
		Remaining -= NewInstance.StackCount;
		InventoryItemsInstance.Add(NewInstance);
	}
	
	OnRep_Inventory();
	return true;
}

void UAC_EquipmentComponent::RequestDropItem(FGuid InstanceID)
{
	Server_DropItem(InstanceID);	
}

void UAC_EquipmentComponent::RequestPickupItem(class AWorldItemActor* WorldItem)
{
	Server_PickupItem(WorldItem);
}

void UAC_EquipmentComponent::RequestTransferItem(FGuid InstanceID, UAC_EquipmentComponent* TargetComponent)
{
	Server_TransferItem(InstanceID,TargetComponent);
}

void UAC_EquipmentComponent::Server_TransferItem_Implementation(FGuid InstanceID,
	UAC_EquipmentComponent* TargetComponent)
{
	if (!TargetComponent || TargetComponent == this)
	{
		return;
	}
	
	int32 InstanceIndex = INDEX_NONE;
	for (int32 i = 0; i < InventoryItemsInstance.Num(); i++)
	{
		if (InventoryItemsInstance[i].InstanceID == InstanceID)
		{
			InstanceIndex = i;
			break;
		}
	}
	
	if (InstanceIndex == INDEX_NONE)
	{
		return;
	}
	
	UEquipmentDataAsset* Item = InventoryItemsInstance[InstanceIndex].Item;
	if (!Item)
	{
		return;
	}
	
	// 移動先に空きがあるか確認してから実行
	const int32 Count = InventoryItemsInstance[InstanceIndex].StackCount;
	if (!TargetComponent->AddItemToInventory(Item, Count))
	{
		// 移動先が満杯なら何もしない
		return;
	}
	
	InventoryItemsInstance.RemoveAt(InstanceIndex);
	OnRep_Inventory();
}

void UAC_EquipmentComponent::Server_PickupItem_Implementation(class AWorldItemActor* WorldItem)
{
	if (!WorldItem || !WorldItem->ItemData)
	{
		// 全部は入らないので拾わない
		return;
	}
	
	// インベントリが満杯なら拾えない
	if (!AddItemToInventory(WorldItem->ItemData, WorldItem->StackCount))
	{
		
		return;
	}
	
	WorldItem->Destroy();
}

void UAC_EquipmentComponent::Server_DropItem_Implementation(FGuid InstanceID)
{
	int32 InstanceIndex = INDEX_NONE;
	for (int32 i = 0; i < InventoryItemsInstance.Num(); i++)
	{
		if (InventoryItemsInstance[i].InstanceID == InstanceID)
		{
			InstanceIndex = i;
			break;
		}
	}
	
	if (InstanceIndex == INDEX_NONE )
	{
		return;
	}
	
	UEquipmentDataAsset* Item = InventoryItemsInstance[InstanceIndex].Item;
	//削除する前にスタック数（個数）を控えておく
	const int32 DropCount = InventoryItemsInstance[InstanceIndex].StackCount;
	AActor* Owner = GetOwner();
	// 共通クラスではなく、アイテムごとに指定されたクラスを使う
	TSubclassOf<AWorldItemActor> ClassToSpawn = Item ? Item->WorldItemClass : nullptr;
	if (!Item || !Owner || !ClassToSpawn)
	{
		return;
	}
	
	// オーナーの少し前方にスポーン
	FVector SpawnLocation = Owner->GetActorLocation() + Owner->GetActorForwardVector() * 150.0f;
	FTransform SpawnTransform(Owner->GetActorRotation(),SpawnLocation);
	
	// 遅延スポーン（Deferred Spawn）による生成
	AWorldItemActor* SpawnedItem = GetWorld()->SpawnActorDeferred<AWorldItemActor>(ClassToSpawn, SpawnTransform, Owner);
	if (SpawnedItem)
	{
		// BeginPlayより前にデータをセット
		SpawnedItem->ItemData = Item; 
		//ワールドアイテムにスタック数を設定
		SpawnedItem->StackCount = DropCount;
		// ここでBeginPlayが走る
		SpawnedItem->FinishSpawning(SpawnTransform); 
	}
	
	InventoryItemsInstance.RemoveAt(InstanceIndex);
	OnRep_Inventory();
}



void UAC_EquipmentComponent::Server_MoveItemInInventory_Implementation(FGuid InstanceID, int32 NewGridX, int32 NewGridY)
{
	Internal_MoveInstance(InstanceID, NewGridX, NewGridY);
}

void UAC_EquipmentComponent::Internal_MoveInstance(FGuid InstanceID, int32 NewGridX, int32 NewGridY)
{
	const int32 SrcIdx = InventoryItemsInstance.IndexOfByPredicate(
		[&](const FInventoryItemInstance& I) {return I.InstanceID == InstanceID;});
	if (SrcIdx == INDEX_NONE)
	{
		return;
	}
	
	// ドロップしたマスに、同じアイテムのスタックがあれば合流する
	for (int32 i = 0; i < InventoryItemsInstance.Num(); i++)
	{
		if (i == SrcIdx)
		{
			continue;
		}
		
		FInventoryItemInstance& Dst = InventoryItemsInstance[i];
		FInventoryItemInstance& Src = InventoryItemsInstance[SrcIdx];
		
		const bool bCursorOnDst = Dst.Item
			&& NewGridX >= Dst.GridX && NewGridX < Dst.GridX + Dst.Item->GridWidth
			&& NewGridY >= Dst.GridY && NewGridY < Dst.GridY + Dst.Item->GridHeight;

		if (bCursorOnDst && Dst.Item == Src.Item && Dst.StackCount < Dst.Item->MaxStackSize)
		{
			const int32 MoveNum = FMath::Min(Dst.Item->MaxStackSize - Dst.StackCount, Src.StackCount);
			Dst.StackCount += MoveNum;
			Src.StackCount -= MoveNum;
			if (Src.StackCount <= 0)
			{
				InventoryItemsInstance.RemoveAt(SrcIdx);
			}
			OnRep_Inventory();
			return;
		}
	}
	
	for (FInventoryItemInstance& Instance : InventoryItemsInstance)
	{
		if (Instance.InstanceID == InstanceID)
		{
			if (CanPlaceItemAt(Instance.Item, NewGridX, NewGridY, InstanceID))
			{
				Instance.GridX = NewGridX;
				Instance.GridY = NewGridY;
				OnRep_Inventory();
			}
			return;
		}
	}
}

bool UAC_EquipmentComponent::Internal_RemoveInstance(FGuid InstanceID, FInventoryItemInstance& OutRemoved)
{
	for (int32 i = 0; i < InventoryItemsInstance.Num(); i++)
	{
		if (InventoryItemsInstance[i].InstanceID == InstanceID)
		{
			OutRemoved = InventoryItemsInstance[i];
			InventoryItemsInstance.RemoveAt(i);
			OnRep_Inventory();
			return true;
		}
	}
	return false;
}

void UAC_EquipmentComponent::RequestTransferFromRemote(UAC_EquipmentComponent* RemoteComponent, FGuid InstanceID)
{
	Server_TransferFromRemote(RemoteComponent, InstanceID);
}

void UAC_EquipmentComponent::Server_TransferFromRemote_Implementation(UAC_EquipmentComponent* RemoteComponent, FGuid InstanceID)
{
	if (!IsValidLootTarget(RemoteComponent)) { return; }
	
	if (!RemoteComponent || RemoteComponent == this)
	{
		return;
	}

	FInventoryItemInstance Removed;
	if (!RemoteComponent->Internal_RemoveInstance(InstanceID, Removed))
	{
		return;
	}

	if (!AddItemToInventory(Removed.Item,Removed.StackCount))
	{
		// 自分側に空きが無かった場合、相手側に戻す
		RemoteComponent->AddItemToInventory(Removed.Item,Removed.StackCount);
	}
}

void UAC_EquipmentComponent::RequestMoveRemoteItem(UAC_EquipmentComponent* RemoteComponent, FGuid InstanceID, int32 NewGridX, int32 NewGridY)
{
	Server_MoveRemoteItem(RemoteComponent, InstanceID, NewGridX, NewGridY);
}

void UAC_EquipmentComponent::RequestDropRemoteItem(UAC_EquipmentComponent* RemoteComponent, FGuid InstanceID)
{
	Server_DropRemoteItem(RemoteComponent, InstanceID);
}

bool UAC_EquipmentComponent::Internal_RemoveEquippedItem(EEquipmentSlot Slot, UEquipmentDataAsset*& OutItem)
{
	FEquippedItemEntry* Entry = EquippedItems.Find(Slot);
	if (!Entry || !Entry->Item)
	{
		return false;
	}

	RemoveItemEffects(*Entry);
	OutItem = Entry->Item;
	EquippedItems.Remove(Slot);

	RebuildEquippedSlotsReplicated();
	OnRep_EquippedSlots();
	return true;
}

void UAC_EquipmentComponent::RequestTransferEquippedFromRemote(UAC_EquipmentComponent* RemoteComponent,
	EEquipmentSlot Slot)
{
	Server_TransferEquippedFromRemote(RemoteComponent, Slot);
}


// 【Remote操作の起点】プレイヤー自身のコンポーネントから呼ばれる想定（死体側RPCはクライアントから届かないため）
void UAC_EquipmentComponent::Server_TransferEquippedFromRemote_Implementation(UAC_EquipmentComponent* RemoteComponent,
	EEquipmentSlot Slot)
{
	if (!IsValidLootTarget(RemoteComponent)) { return; }
	
	if (!RemoteComponent || RemoteComponent == this || !IsValidLootTarget(RemoteComponent))
	{
		return;
	}

	// 自分のインベントリに空きがある時だけ取り出す（先に確認しないと、装備を外した後に入らず消える）
	UEquipmentDataAsset* Peek = RemoteComponent->GetEquippedItem(Slot);
	int32 FreeX, FreeY;
	if (!Peek || !FindFreeGridSlot(Peek, FreeX, FreeY))
	{
		return;
	}

	UEquipmentDataAsset* Item = nullptr;
	if (RemoteComponent->Internal_RemoveEquippedItem(Slot, Item))
	{
		AddItemToInventory(Item);
	}
}

void UAC_EquipmentComponent::Server_DropRemoteItem_Implementation(UAC_EquipmentComponent* RemoteComponent,FGuid InstanceID)
{
	if (!IsValidLootTarget(RemoteComponent)) { return; }
	
	if (!RemoteComponent)
	{
		return;
	}
	
	// サーバー上で権限を持った状態で、対象のリモートコンポーネントのドロップ処理を直接呼び出す
	RemoteComponent->Server_DropItem_Implementation(InstanceID);
}

void UAC_EquipmentComponent::Server_MoveRemoteItem_Implementation(UAC_EquipmentComponent* RemoteComponent, FGuid InstanceID, int32 NewGridX, int32 NewGridY)
{
	
	if (!IsValidLootTarget(RemoteComponent)) { return; }
	
	if (!RemoteComponent)
	{
		return;
	}
	RemoteComponent->Internal_MoveInstance(InstanceID, NewGridX, NewGridY);
}

void UAC_EquipmentComponent::Internal_SetActiveUtilitySlots(int32 NewSlot)
{
	if (NewSlot == ActiveUtilitySlot)
	{
		return;
	}

	ActiveUtilitySlot = NewSlot;
	OnRep_ActiveUtilitySlots();

	UAC_WeaponComponent* WeaponComp = GetOwner() ? GetOwner()->FindComponentByClass<UAC_WeaponComponent>() : nullptr;
	if (!WeaponComp)
	{
		return;
	}

	// 持つスロットがあり、HeldItemClass が設定されていれば出す。それ以外は元の武器（素手）へ戻す
	TSubclassOf<AWeaponBase> HeldClass = nullptr;
	if (UtilitySlots.IsValidIndex(NewSlot) && UtilitySlots[NewSlot].Item)
	{
		HeldClass = UtilitySlots[NewSlot].Item->HeldItemClass;
	}

	if (HeldClass)
	{
		WeaponComp->EquipHeldItem(HeldClass);
	}
	else
	{
		WeaponComp->ReturnFromHeldItem();
	}
}

void UAC_EquipmentComponent::HandleWeaponChanged(AWeaponBase* NewWeapon)
{
	if (ActiveUtilitySlot == INDEX_NONE)
	{
		return;
	}
	
	// 引数ではなく、今の状態を見て判断する（仕舞う動作の通知が遅れて届いても、誤って解除しない）
	const UAC_WeaponComponent* WeaponComp = GetOwner()->FindComponentByClass<UAC_WeaponComponent>();
	const AWeaponBase* Current = WeaponComp ? WeaponComp->EquippedWeapon : nullptr;
	
	if (!IsValid(Current) || !Current->IsHeldItem)
	{
		// 武器を持ったので、ユーティリティは持っていない状態にする
		ActiveUtilitySlot = INDEX_NONE;
		OnRep_ActiveUtilitySlots();
	}
}


// ===================== ユーティリティスロット =====================

void UAC_EquipmentComponent::OnRep_UtilitySlots()
{
	OnUtilityChanged.Broadcast();
}

void UAC_EquipmentComponent::OnRep_ActiveUtilitySlots()
{
	OnUtilityChanged.Broadcast();
}

FUtilitySlotEntry UAC_EquipmentComponent::GetUtilitySlot(int32 SlotIndex) const
{
	return UtilitySlots.IsValidIndex(SlotIndex) ? UtilitySlots[SlotIndex] : FUtilitySlotEntry();
}

bool UAC_EquipmentComponent::CanSetUtilityItem(UEquipmentDataAsset* Item) const
{
	return Item && Item->Category == EItemCategory::Consumable && Item->UseEffects.Num() > 0;
}

void UAC_EquipmentComponent::RequestEquipUtility(FGuid InstanceID, int32 SlotIndex)
{
	Server_EquipUtility(InstanceID, SlotIndex);
}

void UAC_EquipmentComponent::Server_EquipUtility_Implementation(FGuid InstanceID, int32 SlotIndex)
{
	if (!UtilitySlots.IsValidIndex(SlotIndex))
	{
		return;
	}
	
	const int32 SrcIdx = InventoryItemsInstance.IndexOfByPredicate(
		[&](const FInventoryItemInstance& I) {return I.InstanceID == InstanceID;});
	if (SrcIdx == INDEX_NONE)
	{
		return;
	}
	
	UEquipmentDataAsset* Item = InventoryItemsInstance[SrcIdx].Item;
	if (!CanSetUtilityItem(Item))
	{
		return;
	}
	
	const int32 Count = InventoryItemsInstance[SrcIdx].StackCount;
	FUtilitySlotEntry& Slot = UtilitySlots[SlotIndex];
	
	// 同じアイテムが入っていて既に満杯なら、何もしない
	if (Slot.Item == Item && Slot.StackCount >= Item->MaxStackSize)
	{
		return;
	}
	
	// 失敗時に戻す用
	const TArray<FInventoryItemInstance> Backup = InventoryItemsInstance;
	InventoryItemsInstance.RemoveAt(SrcIdx);
	
	if (Slot.Item == Item)
	{
		//同じアイテムは入る分だけ合流し、残りはインベントリへ戻す
		const int32 Move = FMath::Min(Count,Item->MaxStackSize - Slot.StackCount);
		const int32 Rest = Count - Move;
		if (Rest > 0 && !AddItemToInventory(Item, Rest))
		{
			InventoryItemsInstance = Backup;
			return;
		}
		Slot.StackCount += Move;
	}
	else
	{
		// 別のアイテムが入っていれば、先にインベントリへ戻す（入らなければ全部取り消す）
		if (Slot.Item && !AddItemToInventory(Slot.Item, Slot.StackCount))
		{
			InventoryItemsInstance = Backup;
			return;
		}
		Slot.Item = Item;
		Slot.StackCount = Count;
	}
	OnRep_Inventory();
	OnRep_UtilitySlots();
}

void UAC_EquipmentComponent::RequestUnequipUtility(int32 SlotIndex)
{
	Server_UnequipUtility(SlotIndex);
}

void UAC_EquipmentComponent::Server_UnequipUtility_Implementation(int32 SlotIndex)
{
	if (!UtilitySlots.IsValidIndex(SlotIndex) || !UtilitySlots[SlotIndex].Item)
	{
		return;
	}
	
	FUtilitySlotEntry& Slot = UtilitySlots[SlotIndex];
	if (!AddItemToInventory(Slot.Item, Slot.StackCount))
	{
		// インベントリに空きが無ければ、外せない
		return;
	}
	
	Slot = FUtilitySlotEntry();
	if (ActiveUtilitySlot == SlotIndex)
	{
		Internal_SetActiveUtilitySlots(INDEX_NONE);
	}
	OnRep_UtilitySlots();
}

void UAC_EquipmentComponent::RequestCycleUtilityGroup(int32 GroupIndex)
{
	Server_CycleUtilityGroup(GroupIndex);
}

void UAC_EquipmentComponent::Server_CycleUtilityGroup_Implementation(int32 GroupIndex)
{
	if (GroupIndex < 0 || GroupIndex >= NumUtilitySlots / UtilitySlotsPerGroup)
	{
		return;
	}
	
	const int32 First = GroupIndex * UtilitySlotsPerGroup;
	const int32 Last = First + UtilitySlotsPerGroup - 1;
	const bool InGroup = ActiveUtilitySlot >= First && ActiveUtilitySlot <= Last;
	
	int32 Next = INDEX_NONE;
	
	if (InGroup)
	{
		// すでにグループ内を選択している場合は次のスロットへ進む
		Next = ActiveUtilitySlot + 1;
		if (Next > Last)
		{
			// グループの最後まで行ったら最初のスロットに戻す（ループさせる）
			Next = First;
		}
	}
	else
	{
		// グループ外（武器など）にいる場合は、グループの先頭を選択する
		Next = First;
	}
	
	Internal_SetActiveUtilitySlots(Next);
}

void UAC_EquipmentComponent::RequestClearActiveUtility()
{
	Server_ClearActiveUtility();
}

void UAC_EquipmentComponent::Server_ClearActiveUtility_Implementation()
{
	if (ActiveUtilitySlot != INDEX_NONE)
	{
		Internal_SetActiveUtilitySlots(INDEX_NONE);
	}
}

void UAC_EquipmentComponent::RequestUseActiveUtility()
{
	Server_UseActiveUtility();
}

void UAC_EquipmentComponent::Server_UseActiveUtility_Implementation()
{
	if (!UtilitySlots.IsValidIndex(ActiveUtilitySlot) || !CachedASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("Server_UseActiveUtility: UtilitySlot is invalid or CachedASC is nullptr"));
		return;
	}

	// 【追加】AvatarActorが設定されているか確認
	if (!CachedASC->GetAvatarActor())
	{
		UE_LOG(LogTemp, Error, TEXT("Server_UseActiveUtility: CachedASC->GetAvatarActor() is nullptr! (ASC is not fully initialized)"));
		return;
	}

	const ANexusCharacterBase* OwnerChar = Cast<ANexusCharacterBase>(GetOwner());
	if (OwnerChar && OwnerChar->IsCorpse())
	{
		return;
	}
	
	FUtilitySlotEntry& Slot = UtilitySlots[ActiveUtilitySlot];
	if (!Slot.Item || Slot.StackCount <= 0)
	{
		return;
	}
	
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now < NextUtilityUseTime)
	{
		return;
	}
	NextUtilityUseTime = Now + Slot.Item->UseCooldown;
	
	for (TSubclassOf<UGameplayEffect> EffectClass : Slot.Item->UseEffects)
	{
		if (!EffectClass)
		{
			continue;
		}
		
		FGameplayEffectContextHandle Context = CachedASC->MakeEffectContext();
		FGameplayEffectSpecHandle Spec = CachedASC->MakeOutgoingSpec(EffectClass, 1.f, Context);
		
		if (Spec.IsValid() && Spec.Data.IsValid())
		{
			CachedASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Server_UseActiveUtility: Failed to create valid GameplayEffectSpec for effect: %s"), *GetNameSafe(EffectClass));
		}
	}
	
	Slot.StackCount--;
	if (Slot.StackCount <= 0)
	{
		Slot = FUtilitySlotEntry();
		// 使い切ったら武器に戻る
		Internal_SetActiveUtilitySlots(INDEX_NONE);
	}
	OnRep_UtilitySlots();
}

void UAC_EquipmentComponent::RequestTransferUtilityFromRemote(UAC_EquipmentComponent* RemoteComponent, int32 SlotIndex)
{
	Server_TransferUtilityFromRemote(RemoteComponent, SlotIndex);
}

void UAC_EquipmentComponent::Server_TransferUtilityFromRemote_Implementation(UAC_EquipmentComponent* RemoteComponent,
                                                                             int32 SlotIndex)
{
	if (!RemoteComponent || RemoteComponent == this || !IsValidLootTarget(RemoteComponent))
	{
		return;
	}

	const FUtilitySlotEntry Peek = RemoteComponent->GetUtilitySlot(SlotIndex);
	if (!Peek.Item)
	{
		return;
	}

	// 先に自分へ入れる。AddItemToInventoryは全部入らなければ何も変えずに失敗するので、入らない時は死体側も変わらない
	if (!AddItemToInventory(Peek.Item, Peek.StackCount))
	{
		return;
	}

	FUtilitySlotEntry Removed;
	RemoteComponent->Internal_RemoveUtilitySlot(SlotIndex, Removed);
}

void UAC_EquipmentComponent::RequestSwapUtilitySlots(int32 SlotA, int32 SlotB)
{
	Server_SwapUtilitySlots(SlotA, SlotB);
}

void UAC_EquipmentComponent::Server_SwapUtilitySlots_Implementation(int32 SlotA, int32 SlotB)
{
	if (SlotA == SlotB || !UtilitySlots.IsValidIndex(SlotA) ||  !UtilitySlots.IsValidIndex(SlotB))
	{
		return;
	}
	
	FUtilitySlotEntry& From = UtilitySlots[SlotA];
	FUtilitySlotEntry& To = UtilitySlots[SlotB];
	if (!From.Item)
	{
		// 動かす物が無い
		return;
	}
	
	if (To.Item == From.Item)
	{
		// 同じアイテム：先に空きがあれば、入る分だけ重ねる
		const int32 MaxStack = FMath::Max(1,From.Item->MaxStackSize);
		if (To.StackCount >= MaxStack)
		{
			// 先が満杯なら何もしない
			return;
		}
		
		const int32 Move = FMath::Min(MaxStack - To.StackCount, From.StackCount);
		To.StackCount += Move;
		From.StackCount -= Move;
		
		if (From.StackCount <= 0)
		{
			From = FUtilitySlotEntry();
			if (ActiveUtilitySlot == SlotA)
			{
				// 手に持っていた分が全部移ったので、持ち替え先も動かす
				Internal_SetActiveUtilitySlots(SlotB);
			}
		}
	}
	else
	{
		// 別のアイテム、または空のスロット：中身ごと入れ替える
		UtilitySlots.Swap(SlotA, SlotB);
		if (ActiveUtilitySlot == SlotA)
		{
			Internal_SetActiveUtilitySlots(SlotB);
		}
		else if (ActiveUtilitySlot == SlotB)
		{
			Internal_SetActiveUtilitySlots(SlotA);
		}
	}
	
	OnRep_UtilitySlots();
}

// ===================== 武器スロット =====================

void UAC_EquipmentComponent::OnRep_WeaponSlots()
{
	OnWeaponSlotsChanged.Broadcast();
}

UEquipmentDataAsset* UAC_EquipmentComponent::GetWeaponSlotItem(int32 SlotIndex) const
{
	return WeaponSlots.IsValidIndex(SlotIndex) ? WeaponSlots[SlotIndex].Get() : nullptr;
}

bool UAC_EquipmentComponent::CanSetWeaponItem(UEquipmentDataAsset* Item) const
{
	return Item && Item->Slot == EEquipmentSlot::Weapon && Item->HeldItemClass;
}

void UAC_EquipmentComponent::SpawnWeaponSlotActor(int32 SlotIndex)
{
	UAC_WeaponComponent* WeaponComp = GetOwner() ? GetOwner()->FindComponentByClass<UAC_WeaponComponent>(): nullptr;
	if (WeaponComp && WeaponSlots.IsValidIndex(SlotIndex) && WeaponSlots[SlotIndex])
	{
		//アイテムから武器Actorを生成して背中に取り付ける。
		WeaponSlotActors[SlotIndex] = WeaponComp->GiveWeaponForItem(WeaponSlots[SlotIndex]);
	}
}

void UAC_EquipmentComponent::ClearWeaponSlotActor(int32 SlotIndex)
{
	if (!WeaponSlotActors.IsValidIndex(SlotIndex))
	{
		return;
	}
	
	if (AWeaponBase* Weapon = WeaponSlotActors[SlotIndex])
	{
		if (UAC_WeaponComponent* WeaponComp = GetOwner()->FindComponentByClass<UAC_WeaponComponent>())
		{
			//武器Actorを仕舞う。
			WeaponComp->RemoveWeaponActor(Weapon);
		}
	}
	WeaponSlotActors[SlotIndex] = nullptr;
}

void UAC_EquipmentComponent::RequestEquipWeaponToSlot(FGuid InstanceID, int32 SlotIndex)
{
	Server_EquipWeaponToSlot(InstanceID, SlotIndex);
}

void UAC_EquipmentComponent::Server_EquipWeaponToSlot_Implementation(FGuid InstanceID, int32 SlotIndex)
{
	if (!WeaponSlots.IsValidIndex(SlotIndex))
	{
		return;
	}
	
	const int32 SrcIdx = InventoryItemsInstance.IndexOfByPredicate(
		[&](const FInventoryItemInstance& I) {return I.InstanceID == InstanceID;});
	if (SrcIdx == INDEX_NONE)
	{
		return;
	}
	
	UEquipmentDataAsset* Item = InventoryItemsInstance[SrcIdx].Item;
	if (!CanSetWeaponItem(Item))
	{
		return;
	}
	
	// 失敗時に戻す用
	const TArray<FInventoryItemInstance> Backup = InventoryItemsInstance;
	InventoryItemsInstance.RemoveAt(SrcIdx);
	
	// 既に武器が入っていれば、先にインベントリへ戻す（入らなければ全部取り消す）
	if (UEquipmentDataAsset* Old = WeaponSlots[SlotIndex])
	{
		if (!AddItemToInventory(Old,1))
		{
			InventoryItemsInstance = Backup;
			return;
		}
		ClearWeaponSlotActor(SlotIndex);
	}
	
	WeaponSlots[SlotIndex] = Item;
	SpawnWeaponSlotActor(SlotIndex);
	
	OnRep_Inventory();
	OnRep_WeaponSlots();
}

void UAC_EquipmentComponent::RequestUnequipWeaponSlot(int32 SlotIndex)
{
	Server_UnequipWeaponSlot(SlotIndex);
}

void UAC_EquipmentComponent::Server_UnequipWeaponSlot_Implementation(int32 SlotIndex)
{
	if (!WeaponSlots.IsValidIndex(SlotIndex) || !WeaponSlots[SlotIndex])
	{
		return;
	}
	if (!AddItemToInventory(WeaponSlots[SlotIndex],1))
	{
		// インベントリに空きが無ければ、外せない
		return;
	}
	ClearWeaponSlotActor(SlotIndex);
	WeaponSlots[SlotIndex] = nullptr;
	OnRep_WeaponSlots();
}

void UAC_EquipmentComponent::ToggleWeaponSlot(int32 SlotIndex)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	
	if (!WeaponSlotActors.IsValidIndex(SlotIndex) || !IsValid(WeaponSlotActors[SlotIndex]))
	{
		// その武器スロットが空
		return;
	}
	
	const ANexusCharacterBase* OwnerChar = Cast<ANexusCharacterBase>(GetOwner());
	if (OwnerChar && OwnerChar->IsCorpse())
	{
		return;
	}
	
	if (UAC_WeaponComponent* WeaponComp = GetOwner()->FindComponentByClass<UAC_WeaponComponent>())
	{
		//その武器を持つ。既に持っていれば仕舞う。別のものを持っていれば持ち替える。
		WeaponComp->ToggleWeaponInstance(WeaponSlotActors[SlotIndex]);
	}
}

bool UAC_EquipmentComponent::Internal_removeWeaponSlot(int32 SlotIndex, UEquipmentDataAsset*& OutItem)
{
	if (!WeaponSlots.IsValidIndex(SlotIndex) || !WeaponSlots[SlotIndex])
	{
		return false;
	}
	
	OutItem = WeaponSlots[SlotIndex];
	ClearWeaponSlotActor(SlotIndex);
	WeaponSlots[SlotIndex] = nullptr;
	OnRep_WeaponSlots();
	return true;
}

void UAC_EquipmentComponent::RequestTransferWeaponFromRemote(UAC_EquipmentComponent* RemoteComponent, int32 SlotIndex)
{
	Server_TransferWeaponFromRemote(RemoteComponent,SlotIndex);
}

//プレイヤー自身のコンポーネントから呼ばれる
void UAC_EquipmentComponent::Server_TransferWeaponFromRemote_Implementation(UAC_EquipmentComponent* RemoteComponent,
	int32 SlotIndex)
{
	if (!RemoteComponent || RemoteComponent == this || !IsValidLootTarget(RemoteComponent))
	{
		return;
	}
	
	UEquipmentDataAsset* Peek = RemoteComponent->GetWeaponSlotItem(SlotIndex);
	if (!Peek)
	{
		return;
	}
	
	// 先に自分へ入れる。入らなければ何も変えずに失敗するので、死体側の武器は消えない
	if (!AddItemToInventory(Peek,1))
	{
		return;
	}
	
	UEquipmentDataAsset* Removed = nullptr;
	RemoteComponent->Internal_removeWeaponSlot(SlotIndex,Removed);
}

void UAC_EquipmentComponent::RequestSwapWeaponSlots(int32 SlotA, int32 SlotB)
{
	Server_SwapWeaponSlots(SlotA,SlotB);
}

void UAC_EquipmentComponent::Server_SwapWeaponSlots_Implementation(int32 SlotA, int32 SlotB)
{
	if (SlotA == SlotB || !WeaponSlots.IsValidIndex(SlotA) || !WeaponSlots.IsValidIndex(SlotB))
	{
		return;
	}
	if (!WeaponSlots[SlotA] && !WeaponSlots[SlotB])
	{
		// 両方空
		return;
	}
	
	// 武器のActorは、入れ替えずに、スロットとの対応だけを入れ替える。手に持っている物は変わらない
	WeaponSlots.Swap(SlotA,SlotB);
	WeaponSlotActors.Swap(SlotA,SlotB);
	OnRep_WeaponSlots();
}

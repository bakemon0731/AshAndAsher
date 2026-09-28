// Fill out your copyright notice in the Description page of Project Settings.


#include "AC_EquipmentComponent.h"

#include "IDetailTreeNode.h"
#include "Net/UnrealNetwork.h"



UAC_EquipmentComponent::UAC_EquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	
	// このコンポーネントをネットワーク経由で同期（レプリケーション）するように設定
	SetIsReplicatedByDefault(true);
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

void UAC_EquipmentComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                           FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
}

// サーバーからクライアントへ同期するプロパティの登録
void UAC_EquipmentComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	// InventoryItems 配列を同期対象として登録
	DOREPLIFETIME(UAC_EquipmentComponent,InventoryItemsInstance);
	DOREPLIFETIME(UAC_EquipmentComponent, EquippedSlotsReplicated);
}

//GAS（Ability System Component）を紐付ける
void UAC_EquipmentComponent::InitializeEquipmentSystem(UAbilitySystemComponent* InASC)
{
	CachedASC = InASC;
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
	
	// アイテムに設定されたエフェクト（意思、知識）をループ処理
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

FInventoryItemInstance* UAC_EquipmentComponent::FindInventoryInstanceByItem(UEquipmentDataAsset* Item)
{
	for (FInventoryItemInstance& Instance : InventoryItemsInstance)
	{
		if (Instance.Item == Item)
		{
			return &Instance;
		}
	}
	return nullptr;
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
	EquippedSlotsReplicated.Empty();
	for (const auto& Pair : EquippedItems)
	{
		FEquippedSlotInfo Info;
		Info.Slot = Pair.Key;
		Info.Item = Pair.Value.Item;
		EquippedSlotsReplicated.Add(Info);
	}

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
	EquippedSlotsReplicated.Empty();
	for (const auto& Pair : EquippedItems)
	{
		FEquippedSlotInfo Info;
		Info.Slot = Pair.Key;
		Info.Item = Pair.Value.Item;
		EquippedSlotsReplicated.Add(Info);
	}
	
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

bool UAC_EquipmentComponent::AddItemToInventory(UEquipmentDataAsset* Item)
{
	int32 FreeX, FreeY;
	if (!Item || !FindFreeGridSlot(Item, FreeX, FreeY))
	{
		return false;
	}
	
	FInventoryItemInstance NewInstance;
	NewInstance.Item = Item;
	NewInstance.GridX = FreeX;
	NewInstance.GridY = FreeY;
	NewInstance.InstanceID = FGuid::NewGuid();
	InventoryItemsInstance.Add(NewInstance);

	OnRep_Inventory();
	return true;
}

void UAC_EquipmentComponent::Server_MoveItemInInventory_Implementation(FGuid InstanceID, int32 NewGridX, int32 NewGridY)
{
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



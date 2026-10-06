// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AbilitySystemComponent.h"
#include "GAS/EquipmentSystem/EquipmentDataAsset.h"
#include "AC_EquipmentComponent.generated.h"

/**
 * 装備品とインベントリを管理し、GASと連携するコンポーネント
 */

//イベントディスパッチャーの宣言
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEquipmentChanged);

// 装備中のアイテムとそのアイテムによって付与されたエフェクトを管理する構造体
USTRUCT()
struct FEquippedItemEntry
{
	GENERATED_BODY()
	
	// 装備しているアイテムのデータアセット
	UPROPERTY()
	TObjectPtr<UEquipmentDataAsset> Item = nullptr;
	
	//装備時付与されたEffect（意思や知識）をハンドルで保持
	UPROPERTY()
	TArray<FActiveGameplayEffectHandle> AppliedEffectHandles;
};

//装備状況も配列として持たせ、レプリケートする
USTRUCT(BlueprintType)
struct FEquippedSlotInfo 
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly)
	EEquipmentSlot Slot = EEquipmentSlot::None;
	
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UEquipmentDataAsset> Item = nullptr;
};

USTRUCT(BlueprintType)
struct FInventoryItemInstance
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UEquipmentDataAsset> Item = nullptr;
	
	UPROPERTY(BlueprintReadOnly)
	int32 GridX = 0;
	
	UPROPERTY(BlueprintReadOnly)
	int32 GridY = 0;
	
	// 同じItemを複数持つ場合の識別用（ドラッグ操作でどのインスタンスか特定するため）
	UPROPERTY(BlueprintReadOnly)
	FGuid InstanceID;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GAS_API UAC_EquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	
	UAC_EquipmentComponent();

	//インベントリのグリッドの幅
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Equipment")
	int32 InventoryGridWidth = 10;
	
	//インベントリのグリッドの高さ
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Equipment")
	int32 InventoryGridHeight = 5;
	
	// インベントリ内の未装備アイテム一覧
	UPROPERTY(ReplicatedUsing=OnRep_Inventory,EditAnywhere,BlueprintReadOnly,Category="Equipment")
	TArray<FInventoryItemInstance> InventoryItemsInstance;
	
	//装備状態やインベントリが変更された際に通知
	UPROPERTY(BlueprintAssignable, Category="Equipment")
	FOnEquipmentChanged OnEquipmentChanged;
	
	UPROPERTY(ReplicatedUsing = OnRep_EquippedSlots, BlueprintReadOnly, Category = "Equipment")
	TArray<FEquippedSlotInfo> EquippedSlotsReplicated;

	//初期アイテムを付与する
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Equipment")
	TArray<TObjectPtr<UEquipmentDataAsset>> StartingInventoryItems;

	
public:
	
	// コンポーネント初期化時などに呼び出し、対象キャラクターのAbility System Componentを登録
	UFUNCTION(BlueprintCallable, Category="Equipment")
	void InitializeEquipmentSystem(UAbilitySystemComponent* InASC);
	
	//インベントリのアイテムを指定スロットへ装備要求する(サーバーRPCを呼び出す)
	UFUNCTION(BlueprintCallable, Category="Equipment")
	void RequestEquipItem(FGuid InstanceID);
	
	//指定スロットの装備を外す要求を出し、インベントリへ戻す(サーバーRPCを呼び出す)
	UFUNCTION(BlueprintCallable, Category="Equipment")
	void RequestUnequipItem(EEquipmentSlot Slot);
	
	//指定したスロットに現在装備されているアイテムデータを取得する
	UFUNCTION(BlueprintPure, Category="Equipment")
	UEquipmentDataAsset* GetEquippedItem(EEquipmentSlot Slot) const;
	
	UFUNCTION()
	void OnRep_EquippedSlots();
	
	// 指定座標にItemを配置できるか判定（範囲外・重なりチェック）
	UFUNCTION(BlueprintPure, Category="Equipment")
	bool CanPlaceItemAt(UEquipmentDataAsset* Item,int32 GridX, int32 GridY,FGuid IgnoreInstanceID) const;
	
	//指定インスタンスを指定座標へ移動（インベントリ内の並べ替え）
	UFUNCTION(BlueprintCallable, Category="Equipment")
	void RequestMoveItemInventory(FGuid InstanceID,int32 NewGridX, int32 NewGridY);
	
	UFUNCTION(Server,Reliable)
	void Server_MoveItemInInventory(FGuid InstanceID,int32 NewGridX, int32 NewGridY);
	
	//初期アイテムを付与するヘルパー関数
	UFUNCTION(Blueprintable,Category="Equipment")
	bool AddItemToInventory(UEquipmentDataAsset* Item);

	// インベントリのアイテムをワールドに落とす
	UFUNCTION(BlueprintCallable, Category="Equipment")
	void RequestDropItem(FGuid InstanceID);
	
	UFUNCTION(Server,Reliable)
	void Server_DropItem(FGuid InstanceID);
	
	//ワールドのアイテムを拾う
	UFUNCTION(BlueprintCallable,Category="Equipment")
	void RequestPickupItem(class AWorldItemActor* WorldItem);
	
	UFUNCTION(Server,Reliable)
	void Server_PickupItem(class AWorldItemActor* WorldItem);

	// 自分のインベントリにある指定インスタンスを、別のコンポーネントへ移動する（Playerの場合のみ動作）
	UFUNCTION(BlueprintCallable,Category="Equipment")
	void RequestTransferItem(FGuid InstanceID,UAC_EquipmentComponent* TargetComponent);
	
	UFUNCTION(Server,Reliable)
	void Server_TransferItem(FGuid InstanceID,UAC_EquipmentComponent* TargetComponent);
	
	// サーバー上で直接呼ぶ内部ロジック（権限チェック済み前提、RPCではない）
	void Internal_MoveInstance(FGuid InstanceID, int32 NewGridX, int32 NewGridY);
	bool Internal_RemoveInstance(FGuid InstanceID, FInventoryItemInstance& OutRemoved);
	
	// 遠隔コンポーネント(死体等)のアイテムを「自分(プレイヤー)」に転送する
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void RequestTransferFromRemote(UAC_EquipmentComponent* RemoteComponent, FGuid InstanceID);

	UFUNCTION(Server, Reliable)
	void Server_TransferFromRemote(UAC_EquipmentComponent* RemoteComponent, FGuid InstanceID);

	// 遠隔コンポーネント(死体等)の中で、アイテムを並べ替える
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void RequestMoveRemoteItem(UAC_EquipmentComponent* RemoteComponent, FGuid InstanceID, int32 NewGridX, int32 NewGridY);

	UFUNCTION(Server, Reliable)
	void Server_MoveRemoteItem(UAC_EquipmentComponent* RemoteComponent, FGuid InstanceID, int32 NewGridX, int32 NewGridY);
	
	// 遠隔コンポーネント(死体等)のアイテムをワールドに落とす要求
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void RequestDropRemoteItem(UAC_EquipmentComponent* RemoteComponent, FGuid InstanceID);
	
	UFUNCTION(Server,Reliable)
	void Server_DropRemoteItem(UAC_EquipmentComponent* RemoteComponent, FGuid InstanceID);
	
	
protected:

	// 登録されたAbility System Componentのキャッシュ
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> CachedASC;
	
	// スロットごとの装備状況を保持するMap（Key: スロット, Value: 装備アイテムとエフェクト情報）
	// （Replicatedにすると構造体内配列の同期が複雑になるため、
	// クライアントへの見た目反映はOnRep_Inventory + 個別Getterの組み合わせで対応）
	UPROPERTY()
	TMap<EEquipmentSlot,FEquippedItemEntry> EquippedItems;
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	
	//装備処理を行うことを要求（ServerRPC）
	UFUNCTION(Server,Reliable)
	void Server_EquipItem(FGuid InstanceID);
	
	//装備解除処理を行うことを要求（ServerRPC）
	UFUNCTION(Server, Reliable)
	void Server_UnequipItem(EEquipmentSlot Slot);
	
	//InventoryItemsがサーバーから同期されたときにクライアントで呼ばれる関数
	UFUNCTION()
	void OnRep_Inventory();
	
	// 装備アイテムに設定されているGameplay Effect（意思、知識）を適用する
	void ApplyItemEffect(FEquippedItemEntry& Entry);
	
	// 装備アイテムを外す際に、適用していたGameplay Effect（意思、知識）を削除する
	void RemoveItemEffects(FEquippedItemEntry& Entry);
	
	
	// 空いているグリッド座標を探す
	bool FindFreeGridSlot(UEquipmentDataAsset* Item, int32& OutX, int32& OutY) const;
	
	
	// ネットワーク同期させる変数を登録するための必須関数
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
};

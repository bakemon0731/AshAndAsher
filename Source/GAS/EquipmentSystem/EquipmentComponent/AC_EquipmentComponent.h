// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AbilitySystemComponent.h"
#include "GAS/EquipmentSystem/DataAsset/EquipmentDataAsset.h"
#include "AC_EquipmentComponent.generated.h"

/**
 * 装備品とインベントリを管理し、GASと連携するコンポーネント
 */

//前方宣言
class AWeaponBase;

//イベントディスパッチャーの宣言
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEquipmentChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUtilityChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponSlotsChanged);

// 装備中のアイテムとそのアイテムによって付与されたエフェクトを管理する構造体
USTRUCT(BlueprintType)
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
	
	//このアイテムインスタンスが所持しているスタック数
	UPROPERTY(BlueprintReadOnly)
	int32 StackCount = 1;
};

USTRUCT(BlueprintType)
struct FUtilitySlotEntry
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UEquipmentDataAsset> Item = nullptr;
	
	UPROPERTY(BlueprintReadOnly)
	int32 StackCount = 0;
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

	//移動速度ペナルティ用の汎用GE（SetByCallerで数値を渡して使う)
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Equipment|Weight")
	TSubclassOf<UGameplayEffect> MoveSpeedPenaltyEffect;

	// 左3 + 右3
	static constexpr int32 NumUtilitySlots = 6;
	static constexpr int32 UtilitySlotsPerGroup = 3;
	
	//0〜2 = 左グループ(3キー)、3〜5 = 右グループ(4キー)
	UPROPERTY(ReplicatedUsing = OnRep_UtilitySlots,BlueprintReadOnly,Category= "Utility")
	TArray<FUtilitySlotEntry> UtilitySlots;
	
	// 今手に持っているスロット。INDEX_NONE(-1) なら武器を持っている
	UPROPERTY(ReplicatedUsing = OnRep_ActiveUtilitySlots,BlueprintReadOnly,Category= "Utility")
	int32 ActiveUtilitySlot = INDEX_NONE;
	
	//イベントディスパッチの定義(FOnUtilityChanged)の変数
	UPROPERTY(BlueprintAssignable, Category="Utility")
	FOnUtilityChanged OnUtilityChanged;
	
	// 0=左(1キー)、1=右(2キー)
	static constexpr int32 NumWeaponSlots = 2;
	
	//0 = 左グループ(1キー)、1 = 右グループ(2キー)
	UPROPERTY(ReplicatedUsing=OnRep_WeaponSlots,BlueprintReadOnly,Category="Weapon")
	TArray<TObjectPtr<UEquipmentDataAsset>> WeaponSlots;
	
	//イベントディスパッチの定義FOnWeaponSlitsChanged)の変数
	UPROPERTY(BlueprintAssignable,Category="Weapon")
	FOnWeaponSlotsChanged OnWeaponSlotsChanged;
	
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
	UFUNCTION(BlueprintCallable,Category="Equipment")
	bool AddItemToInventory(UEquipmentDataAsset* Item,int32 Count = 1);

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
	
	// サーバー専用：装備を外して取り出す（死体の装備を回収する時に使う）
	bool Internal_RemoveEquippedItem(EEquipmentSlot Slot, UEquipmentDataAsset*& OutItem);

	// 死体の装備を、プレイヤー自身のインベントリへ移す（プレイヤー自身のコンポーネントから呼ぶこと）
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void RequestTransferEquippedFromRemote(UAC_EquipmentComponent* RemoteComponent, EEquipmentSlot Slot);
	UFUNCTION(Server, Reliable)
	void Server_TransferEquippedFromRemote(UAC_EquipmentComponent* RemoteComponent, EEquipmentSlot Slot);
	
	
	// ===================== ユーティリティスロット =====================
	UFUNCTION(BlueprintPure,Category="Utility")
	FUtilitySlotEntry GetUtilitySlot(int32 SlotIndex) const;
	
	// ユーティリティスロットに入れられるアイテムか（UIのドロップ判定にも使う)
	UFUNCTION(BlueprintPure,Category="Utility")
	bool CanSetUtilityItem(UEquipmentDataAsset* Item) const;
	
	// インベントリのスタックを、指定スロットへ入れる
	UFUNCTION(BlueprintCallable,Category="Utility")
	void RequestEquipUtility(FGuid InstanceID,int32 SlotIndex);
	UFUNCTION(Server,Reliable)
	void Server_EquipUtility(FGuid InstanceID,int32 SlotIndex);
	
	// スロットの中身をインベントリへ戻す
	UFUNCTION(BlueprintCallable, Category = "Utility")
	void RequestUnequipUtility(int32 SlotIndex);
	UFUNCTION(Server, Reliable)
	void Server_UnequipUtility(int32 SlotIndex);

	// 3キー(GroupIndex=0)・4キー(GroupIndex=1)：グループ内を順に持ち替える
	UFUNCTION(BlueprintCallable, Category = "Utility")
	void RequestCycleUtilityGroup(int32 GroupIndex);
	UFUNCTION(Server,Reliable)
	void Server_CycleUtilityGroup(int32 GroupIndex);
	
	// 手に持っているものを仕舞って、武器に戻る
	UFUNCTION(BlueprintCallable, Category = "Utility")
	void RequestClearActiveUtility();
	UFUNCTION(Server,Reliable)
	void Server_ClearActiveUtility();
	
	// 左クリック：持っているアイテムを使う
	UFUNCTION(BlueprintCallable, Category = "Utility")
	void RequestUseActiveUtility();
	UFUNCTION(Server,Reliable)
	void Server_UseActiveUtility();
	
	// 死体のユーティリティスロットを、プレイヤー自身のインベントリへ移す（プレイヤー自身のコンポーネントから呼ぶこと）
	UFUNCTION(BlueprintCallable, Category = "Utility")
	void RequestTransferUtilityFromRemote(UAC_EquipmentComponent* RemoteComponent, int32 SlotIndex);
	UFUNCTION(Server, Reliable)
	void Server_TransferUtilityFromRemote(UAC_EquipmentComponent* RemoteComponent, int32 SlotIndex);
	
	// ユーティリティスロット同士の入れ替え・合流
	UFUNCTION(BlueprintCallable, Category = "Utility")
	void RequestSwapUtilitySlots(int32 SlotA, int32 SlotB);
	UFUNCTION(Server, Reliable)
	void Server_SwapUtilitySlots(int32 SlotA, int32 SlotB);
	
	// ===================== 武器スロット =====================
	UFUNCTION(BlueprintPure,Category="Weapon")
	UEquipmentDataAsset* GetWeaponSlotItem(int32 SlotIndex) const;
	
	// 武器スロットに入れられるアイテムか（UIのドロップ判定にも使う）
	UFUNCTION(BlueprintPure,Category="Weapon")
	bool CanSetWeaponItem(UEquipmentDataAsset* Item) const;
	
	// インベントリの武器を、指定の武器スロットへ入れる
	UFUNCTION(BlueprintCallable,Category="Weapon")
	void RequestEquipWeaponToSlot(FGuid InstanceID,int32 SlotIndex);
	UFUNCTION(Server,Reliable)
	void Server_EquipWeaponToSlot(FGuid InstanceID,int32 SlotIndex);
	
	// 武器スロットの武器を、インベントリへ戻す
	UFUNCTION(BlueprintCallable,Category="Weapon")
	void RequestUnequipWeaponSlot(int32 SlotIndex);
	UFUNCTION(Server,Reliable)
	void Server_UnequipWeaponSlot(int32 SlotIndex);
	
	// 1キー・2キー：スロットの武器を持つ／仕舞う。サーバーでのみ動く（GAから呼ぶ。EquipWeaponと同じ扱い）
	UFUNCTION(BlueprintCallable,Category="Weapon")
	void ToggleWeaponSlot(int32 SlotIndex);
	
	// サーバー専用：武器スロットの中身を取り出す（死体から回収する時に使う）
	bool Internal_removeWeaponSlot(int32 SlotIndex,UEquipmentDataAsset*& OutItem);
	
	// 死体の武器スロットを、プレイヤー自身のインベントリへ移す（プレイヤー自身のコンポーネントから呼ぶこと）
	UFUNCTION(BlueprintCallable,Category="Weapon")
	void RequestTransferWeaponFromRemote(UAC_EquipmentComponent* RemoteComponent,int32 SlotIndex);
	UFUNCTION(Server,Reliable)
	void Server_TransferWeaponFromRemote(UAC_EquipmentComponent* RemoteComponent,int32 SlotIndex);
	
	// 左右の武器スロットの入れ替え
	UFUNCTION(BlueprintCallable,Category="Weapon")
	void RequestSwapWeaponSlots(int32 SlotA, int32 SlotB);
	UFUNCTION(Server,Reliable)
	void Server_SwapWeaponSlots(int32 SlotA, int32 SlotB);
	
protected:

	// 登録されたAbility System Componentのキャッシュ
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> CachedASC;
	
	UPROPERTY()
	TMap<EEquipmentSlot,FEquippedItemEntry> EquippedItems;

	// サーバーのみ：各武器スロットに対応する、ワールド上の武器Actor
	TArray<TObjectPtr<AWeaponBase>> WeaponSlotActors;
	
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
	
	void RebuildEquippedSlotsReplicated();
	
	//クライアントが生きている他プレイヤーの装備を盗めないようにするための検証
	bool IsValidLootTarget(const UAC_EquipmentComponent* Remote) const;
	
	UFUNCTION()
	void OnRep_UtilitySlots();
	
	UFUNCTION()	
	void OnRep_ActiveUtilitySlots();
	
	// 手に持つスロットを変更し、武器コンポーネントへ反映する（サーバーのみ）
	UFUNCTION()
	void Internal_SetActiveUtilitySlots(int32 NewSlot);
	
	// 武器コンポーネントの持ち物が変わった時に呼ばれる（武器キーで武器に持ち替えた等）
	UFUNCTION()
	void HandleWeaponChanged(AWeaponBase* NewWeapon);
	
	//サーバーのみ使用
	double NextUtilityUseTime = 0.0f;
	
	// サーバー専用：ユーティリティスロットの中身を取り出す（死体から回収する時に使う）
	bool Internal_RemoveUtilitySlot(int32 SlotIndex, FUtilitySlotEntry& OutEntry);
	
	UFUNCTION()
	void OnRep_WeaponSlots();
	
	void SpawnWeaponSlotActor(int32 SlotIndex);
	
	void ClearWeaponSlotActor(int32 SlotIndex);
	
public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
};

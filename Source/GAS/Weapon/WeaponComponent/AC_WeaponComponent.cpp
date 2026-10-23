// Fill out your copyright notice in the Description page of Project Settings.


#include "AC_WeaponComponent.h"
#include "GAS/GameplayAbilitySystem/Characters/NexusCharacterBase.h"
#include "GAS/Weapon/WeaponBase/WeaponBase.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GAS/EquipmentSystem/DataAsset/EquipmentDataAsset.h"


UAC_WeaponComponent::UAC_WeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	
	// コンポーネントのレプリケーションを有効にする
	SetIsReplicatedByDefault(true);


}

void UAC_WeaponComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	//EquippedWeapon変数をレプリケーション対象に登録。
	DOREPLIFETIME(UAC_WeaponComponent,EquippedWeapon);
	//PreviouslyEquippedWeapon変数をレプリケーション対象に登録。
	DOREPLIFETIME(UAC_WeaponComponent,PreviouslyEquippedWeapon);
	
}

void UAC_WeaponComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// Ownerを取得し、NexusCharacterBaseにキャストして保存
	OwningCharacter = Cast<ANexusCharacterBase>(GetOwner());
	
	// Authority(サーバー)でのみ実行
	if (OwningCharacter && OwningCharacter -> HasAuthority())
	{
		// StartingWeapons配列をループして武器を付与
		for (TSubclassOf<AWeaponBase> WeaponClass : StartingWeapons)
		{
			if (WeaponClass)
			{
				//GiveWeapon関数を実行
				GiveWeapon(WeaponClass);
			}
		}
	}
}

void UAC_WeaponComponent::OnRep_EquippedWeapon(AWeaponBase* OldWeapon)
{
	// 別の物へ直接持ち替えた場合、仕舞うモンタージュは再生されないので、ここで直接仕舞う
	if (IsValid(EquippedWeapon) && IsValid(OldWeapon) && OldWeapon != EquippedWeapon)
	{
		StowWeaponImmediately(OldWeapon);
	}
	
	// サーバーの武器の見た目をクライアントに反映させる処理
	if (IsValid(EquippedWeapon))
	{
		SetEquippedWeaponProperties();
	}
	else
	{
		SetUnarmedWeaponConfig();
	}
}

void UAC_WeaponComponent::GiveWeapon(TSubclassOf<AWeaponBase> WeaponClass)
{
	
	UWorld* World = GetWorld();
	if (!OwningCharacter ||	!WeaponClass || !World)
	{
		return;
	}
	
	// Spawn Actor 用のパラメータ設定
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Instigator = OwningCharacter;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	//スポーンアクター
	AWeaponBase* SpawnWeapon = World->SpawnActor<AWeaponBase>(
		WeaponClass,FVector::ZeroVector,FRotator::ZeroRotator,SpawnParameters);
	
	if (SpawnWeapon)
	{
		// StowedWeapons配列にユニーク追加
		StowedWeapons.AddUnique(SpawnWeapon);
		
		// 武器のコンフィグからアタッチ用のソケット名を取得
		FName SocketName = SpawnWeapon->WeaponConfig.StowedSocketName;
		
		// OwningCharacterのメッシュを取得してアタッチ
		if (USkeletalMeshComponent* CharacterMesh = OwningCharacter->GetMesh())
		{
			// Location/Rotation/Scale は Keep Relative (相対位置を維持) を指定
			SpawnWeapon->AttachToComponent(CharacterMesh,FAttachmentTransformRules::KeepRelativeTransform,SocketName);
		}
	}
}

void UAC_WeaponComponent::EquipWeapon(TSubclassOf<AWeaponBase> WeaponClass)
{
	// サーバーのみ実行 (Switch Has Authority)
	if (!GetOwner()->HasAuthority())
	{
		return;
	}
	
	//OwningCharacterの有効チェック
	if (!IsValid(OwningCharacter))
	{
		return;
	}
	
	// 武器キーで武器を出したので、持つ前の記録は不要
	WeaponBeforeHeld = nullptr;
	
	// 現在の武器クラスをチェックし、同じなら終了、違うなら外す
	if (IsValid(EquippedWeapon))
	{
		if (EquippedWeapon->GetClass() == WeaponClass)
		{
			// 等しい場合は武器をすでに持っていて、武器をしまうことを示唆している。
			UnequipWeapon();
			
			return;
		}
		else
		{
			// 等しくない場合は、今の物を外して、直接背中へ戻す
			AWeaponBase* Old = EquippedWeapon;
			UnequipWeapon();
			StowWeaponImmediately(Old);
		}
	}
	
	//取り出す武器が背中にある武器と等しいか調べる。
	AWeaponBase* StowedWeapon = GetStowedWeaponByClass(WeaponClass);
	
	if (IsValid(StowedWeapon))
	{
		// 武器をセットして通知（レプリケート)
		EquippedWeapon = StowedWeapon;
		
		// 装備武器のプロパティの設定関数。
		SetEquippedWeaponProperties();
	}
}

void UAC_WeaponComponent::UnequipWeapon()
{
	// EquippedWeaponがValidならPreviouslyEquippedWeaponに保存。
	if (IsValid(EquippedWeapon))
	{
		PreviouslyEquippedWeapon = EquippedWeapon;
		
		//消耗品は背中に戻さず、仕舞う動作の後に消す
		if (EquippedWeapon->IsHeldItem)
		{
			EquippedWeapon->SetLifeSpan(EquippedWeapon->HeldItemDestroyDelay);
		}
	}
	
	// EquippedWeaponを空にする。
	EquippedWeapon = nullptr;
	
	// 解除武器のプロパティの設定関数を実行。
	SetUnarmedWeaponConfig();
}

AWeaponBase* UAC_WeaponComponent::GetStowedWeaponByClass(TSubclassOf<AWeaponBase> WeaponClass)
{
	// 配列をループしてクラスが一致するものを探す
	for (AWeaponBase* Weapon : StowedWeapons)
	{
		if (IsValid(Weapon) && Weapon->GetClass() == WeaponClass)
		{
			// 一致したら値を返す
			return Weapon;
		}
	}
	// 見つからなかった場合はnullptrを返す
	return nullptr;
}

void UAC_WeaponComponent::SetEquippedWeaponProperties()
{
	// EquippedWeaponとOwningCharacterのValidチェック
	if (!IsValid(EquippedWeapon) || !IsValid(OwningCharacter))
	{
		return;
	}
	
	//武器の移動速度ペナルティGEを適用
	RefreshWieldPenalty();
	
	// 武器の親アクターから継承した構造体を取り出す (Break SWeaponConfig)
	FSWeaponConfig WeaponConfig=EquippedWeapon->WeaponConfig;
	
	// メッシュの取得
	USkeletalMeshComponent* CharacterMesh = OwningCharacter->GetMesh();
	if (!IsValid(CharacterMesh))
	{
		return;
	}
	
	// AnimInstanceClassを設定
	if (WeaponConfig.AnimClass)
	{
		CharacterMesh->SetAnimInstanceClass(WeaponConfig.AnimClass);
	}
	
	// カメラを設定 (Break SMovementProperties)
	UCharacterMovementComponent* MovementComp = OwningCharacter->GetCharacterMovement();
	if (IsValid(MovementComp) && !EquippedWeapon->IsHeldItem)
	{
		FSMovementProperties MoveProps = WeaponConfig.MovementProperties;
		
		MovementComp->bOrientRotationToMovement = MoveProps.OrientRotationToMovement;
		MovementComp->bUseControllerDesiredRotation = MoveProps.UseControllerDesiredRotation;
	}
	
	//武器を変更したことをコール (イベントディスパッチャー)
	OnWeaponChanged.Broadcast(EquippedWeapon);
	
	//BPでPlayMontageは任せる
	if (IsValid(EquippedWeapon))
	{
		PlayEquipMontage(EquippedWeapon->WeaponConfig);
	}
}

void UAC_WeaponComponent::SetUnarmedWeaponConfig()
{
	if (!IsValid(OwningCharacter))
	{
		return;
	}
	
	RefreshWieldPenalty();
	
	USkeletalMeshComponent* CharacterMesh = OwningCharacter->GetMesh();
	
	if (!IsValid(CharacterMesh))
	{
		return;
	}
	
	// AnimInstanceClassを設定
	if (UnarmedWeaponConfig.AnimClass)
	{
		CharacterMesh->SetAnimInstanceClass(UnarmedWeaponConfig.AnimClass);
	}
	
	//BPでPlayMontageは任せる
	if (IsValid(PreviouslyEquippedWeapon))
	{
		PlayUnequipMontage(PreviouslyEquippedWeapon->WeaponConfig);
	}
}

void UAC_WeaponComponent::SetUnarmedProperties()
{
	// UnarmedWeaponConfig変数から構造体を取り出す (Break SWeaponConfig)
	FSWeaponConfig WeaponConfig = UnarmedWeaponConfig;
	
	UCharacterMovementComponent* MovementComp = OwningCharacter->GetCharacterMovement();
	if (IsValid(MovementComp))
	{
		FSMovementProperties MoveProps = WeaponConfig.MovementProperties;
		
		MovementComp->bOrientRotationToMovement = MoveProps.OrientRotationToMovement;
		MovementComp->bUseControllerDesiredRotation = MoveProps.UseControllerDesiredRotation;
	}
	
	//武器を変更したことをコール (イベントディスパッチャー)
	OnWeaponChanged.Broadcast(nullptr);
	
	IsWieldingWeapon = false;
}

void UAC_WeaponComponent::StowWeaponImmediately(AWeaponBase* Weapon)
{
	if (!IsValid(Weapon) || !IsValid(OwningCharacter))
	{
		return;
	}
	
	// 消耗品は仕舞わずに消す。クライアントは見えなくするだけで、実体の破棄はサーバーに任せる
	if (Weapon->IsHeldItem)
	{
		// 寿命が未設定の場合だけ設定する（サーバーのみ）
		if (Weapon->HasAuthority() && Weapon->GetLifeSpan() <= 0.f)
		{
			Weapon->SetLifeSpan(Weapon->HeldItemDestroyDelay);
		}
		return;
	}
	
	if (USkeletalMeshComponent* CharacterMesh = OwningCharacter->GetMesh())
	{
		Weapon->AttachToComponent(CharacterMesh,FAttachmentTransformRules::SnapToTargetIncludingScale,
			Weapon->WeaponConfig.StowedSocketName);
	}
}

void UAC_WeaponComponent::RefreshWieldPenalty()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
	if (!ASC)
	{
		return;
	}
	
	//前のペナルティを外す
	if (WieldPenaltyHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(WieldPenaltyHandle);
		WieldPenaltyHandle.Invalidate();
	}
	
	float Penalty = 0.0f;
	if (IsValid(EquippedWeapon))
	{
		if (EquippedWeapon->ItemData)
		{
			Penalty = EquippedWeapon->ItemData->MoveSpeedPenalty;
		}
		else if (!EquippedWeapon->IsHeldItem)
		{
			// 武器にItemDataを設定し忘れた用
			UE_LOG(LogTemp, Warning, TEXT("%s に ItemData が設定されていないため、移動速度ペナルティが掛かりません"), 
				*GetNameSafe(EquippedWeapon));
		}
	}
	
	if (Penalty <= 0.0f ||!WieldMoveSpeedPenaltyEffect)
	{
		return;
	}
	
	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(WieldMoveSpeedPenaltyEffect, 1.f, Context);
	//GameplayEffectを適用
	if (Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.WeightPenalty")), -Penalty);
		WieldPenaltyHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}
}


void UAC_WeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                        FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

//--------------------------------HeldItem-------------------------------
void UAC_WeaponComponent::EquipHeldItem(TSubclassOf<AWeaponBase> HeldClass)
{
	UWorld* World = GetWorld();
	if (!GetOwner()->HasAuthority() || !IsValid(OwningCharacter) || !HeldClass || !World)
	{
		return;
	}
    
	// 既に同じ物を持っているなら何もしない
	if (IsValid(EquippedWeapon) && EquippedWeapon->GetClass() == HeldClass)
	{
		return;
	}
    
	//既に別の武器を持っている場合は、安全に仕舞う処理を入れる
	if (IsValid(EquippedWeapon))
	{
		AWeaponBase* Old = EquippedWeapon;
		UnequipWeapon();
		StowWeaponImmediately(Old);
	}
    
	// 持つ前の武器を覚える（別の消耗品から持ち替える時は、最初の記録を引き継ぐ）
	if (!IsValid(EquippedWeapon))
	{
		WeaponBeforeHeld = nullptr;
	}
	else if (!EquippedWeapon->IsHeldItem)
	{
		WeaponBeforeHeld = EquippedWeapon->GetClass();
	}
    
	// スポーンパラメーターの設定
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Instigator = OwningCharacter;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    
	AWeaponBase* Held = World->SpawnActor<AWeaponBase>(HeldClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
	if (!Held)
	{
		return;
	}
    
	// 常設の武器と違い、StowedWeaponsには入れない。（生成直後に手のソケットへ付けて、原点に出るのを防ぐため）
	if (USkeletalMeshComponent* CharacterMesh = OwningCharacter->GetMesh())
	{
		Held->AttachToComponent(CharacterMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		   Held->WeaponConfig.EquippedSocketName);
	}
    
	EquippedWeapon = Held;
	SetEquippedWeaponProperties();
}

void UAC_WeaponComponent::ReturnFromHeldItem()
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}
	
	// 既に武器キーで別の武器に替わっているなら、何もしない
	if (!IsValid(EquippedWeapon) || !EquippedWeapon->IsHeldItem)
	{
		return;
	}
	
	const TSubclassOf<AWeaponBase> Before = WeaponBeforeHeld;
	WeaponBeforeHeld = nullptr;
	
	if (Before)
	{
		EquipWeapon(Before);
	}
	else
	{
		UnequipWeapon();
	}
}


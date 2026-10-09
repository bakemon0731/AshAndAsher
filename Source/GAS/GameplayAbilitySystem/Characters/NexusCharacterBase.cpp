// Fill out your copyright notice in the Description page of Project Settings.


#include "NexusCharacterBase.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS/EquipmentSystem/EquipmentComponent/AC_EquipmentComponent.h"
#include "GAS/GamePlayAbilitySystem/NexusAbilitySystemComponent.h"
#include "GAS/GamePlayAbilitySystem/Abilities/NexusGameplayAbility.h"
#include "GAS/GamePlayAbilitySystem/AttributeSets/BasicAttributeSet.h"
#include "GAS/GamePlayAbilitySystem/AttributeSets/CombatAttributeSet.h"
#include "GAS/GameplayAbilitySystem/AttributeSets/PrimaryAttributeSet.h"
#include "GAS/SpellSystem/SpellComponent/AC_SpellComponent.h"
#include "GAS/StatSystem/AC_StatComponent.h"
#include "GAS/EquipmentSystem/EquipmentComponent/AC_EquipmentComponent.h"

// Sets default values
ANexusCharacterBase::ANexusCharacterBase()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Add the Ability System Component
	AbilitySystemComponent = CreateDefaultSubobject<UNexusAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(AscReplicationMode);
	
	//Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(35.f, 90.0f);
	
	//Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	
	//Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; 
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.f;
	
	//基本属性セットを追加
	BasicAttributeSet = CreateDefaultSubobject<UBasicAttributeSet>(TEXT("BasicAttributeSet"));
	//戦闘用の属性セットを追加
	CombatAttributeSet = CreateDefaultSubobject<UCombatAttributeSet>(TEXT("CombatAttributeSet"));
	//ステータス用の属性セットを追加
	PrimaryAttributeSet = CreateDefaultSubobject<UPrimaryAttributeSet>(TEXT("PrimaryAttributeSet"));
	//AC_SpellComponentを追加
	SpellManagerComponent = CreateDefaultSubobject<UAC_SpellComponent>(TEXT("SpellManagerComponent"));
	//AC_StatComponentを追加
	StatManagerComponent = CreateDefaultSubobject<UAC_StatComponent>(TEXT("StatManagerComponent"));
	//AC_EquipmentComponentを追加
	EquipmentManagerComponent = CreateDefaultSubobject<UAC_EquipmentComponent>(TEXT("EquipmentManagerComponent"));
}

int32 ANexusCharacterBase::GetTeamNumber() const
{
	return TeamNumber;
}

UAnimMontage* ANexusCharacterBase::GetHitReactionMontage_Implementation() const
{
	return HitReactionMontage;
}

// Called when the game starts or when spawned
void ANexusCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	
	AbilitySystemComponent->RegisterGameplayTagEvent(FGameplayTag::RequestGameplayTag("State.Dead"))//State.Deadタグが追加された時に呼ばれるバインドイベント。
	.AddUObject(this, &ANexusCharacterBase::OnDeathTagChanged);
}

// Called every frame
void ANexusCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input

//移動、ジャンプ、アビリティ発動などの入力を設定する場所
void ANexusCharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}


//コントローラーがキャラクターを操作し始めた時に呼ばれます（サーバー側でのみ呼ばれる）
void ANexusCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
		// GA_CastSelectedSpell を含む
		GrantAbilities(StartingAbilities);
		
		if (SpellManagerComponent)
		{
			SpellManagerComponent->InitializeKnownSpells(AbilitySystemComponent);
		}
		
		if(EquipmentManagerComponent)
		{
			EquipmentManagerComponent->InitializeEquipmentSystem(AbilitySystemComponent);
		}
		
		if (SpellManagerComponent)
		{
			StatManagerComponent->InitializeStats();
		}
	}
	
	
}

bool ANexusCharacterBase::ActivateSpellByIndex(int32 SpellIndex)
{
	if (!AbilitySystemComponent)
	{
		return false;
	}
	
	if (!SpellAbilityHandles.IsValidIndex(SpellIndex))
	{
		return false;
	}
	
	return AbilitySystemComponent->TryActivateAbility(SpellAbilityHandles[SpellIndex]);
}

//PlayerStateがネットワーク上でレプリケート（同期）された時に呼ばれます（クライアント側）
//マルチプレイヤーゲームで、クライアント側でも初期化が必要だから
void ANexusCharacterBase::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

//Ability System Componentへのアクセスを提供するゲッター関数
//他のクラスからこのキャラクターのアビリティシステムにアクセスする時に使用
UAbilitySystemComponent* ANexusCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}


TArray<FGameplayAbilitySpecHandle> ANexusCharacterBase::GrantAbilities//付与するアビリティのハンドル取得
(TArray<TSubclassOf<UGameplayAbility>> AbilitiesToGrant,int32 Level)
{
	if (!AbilitySystemComponent || !HasAuthority())// AbilitySystemComponent が存在し、このアクター（キャラクター）がサーバー上で権限を持つか確認
	{
		return TArray<FGameplayAbilitySpecHandle>();//もし条件を満たさなければ、空の配列を返す。
	}
	
	TArray<FGameplayAbilitySpecHandle> AbilityHandles;
	
	for (TSubclassOf<UGameplayAbility> Ability : AbilitiesToGrant)
	{
		
		int32 InputID = -1;// InputID を初期化
		bool ShouldActivate = false;
		if (const UNexusGameplayAbility* NexusAbilityCDO = GetDefault<UNexusGameplayAbility>(Ability))// もし Ability が NexusGameplayAbility のサブクラスであれば、CDO（Class Default Object）を取得
		{
			InputID = static_cast<int32>(NexusAbilityCDO->AbilityInputID);// AbilityInputID を CDO から取得して設定
			ShouldActivate = NexusAbilityCDO->AutoActivateWhenGranted;// AutoActivateWhenGranted を CDO から取得して設定
		}
		
		
		FGameplayAbilitySpecHandle SpecHandle = AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(
			Ability,Level, InputID, this
			));
		AbilityHandles.Add(SpecHandle);//返されたハンドルを配列に追加して返す
		
		
		if (ShouldActivate)//もしアビィリティが付与された時に自動で発動する設定なら、アビリティを発動する
		{
			AbilitySystemComponent->TryActivateAbility(SpecHandle);
		}
	}
	
	SendAbilitiesChangedEvent();
	return AbilityHandles;
}

//ハンドルで削除
void ANexusCharacterBase::RemoveAbilities
(TArray<FGameplayAbilitySpecHandle> AbilityHandlesToRemove)
{
	if (!AbilitySystemComponent || !HasAuthority())
	{
		return;
	}
	
	for (FGameplayAbilitySpecHandle AbilityHandle : AbilityHandlesToRemove)
	{
		AbilitySystemComponent->ClearAbility(AbilityHandle);
	}
	
	SendAbilitiesChangedEvent();
}

//アビリティの変更を知らせるゲームプレイイベントを送る関数
void ANexusCharacterBase::SendAbilitiesChangedEvent()
{
	FGameplayEventData EventData;//送るイベント用のデータ構造を作成
	EventData.EventTag = FGameplayTag::RequestGameplayTag(FName("Event.Abilities.Changed"));//イベントのタグを設定
	EventData.Instigator = this;//イベントの発生者・対象を設定（ここでは同じキャラクター）
	EventData.Target = this;// 指定したアクター（このキャラクター）へ Gameplay Event を送信
	
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, EventData.EventTag, EventData);
}

//サーバー側から全クライアントにイベントを送信する関数の実装
void ANexusCharacterBase::MultiSendGameplayEventToSelf_Implementation(AActor* TargetActor,
                                                                      FGameplayEventData EventData)
{
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetActor, EventData.EventTag, EventData);
}

// サーバー上で、このキャラクター（自身）に対して GameplayEvent を送る処理
void ANexusCharacterBase::ServerSendGameplayEventToSelf_Implementation(FGameplayEventData EventData)
{
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, EventData.EventTag, EventData);
}

//死亡時の処理を実装する関数（ブループリントでオーバーライド可能）
void ANexusCharacterBase::HandleDeath_Implementation()
{
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->DisableMovement();
	
}

void ANexusCharacterBase::OnDeathTagChanged(const FGameplayTag CallbackTag, int32 NewCount)//死亡時のタグが追加された。
{
	if (NewCount > 0)
	{
		HandleDeath();//死亡時の処理を実行
	}
	
}

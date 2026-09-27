// Fill out your copyright notice in the Description page of Project Settings.

#include "BasicAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "GameFramework/Pawn.h"
#include "GAS/GamePlayAbilitySystem/Characters/NexusCharacterBase.h"
#include "Perception/AISense_Damage.h"
#include "Net/UnrealNetwork.h"
#include "GAS/Interface/Damageable.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UBasicAttributeSet::UBasicAttributeSet()
{
    Health = 100.f;
    MaxHealth = 100.f;
    Stamina = 100.f;
    MaxStamina = 100.f;
    Damage = 0.f;
    Shield = 0.f;
    MaxShield = 100.f;
   MoveSpeed = 500.f;
   MaxMoveSpeed = 500.f;
   MaxMemoryCapacity = 0.f;
}

//変数の同期設定
void UBasicAttributeSet::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(UBasicAttributeSet, Health, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UBasicAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UBasicAttributeSet, Stamina, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UBasicAttributeSet, MaxStamina, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UBasicAttributeSet, Shield, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UBasicAttributeSet, MaxShield, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UBasicAttributeSet, MoveSpeed, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UBasicAttributeSet, MaxMoveSpeed, COND_None, REPNOTIFY_Always);
   DOREPLIFETIME_CONDITION_NOTIFY(UBasicAttributeSet, MaxMemoryCapacity, COND_None, REPNOTIFY_Always);
}

//値の制限
void UBasicAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);

    if (Attribute == GetHealthAttribute())
    {
       NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
    }
    else if (Attribute == GetStaminaAttribute())
    {
       NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxStamina());
    }
    else if (Attribute == GetShieldAttribute())
    {
       NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxShield());
    }  
    else if (Attribute == GetMoveSpeedAttribute())
    {
       NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxMoveSpeed());
    }
}

//Gameplay Effect が実行された時
void UBasicAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);
    
    if (Data.EvaluatedData.Attribute == GetDamageAttribute())
    {
       float TotalDamage = GetDamage();
       SetDamage(0.f); // Meta Attribute を即座にリセット
       
       // -------------------------------------------------------------------
       // チーム判定
       // -------------------------------------------------------------------
       AActor* TargetActor = GetOwningActor();
       AActor* InstigatorActor = Data.EffectSpec.GetContext().GetInstigator();
       
       if (TargetActor && InstigatorActor)
       {
          // アクターを IDamageable インターフェースにキャストする
         IDamageable* TargetDamageable = Cast<IDamageable>(TargetActor);
          IDamageable* InstigatorDamageable = Cast<IDamageable>(InstigatorActor);
          
          // 両者がインターフェースを実装している場合のみ判定を行う
          if (TargetDamageable && InstigatorDamageable)
          {
             UE_LOG(LogTemp, Warning, TEXT("Target Team: %d, Instigator Team: %d"),
      TargetDamageable->GetTeamNumber(), InstigatorDamageable->GetTeamNumber());
             
             // インターフェース経由でTeamNumberを取得して比較
             if (TargetDamageable->GetTeamNumber() == InstigatorDamageable->GetTeamNumber())
             {
                // 同チームなら処理を中断
                return;
             }
          }
       }   
       
       if (TotalDamage > 0.f)
       {
          
          // -------------------------------------------------------------------
          // ダメージテキスト用 GameplayCue の実行（各クライアントへ通知）
          // -------------------------------------------------------------------
          FGameplayCueParameters CueParams;
          CueParams.RawMagnitude = TotalDamage; // 計算された最終ダメージ数値

          // ヒット位置を取得（ContextにHitResultがあればその位置、なければアクター位置）
          const FHitResult* HitResult = Data.EffectSpec.GetContext().GetHitResult();
          if (HitResult && HitResult->bBlockingHit)
          {
             CueParams.Location = HitResult->ImpactPoint;
          }
          else if (TargetActor)
          {
             CueParams.Location = TargetActor->GetActorLocation();
          }

          CueParams.EffectCauser = Data.EffectSpec.GetContext().GetEffectCauser();
          CueParams.Instigator = Data.EffectSpec.GetContext().GetInstigator();

          // ターゲットの AbilitySystemComponent 経由で GameplayCue を発火
          if (UAbilitySystemComponent* TargetASC = GetOwningAbilitySystemComponent())
          {
             TargetASC->ExecuteGameplayCue(
                FGameplayTag::RequestGameplayTag(FName("GameplayCue.Damage.DamageText")),
                CueParams
             );
          }

          // -------------------------------------------------------------------
          // AIへの感知（ダメージイベント）送信
          // -------------------------------------------------------------------
          AActor* SourceActor = Data.EffectSpec.GetContext().GetInstigator();
          APawn* Sourcepawn = Cast<APawn>(SourceActor);
          
          if (TargetActor && Sourcepawn)
          {
             if (TargetActor != Sourcepawn)
             {
                UAISense_Damage::ReportDamageEvent(
                   TargetActor,
                   TargetActor,
                   Sourcepawn,
                   TotalDamage,
                   TargetActor->GetActorLocation(),
                   TargetActor->GetActorLocation(),
                   FName("Damage")
                );
             }
          }

          // -------------------------------------------------------------------
          // シールドおよび HP 減算処理
          // -------------------------------------------------------------------
          float CurrentShield = GetShield();
          if (CurrentShield > 0.f)
          {
             SetShield(CurrentShield - TotalDamage);
             float RemaningDamage = TotalDamage - CurrentShield;
             
             if (RemaningDamage > 0.f)
             {
                SetHealth(GetHealth() - RemaningDamage);
             }
          }
          else
          {
             SetHealth(GetHealth() - TotalDamage);
          }
          
          // -------------------------------------------------------------------
          // 被弾リアクションアビリティの発動
          // -------------------------------------------------------------------
          if (Data.EffectSpec.Def->GetAssetTags().HasTag(FGameplayTag::RequestGameplayTag("Effects.HitReaction"))
             && Data.EvaluatedData.Magnitude != 0.f)
          {
             FGameplayTagContainer HitReactionTagContainer;
             HitReactionTagContainer.AddTag(FGameplayTag::RequestGameplayTag("GameplayAbility.HitReaction"));
             GetOwningAbilitySystemComponent()->TryActivateAbilitiesByTag(HitReactionTagContainer);
          }
       }
    }
   
    if (Data.EvaluatedData.Attribute == GetHealthAttribute())
    {
       SetHealth(GetHealth());
    }
    else if (Data.EvaluatedData.Attribute == GetStaminaAttribute())
    {
       SetStamina(GetStamina());
    }
   
   
}

//「値が変化したすべてのタイミング」で確実に呼ばれる
void UBasicAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
    Super::PostAttributeChange(Attribute, OldValue, NewValue);
    
   //死亡判定処理
    if (Attribute == GetHealthAttribute() && NewValue <= 0.f)
    {
       FGameplayTagContainer DeathAbilityTagContainer;
       DeathAbilityTagContainer.AddTag(FGameplayTag::RequestGameplayTag("GameplayAbility.Death"));
       GetOwningAbilitySystemComponent()->TryActivateAbilitiesByTag(DeathAbilityTagContainer);
    }
    
   //シールド判定処理
    if (Attribute == GetShieldAttribute())
    {
       if (NewValue > 0.f && OldValue <= 0.f)
       {
          GetOwningAbilitySystemComponent()->AddGameplayCue(FGameplayTag::RequestGameplayTag("GameplayCue.ShieldUp"));
       }  
       else if(NewValue <= 0.f && OldValue > 0.f)
       {
          GetOwningAbilitySystemComponent()->RemoveGameplayCue(FGameplayTag::RequestGameplayTag("GameplayCue.ShieldUp"));
          GetOwningAbilitySystemComponent()->ExecuteGameplayCue(FGameplayTag::RequestGameplayTag("GameplayCue.ShieldDown"));
       }  
    }
   
   //移動速度反映処理
   if (Attribute == GetMoveSpeedAttribute())
   {
      // オーナーをACharacterとして取得
      ACharacter* OwningCharacter = Cast<ACharacter>(GetOwningActor());
      
      // CharacterMovementComponentを持っているか確認し、値を上書きする
      if (OwningCharacter && OwningCharacter->GetCharacterMovement())
      {
         OwningCharacter->GetCharacterMovement()->MaxWalkSpeed = NewValue;
      }
   }
   
}
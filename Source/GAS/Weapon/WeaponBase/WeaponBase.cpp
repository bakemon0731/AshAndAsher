// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponBase.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/KismetSystemLibrary.h"// トレース処理に必要
#include "AbilitySystemGlobals.h"// アクターからASCを簡単に取得するために必要
#include "AbilitySystemComponent.h"     // GAS処理に必要
#include "TimerManager.h"// タイマー処理に必要


// Sets default values
AWeaponBase::AWeaponBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
	// ネットワークマルチプレイ用のレプリケーションを有効化
	bReplicates = true;
	
	//宣言したコンポーネントを生成し、ブループリントのコンポーネントツリーと同じ階層構造
	// ルートコンポーネントの作成
	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = DefaultSceneRoot;

	// 武器メッシュの作成とアタッチ
	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(DefaultSceneRoot);
	
	// トレース開始・終了ポイントの作成とアタッチ
	TraceStart = CreateDefaultSubobject<USceneComponent>(TEXT("TraceStart"));
	TraceStart->SetupAttachment(DefaultSceneRoot);

	TraceEnd = CreateDefaultSubobject<USceneComponent>(TEXT("TraceEnd"));
	TraceEnd->SetupAttachment(DefaultSceneRoot);
	
	//発射物などの生成位置の作成とアタッチ
	SpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnPoint"));
	SpawnPoint->SetupAttachment(DefaultSceneRoot);
	
}

// Called when the game starts or when spawned
void AWeaponBase::BeginPlay()
{
	Super::BeginPlay();
	
	//動的マテリアルを一度だけ作成して保持
	if (WeaponMesh)
	{
		WeaponMID = WeaponMesh->CreateAndSetMaterialInstanceDynamic(0);
	}
	
}

void AWeaponBase::HitScan()
{
	// サーバーでのみ実行する（マルチプレイ対策）
	if (!HasAuthority()) return;
	
	// 念のためコンポーネントが存在するかチェック
	if (!TraceStart || !TraceEnd) return;
	
	// トレースの始点と終点を取得
	FVector StartLocation = TraceStart->GetComponentLocation();
	FVector EndLocation = TraceEnd->GetComponentLocation();
	
	// トレース対象のオブジェクトタイプ（Pawnのみヒットする設定）
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));
	
	// 無視するアクターの設定（武器の所有者はダメージを受けないようにする）
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(GetInstigator());
	
	// トレース結果を格納する配列
	TArray<FHitResult> OutHits;
	
	// 球トレースの実行 (BPの Multi Sphere Trace For Objects ノード)
	bool bHit = UKismetSystemLibrary::SphereTraceMultiForObjects(this,
		StartLocation, 
		EndLocation, 
		HitScanRadius, 
		ObjectTypes, 
		false,// Trace Complex
		ActorsToIgnore, 
		EDrawDebugTrace::None, // デバッグ描画（完成時は None に変更）
		OutHits, 
		true,// Ignore Self
		FLinearColor::Red,
		FLinearColor::Green,
		0.5f// デバッグ描画の表示時間
		);
	
	// トレースで何かにヒットした場合の処理
	if (bHit)
	{
		// 取得したヒット結果を1つずつ処理 (BPの For Each Loop ノード)
		for (const FHitResult& Hit : OutHits)
		{
			AActor* HitActor = Hit.GetActor();
			
			// アクターが有効か確認
			if (IsValid(HitActor))
			{
				// ヒットしたアクターが既にHitActors配列に含まれているか確認 (BPの CONTAINS ノード)
				if(!HitActors.Contains(HitActor))
				{
					// 含まれていないので配列に追加 (BPの ADD UNIQUE ノード)
					HitActors.AddUnique(HitActor);
					
					// HitActor(ターゲット)とInstigator(ソース)のAbilitySystemComponentを取得
					UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(HitActor);
					UAbilitySystemComponent* SourceASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetInstigator());
					
					// 双方のAbilitySystemComponentが有効かつ、EffectSpecHandleが有効である場合のみ実行
					if (TargetASC && SourceASC && EffectSpecHandle.IsValid())
					{
						// この武器を所有しているアクター(ソース)から、ターゲットへGameplayEffectを実行する
						SourceASC->ApplyGameplayEffectSpecToTarget(*EffectSpecHandle.Data.Get(), TargetASC);
						
					}
					
					if (TargetASC && SourceASC && DebufftoApply)
					{
						// 1. エフェクトコンテキストを作成
						FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
						EffectContext.AddSourceObject(this);

						// 2. スペックハンドルを作成（スクショに合わせてレベルを 0.0f に変更）
						FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DebufftoApply, 0.0f, EffectContext);

						if (SpecHandle.IsValid())
						{
							// 3. ターゲットにGEを適用
							SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
						}
					}
				}  
			}
		}
	}
}

void AWeaponBase::HitScanStart(const FGameplayEffectSpecHandle InEffectSpecHandle)
{
	// 1. 受け取ったEffectSpecHandleをセット
	EffectSpecHandle = InEffectSpecHandle;
	
	// 2. HitActors配列を空にして、同じアクターに再びヒットできるようにする (BPの CLEAR ノード)
	HitActors.Empty();
	
	// 3. 1秒間に約30回ヒットスキャンを実行するタイマーをセット(BPの Set Timer by Event ノード)
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			HitScanTimer,//Return Value
			this, 
			&AWeaponBase::HitScan, //Create Event = HitScan
			0.033333f, 
			true// Looping = true
			);
	}
}

void AWeaponBase::HitScanEnd()
{
	// ヒットスキャン(タイマー)を止めてハンドルを無効化する
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HitScanTimer);
	}
}

FVector AWeaponBase::GetSpawnPointLocation() const
{
	// SpawnPointコンポーネントが有効か安全確認
	if (SpawnPoint)
	{
		// C++での Get World Location = Get World Location
		return SpawnPoint->GetComponentLocation();
	}
	// 万が一コンポーネントが見つからなかった場合の保険（Actor自身の座標を返す）
	return GetActorLocation();
}

void AWeaponBase::ChangeWeaponColor(FLinearColor NewColor)
{
	// 作成済みの動的マテリアルに対して色パラメータをセット
	if (WeaponMID)
	{
		WeaponMID->SetVectorParameterValue(FName("color"), NewColor);
	}
}

void AWeaponBase::ChangeWeaponIntensity(float NewIntensity)
{
	// 作成済みの動的マテリアルに対して強度をセット
	if (WeaponMID)
	{
		WeaponMID->SetScalarParameterValue(FName("intensity"), NewIntensity);
	}
}

void AWeaponBase::ChangeWeaponPanner(float NewPanner)
{
	// 作成済みの動的マテリアルに対してパナーをセット
	if (WeaponMID)
	{
		WeaponMID->SetScalarParameterValue(FName("2_panner"), NewPanner);
	}
}

// Called every frame
void AWeaponBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


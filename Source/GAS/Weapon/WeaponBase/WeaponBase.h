

#pragma once

#include "GameplayEffectTypes.h"
#include "Animation/AnimInstance.h"
#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GameFramework/Actor.h"
#include "WeaponBase.generated.h"

// 中に入る構造体(S_MovementProperties 構造体を作成)
USTRUCT(BlueprintType)
struct FSMovementProperties
{
	GENERATED_BODY()
	// --- 変数 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	bool OrientRotationToMovement = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	bool UseControllerDesiredRotation = false;
};

// 中に入る構造体(S_CameraSettings 構造体を作成)
USTRUCT(BlueprintType)
struct FSCameraSettings
{
	GENERATED_BODY()
	// --- 変数 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float FieldOfView = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FVector Offset = FVector::ZeroVector;
};

// メインの構造体 (S_WeaponConfig 構造体を作成)
USTRUCT(BlueprintType)
struct FSWeaponConfig
{
	GENERATED_BODY()
	// --- 変数 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponConfig")
	FName EquippedSocketName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponConfig")
	FName StowedSocketName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponConfig")
	TSubclassOf<UAnimInstance> AnimClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponConfig")
	UAnimMontage* EquipMontage;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponConfig")
	UAnimMontage* UnequipMontage;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponConfig")
	FSMovementProperties MovementProperties;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponConfig")
	bool ShouldOverrideCameraSettings = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponConfig")
	FSCameraSettings CameraSettings;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponConfig")
	bool ShouldShowCrossHair = false;
};



// 前方宣言（ヘッダーのインクルードを減らすため）
class USceneComponent;
class UStaticMeshComponent;
class UEquipmentDataAsset;

// このクラスをBPで継承・型指定できるように設定
UCLASS(Blueprintable, BlueprintType)
class GAS_API AWeaponBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWeaponBase();
	
	// この武器の元になっているアイテムデータ。移動速度ペナルティなどは、ここから読む
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Item")
	TObjectPtr<UEquipmentDataAsset> ItemData;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	// HitScanカスタムイベント
	UFUNCTION(BlueprintCallable, Category = "HitScanTrace")
	void HitScan();
	
	// トレース開始イベント
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void HitScanStart(const FGameplayEffectSpecHandle InEffectSpecHandle);
	
	// トレース終了イベント
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void HitScanEnd();
	
	//何か（発射物）を出す時の位置
	// 値を返すだけなので、状態を変更しない const を付ける
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	FVector GetSpawnPointLocation() const;
	
	
	// 武器コンポーネントやブループリントから参照・設定できるようにする変数
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "WeaponConfig")
	FSWeaponConfig WeaponConfig;
	
	// 色を変更するための関数
	UFUNCTION(BlueprintCallable,Category = "Weapon|Material")
	void ChangeWeaponColor(FLinearColor NewColor);
	
	//武器の強度を調整する関数
	UFUNCTION(BlueprintCallable,Category = "Weapon|Material")
	void ChangeWeaponIntensity (float NewIntensity);
	
	// Panner（パナー）を調整する関数
	UFUNCTION(BlueprintCallable,Category = "Weapon|Material")
	void ChangeWeaponPanner (float NewPanner);

	// 手に持つ消耗品（ポーション等）として使う時にチェック。
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="HeldItem")
	bool IsHeldItem = false;
	
	// 仕舞う動作の後、この秒数で消える（仕舞うモンタージュより少し長くする）
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="HeldItem",meta=(ClampMin = "0.1"))
	float HeldItemDestroyDelay = 0.8f;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// --- コンポーネント ---
	// BPのビューポートで位置調整できるようにVisibleAnywhereを設定
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
	USceneComponent* DefaultSceneRoot;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
	UStaticMeshComponent* WeaponMesh;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
	USceneComponent* TraceStart;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
	USceneComponent* TraceEnd;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
	USceneComponent* SpawnPoint;
	
	// --- 変数 ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HitScanTrace")
	float HitScanRadius = 20.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "HitScanTrace")
	TArray<AActor*> HitActors;
	
	UPROPERTY(EditDefaultsOnly, Category = "HitScanTrace")
	FTimerHandle HitScanTimer;
	
	//ダメージエフェクトを適用する変数
	UPROPERTY(EditDefaultsOnly,Category = "GameplayEffect")
	FGameplayEffectSpecHandle EffectSpecHandle;
	
	//デバフのゲームプレイエフェクトを適用する変数
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly, Category = "GameplayEffect")
	TSubclassOf<UGameplayEffect> DebufftoApply;
	
	// 動的マテリアルインスタンスを保持する変数
	UPROPERTY(BlueprintReadOnly, Category = "Material")
	UMaterialInstanceDynamic* WeaponMID;
	
};

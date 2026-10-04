// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

// This class does not need to be modified.
UINTERFACE(BlueprintType)
class UInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 宝箱・ドア・アイテムなど全く異なるActorでも同じUIロジックで表示出来るようにするインターフェース
 */
class GAS_API IInteractable
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	
	// 「アイテムを取る」「開く」等、Fキーの隣に出すアクション文言
	UFUNCTION(BlueprintNativeEvent,BlueprintCallable,Category="Interaction")
	FText GetInteractionText() const ;
	
	// 「ゆるいズボン」「小さな宝箱」等、対象の名前
	UFUNCTION(BlueprintNativeEvent,BlueprintCallable,Category="Interaction")
	FText GetActorDisplayName() const ;
	
	// レアリティに応じた名前の色
	UFUNCTION(BlueprintNativeEvent,BlueprintCallable,Category="Interaction")
	FLinearColor GetDisplayNameColor() const ;
	
	// 実際にインタラクトを実行する（拾う／開ける等、対象ごとに中身が変わる）
	UFUNCTION(BlueprintNativeEvent,BlueprintCallable,Category="Interaction")
	void OnInteract(AActor* Interactor);
};

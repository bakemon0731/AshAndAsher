// Fill out your copyright notice in the Description page of Project Settings.


#include "RarityLibrary.h"

FLinearColor URarityLibrary::GetRarityColor(EItemRarity Rarity)
{
	switch (Rarity)
	{
	case EItemRarity::Common:    return FLinearColor::White;
	case EItemRarity::Uncommon:  return FLinearColor(0.1f, 0.8f, 0.1f); // 緑
	case EItemRarity::Rare:      return FLinearColor(0.0f, 0.4f, 1.0f); // 青
	case EItemRarity::Epic:      return FLinearColor(0.6f, 0.1f, 0.9f); // 紫
	case EItemRarity::Legendary: return FLinearColor(1.0f, 0.6f, 0.0f); // 金
	case EItemRarity::Artifact:  return FLinearColor(1.0f, 0.0f, 0.0f); // 赤
	default: return FLinearColor::White;
	}
}

FText URarityLibrary::GetRarityText(EItemRarity Rarity)
{
	const UEnum* EnumPtr = StaticEnum<EItemRarity>();
	if (!EnumPtr)
	{
		return FText::GetEmpty();
	}
	// UMETA(DisplayName) で設定した表示名を取得
	return EnumPtr->GetDisplayNameTextByValue(static_cast<int64>(Rarity));
}

FText URarityLibrary::GetEquipmentSlotText(EEquipmentSlot Slot)
{
	const UEnum* EnumPtr = StaticEnum<EEquipmentSlot>();
	if (!EnumPtr)
	{
		return FText::GetEmpty();
	}
	// UMETA(DisplayName) で設定した表示名（「頭」「胴体」など）を取得
	return EnumPtr->GetDisplayNameTextByValue(static_cast<int64>(Slot));
}

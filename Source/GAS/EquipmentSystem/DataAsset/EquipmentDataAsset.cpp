// Fill out your copyright notice in the Description page of Project Settings.


#include "EquipmentDataAsset.h"

FText UEquipmentDataAsset::GetItemTypeText() const
{
	switch (Category)
	{
	case EItemCategory::Consumable: return NSLOCTEXT("item", "TypeUtility","ユーティリティ");
	case EItemCategory::Material: return NSLOCTEXT("Item","TypeMaterial","素材");
	default: break;
	}
	
	if (const UEnum* SlotEnum = StaticEnum<EEquipmentSlot>())
	{
		return SlotEnum->GetDisplayNameTextByValue(static_cast<int64>(Slot));
	}
	return FText::GetEmpty();
}

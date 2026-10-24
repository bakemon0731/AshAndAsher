#pragma once

#include "CoreMinimal.h"
#include "EquipmentType.generated.h"

UENUM(BlueprintType)
enum class EItemCategory : uint8
{
	//装備品
	Equipment UMETA(DisplayName = "Equipment"),
	//消耗品（ポーションなど）
	Consumable UMETA(DisplayName = "Consumable"),
	//敵の素材
	Material UMETA(DisplayName = "Material"),
};

UENUM(BlueprintType)
enum class EEquipmentSlot : uint8
{
	Head        UMETA(DisplayName = "頭"),
	Chest       UMETA(DisplayName = "胴体"),
	Hands       UMETA(DisplayName = "手"),
	Legs        UMETA(DisplayName = "脚"),
	Feet        UMETA(DisplayName = "足"),
	Cloak       UMETA(DisplayName = "マント"),
	Ring        UMETA(DisplayName = "指輪"),
	Pendant		UMETA(DisplayName = "ネックレス"),
	Weapon      UMETA(DisplayName = "武器"),
	None        UMETA(DisplayName = "None")
};
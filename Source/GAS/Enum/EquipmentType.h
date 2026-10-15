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

#pragma once

#include "CoreMinimal.h"
#include "Rarity.generated.h"

UENUM(BlueprintType)
enum class EItemRarity : uint8
{
	Common      UMETA(DisplayName = "コモン"),
	Uncommon    UMETA(DisplayName = "アンコモン"),
	Rare        UMETA(DisplayName = "レア"),
	Epic        UMETA(DisplayName = "エピック"),
	Legendary   UMETA(DisplayName = "レジェンダリー"),
	Artifact	UMETA(DisplayName = "アーティファクト")
};
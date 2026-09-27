// Fill out your copyright notice in the Description page of Project Settings.


#include "AC_SpellComponent.h"
#include "Net/UnrealNetwork.h"
#include "Algo/Sort.h"
#include "GAS/GameplayAbilitySystem/AttributeSets/BasicAttributeSet.h"


// Sets default values for this component's properties
UAC_SpellComponent::UAC_SpellComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	
	SetIsReplicatedByDefault(true);
}

void UAC_SpellComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UAC_SpellComponent, EquippedSpellSlots);
}

void UAC_SpellComponent::InitializeKnownSpells(UAbilitySystemComponent* InASC)
{
	if (!InASC || !InASC->GetOwner() || !InASC->GetOwner()->HasAuthority())
	{
		return;
	}
	
	CachedASC = InASC;
	
	for (USpellDataAsset* Spell : KnownSpells)
	{
		if (!Spell || !Spell->AbilityClass)
		{
			continue;
		}
		
		FGameplayAbilitySpecHandle Handle = CachedASC->GiveAbility(
			FGameplayAbilitySpec(Spell->AbilityClass,1,INDEX_NONE,GetOwner())
			);
			SpellHandleMap.Add(Spell->AbilityClass,Handle);
	}
	EquippedSpellSlots.SetNum(NumSpellSlots);
}

bool UAC_SpellComponent::ActivateSpellIndex(int32 SpellIndex)
{
	if (!CachedASC || !EquippedSpellSlots.IsValidIndex(SpellIndex))
	{
		return false;
	}
	
	USpellDataAsset* Spell = EquippedSpellSlots[SpellIndex];
	if (!Spell || !Spell->AbilityClass)
	{
		// 空スロット
		return false;
	}
	
	if (const FGameplayAbilitySpecHandle* Handle = SpellHandleMap.Find(Spell->AbilityClass))
	{
		return CachedASC->TryActivateAbility(*Handle);
	}
	
	return false;
}

void UAC_SpellComponent::EquipSpellToSlot(int32 SlotIndex, USpellDataAsset* Spell)
{
	Server_EquipSpellToSlot(SlotIndex, Spell);
}

void UAC_SpellComponent::Server_EquipSpellToSlot_Implementation(int32 SlotIndex, USpellDataAsset* Spell)
{
	if (!EquippedSpellSlots.IsValidIndex(SlotIndex))
	{
		return;
	}
	if (Spell && !KnownSpells.Contains(Spell))
	{
		// 未習得の魔法は装備不可
		return;
	}
	
	//SpellCostの記憶容量オーバーなら装備拒否
	if (!CanEquipSpellToSlot(SlotIndex,Spell))
	{
		return;
	}
	
	// 同じ魔法が既に別スロットに入っていたら、そちらを空にする（一意装備の強制）
	if (Spell)
	{
		for (int32 i = 0; i < EquippedSpellSlots.Num(); i++)
		{
			if (i != SlotIndex && EquippedSpellSlots[i] == Spell)
			{
				EquippedSpellSlots[i] = nullptr;
			}
		}
	}
	EquippedSpellSlots[SlotIndex] = Spell;
	// サーバー自身のUIにも反映
	OnRep_EquippedSpellSlots();
}

void UAC_SpellComponent::OnRep_EquippedSpellSlots()
{
	OnEquippedSpellsChanged.Broadcast();
}

USpellDataAsset* UAC_SpellComponent::GetEquippedSpellAtSlot(int32 SlotIndex) const
{
	if (!EquippedSpellSlots.IsValidIndex(SlotIndex))
	{
		return nullptr;
	}
	return EquippedSpellSlots[SlotIndex];
}

bool UAC_SpellComponent::IsSpellEquippedInAnySlot(USpellDataAsset* Spell) const
{
	return Spell && EquippedSpellSlots.Contains(Spell);
}

void UAC_SpellComponent::Server_SetSelectedSpellIndex_Implementation(int32 NewIndex)
{
	SelectedSpellIndex = NewIndex;
}

//装備しているSpellSlotからデータアセットのCost変数を出してトータルで返す
int32 UAC_SpellComponent::GetUsedMemoryCapacity() const
{
	int32 Total = 0;
	for (USpellDataAsset* Spell : EquippedSpellSlots)
	{
		if (Spell)
		{
			Total += Spell->Cost;
		}
	}
	return Total;
}

bool UAC_SpellComponent::CanEquipSpellToSlot(int32 SlotIndex, USpellDataAsset* Spell) const
{
	//もし渡されたSpellがnullptrなら、スロットの枠が空くのでtrue（装備可能）を返す。
	if (!Spell)
	{
		return true;
	}
	
	//有効ではない存在しないスロット番号はfalseを返しす。
	if (!EquippedSpellSlots.IsValidIndex(SlotIndex))
	{
		return false;
	}
	
	//現在のSpellSlotにあるSpellCostの合計を取得。
	int32 UsedWithoutThisSpell = GetUsedMemoryCapacity();
	
	//現在の使用量の計算。（現在のSpellCostの合計　- 古いSpellCost値　= その古いSpellCost値が無いと仮定した使用量）
	if (USpellDataAsset* CurrentInSlot = EquippedSpellSlots[SlotIndex])
	{
		UsedWithoutThisSpell -= CurrentInSlot->Cost;
	}
	
	//新しいSpellCostを足して、最大記憶容量を超えないかチェック
	return (UsedWithoutThisSpell + Spell -> Cost) <= GetMaxMemoryCapacity();
}

//「覚えている魔法リストをコピーして、C++の自動並べ替え機能（Algo::Sort）にコストが小さい順に並び替えたものを返す」という処理
TArray<USpellDataAsset*> UAC_SpellComponent::GetKnownSpellsSortedByCost() const
{
	//KnownSpell変数をSorted変数にコピー。
	TArray<USpellDataAsset*> Sorted = KnownSpells;
	
	Algo::Sort(Sorted,[](const USpellDataAsset* A,const USpellDataAsset* B)
	{
		//比較する魔法データ（AとB）がのどちらかnullptrの場合、クラッシュ防止のため並べ替えない。
		if (!A || !B)
		{
			return false;
		}
		
		// Aのコストが、B のコストよりも「小さい（ < ）」ときに true を返す。（ソートのルール）
		return A -> Cost < B -> Cost;
	});
	
	return Sorted;
}

int32 UAC_SpellComponent::GetUnequippedSpellCountByCost(int32 Cost) const
{
	int32 Count = 0;
	for (USpellDataAsset* Spell : KnownSpells)
	{
		if (Spell && Spell -> Cost == Cost && !EquippedSpellSlots.Contains(Spell))
		{
			Count++;
		}
	}
	return Count;
}

void UAC_SpellComponent::SwapEquippedSpells(int32 SlotIndexA, int32 SlotIndexB)
{
	Server_SwapEquippedSpells(SlotIndexA,SlotIndexB);
}

int32 UAC_SpellComponent::GetMaxMemoryCapacity() const
{
	if (CachedASC)
	{
		if (const UBasicAttributeSet* BasicSet = CachedASC->GetSet<UBasicAttributeSet>())
		{
			return FMath::RoundToInt(BasicSet->GetMaxMemoryCapacity());
		}
	}
	// フォールバック値
	return 15;
}

void UAC_SpellComponent::Server_SwapEquippedSpells_Implementation(int32 SlotIndexA, int32 SlotIndexB)
{
	if (!EquippedSpellSlots.IsValidIndex(SlotIndexA) || !EquippedSpellSlots.IsValidIndex(SlotIndexB))
	{
		return;
	}
	
	int32 UsedExcludingBoth = GetUsedMemoryCapacity();
	if (USpellDataAsset* SpellA = EquippedSpellSlots[SlotIndexA])
	{
		UsedExcludingBoth -= SpellA -> Cost;
	}
	if (USpellDataAsset* SpellB = EquippedSpellSlots[SlotIndexB])
	{
		UsedExcludingBoth -= SpellB -> Cost;
	}
	
	int32 NewCostA = EquippedSpellSlots[SlotIndexB] ? EquippedSpellSlots[SlotIndexB]->Cost : 0;
	int32 NewCostB = EquippedSpellSlots[SlotIndexA] ? EquippedSpellSlots[SlotIndexA]->Cost : 0;
	
	if (UsedExcludingBoth + NewCostA + NewCostB > GetMaxMemoryCapacity())
	{
		return;
	}
	
	EquippedSpellSlots.Swap(SlotIndexA,SlotIndexB);
	OnRep_EquippedSpellSlots();
}


// Called when the game starts
void UAC_SpellComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void UAC_SpellComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                       FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
}


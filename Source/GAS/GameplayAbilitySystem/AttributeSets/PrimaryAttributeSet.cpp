// Fill out your copyright notice in the Description page of Project Settings.


#include "PrimaryAttributeSet.h"
#include "Net/UnrealNetwork.h"

UPrimaryAttributeSet::UPrimaryAttributeSet()
{
	//マクロによって自動生成されるInit関数
	InitStrength(0.f);
	InitKnowledge(0.f);
	InitWillpower(0.f);
	InitAgility(0.f);
	InitVitality(0.f);
}

//変数の同期
void UPrimaryAttributeSet::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION_NOTIFY(UPrimaryAttributeSet,Knowledge,COND_None,REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPrimaryAttributeSet,Willpower,COND_None,REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPrimaryAttributeSet,Agility,COND_None,REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPrimaryAttributeSet,Vitality,COND_None,REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPrimaryAttributeSet,Strength,COND_None,REPNOTIFY_Always);
}



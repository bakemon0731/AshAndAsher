// Fill out your copyright notice in the Description page of Project Settings.


#include "WorldItemActor.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GAS/EquipmentSystem/EquipmentComponent/AC_EquipmentComponent.h"
#include "GAS/GameplayAbilitySystem/Characters/PlayerCharacter/NexusPlayerbase.h"
#include "GAS/BlueprintFunctionLibrary/RarityLibrary.h"
#include "Net/UnrealNetwork.h"


// Sets default values
AWorldItemActor::AWorldItemActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;
	MeshComponent->SetCollisionProfileName(TEXT("Custom"));
	MeshComponent->SetSimulatePhysics(true);
	
	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(RootComponent);
	InteractionSphere->SetSphereRadius(150.0f);
	InteractionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

void AWorldItemActor::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWorldItemActor, ItemData);
}

FText AWorldItemActor::GetInteractionText_Implementation() const
{
	return FText::FromString(TEXT("アイテムを取る"));
}

FText AWorldItemActor::GetActorDisplayName_Implementation() const
{
	return ItemData ? ItemData->DisplayName : FText::GetEmpty();
}

FLinearColor AWorldItemActor::GetDisplayNameColor_Implementation() const
{
	//アイテムデータが無い場合は色を白にする
	if (!ItemData)
	{
		return FLinearColor::White;
	}
	
	// BlueprintFunctionLibrary から共通の色を取得して返す
	return URarityLibrary::GetRarityColor(ItemData->Rarity);
}

void AWorldItemActor::OnInteract_Implementation(AActor* Interactor)
{
	if (ANexusPlayerbase* Character = Cast<ANexusPlayerbase>(Interactor))
	{
		if (Character->EquipmentManagerComponent)
		{
			Character->EquipmentManagerComponent->RequestPickupItem(this);
		}
	}
}


// Called when the game starts or when spawned
void AWorldItemActor::BeginPlay()
{
	Super::BeginPlay();
	OnItemDataUpdated();
	
	InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &AWorldItemActor::OnInteractionSphereBeginOverlap);
	InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &AWorldItemActor::OnInteractionSphereEndOverlap);
}

void AWorldItemActor::OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (ANexusPlayerbase* Character = Cast<ANexusPlayerbase>(OtherActor))
	{
		Character->RegisterNearbyInteractable(this);
		SetHighlighted(true);
	}
}

void AWorldItemActor::OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (ANexusPlayerbase* Character = Cast<ANexusPlayerbase>(OtherActor))
	{
		Character->UnregisterNearbyInteractable(this);
		SetHighlighted(false);
	}
}

void AWorldItemActor::OnRep_ItemData()
{
	OnItemDataUpdated();
}

// Called every frame
void AWorldItemActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}


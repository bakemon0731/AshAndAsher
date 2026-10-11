// Fill out your copyright notice in the Description page of Project Settings.


#include "LootContainerActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "GAS/EquipmentSystem/EquipmentComponent/AC_EquipmentComponent.h"
#include "GAS/EquipmentSystem/DataAsset/EquipmentDataAsset.h"
#include "GAS/GameplayAbilitySystem/Characters/PlayerCharacter/NexusPlayerbase.h"


// Sets default values
ALootContainerActor::ALootContainerActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(RootComponent);
	InteractionSphere->SetSphereRadius(150.f);
	InteractionSphere->SetCollisionProfileName(TEXT("Custom"));
	
	EquipmentComponent = CreateDefaultSubobject<UAC_EquipmentComponent>(TEXT("EquipmentComponent"));
	EquipmentComponent->InventoryGridWidth = 10;
	EquipmentComponent->InventoryGridHeight = 10;
}

void ALootContainerActor::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ALootContainerActor,DisplayName);
}

// Called when the game starts or when spawned
void ALootContainerActor::BeginPlay()
{
	Super::BeginPlay();
	
	InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &ALootContainerActor::OnInteractionSphereBeginOverlap);
	InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &ALootContainerActor::OnInteractionSphereEndOverlap);

	if (HasAuthority())
	{
		EquipmentComponent->OnEquipmentChanged.AddDynamic(this, &ALootContainerActor::HandleContentsChanged);
	}
}

void ALootContainerActor::OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (ANexusPlayerbase* Character = Cast<ANexusPlayerbase>(OtherActor))
	{
		Character->RegisterNearbyInteractable(this);
	}
}

void ALootContainerActor::OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (ANexusPlayerbase* Character = Cast<ANexusPlayerbase>(OtherActor))
	{
		Character->UnregisterNearbyInteractable(this);
	}
}

void ALootContainerActor::HandleContentsChanged()
{
	if (!HasAuthority())
	{
		return;
	}
	
	// 空になったら2秒後に消滅、中身がある間は最大存在時間で消滅
	SetLifeSpan(EquipmentComponent->InventoryItemsInstance.Num() == 0 ? 2.0f : MaxLifeSeconds);
}

void ALootContainerActor::InitializeContents(TArray<UEquipmentDataAsset*>& Items, const FText& InDisplayName)
{
	DisplayName = InDisplayName;
	
	Items.Sort([](const UEquipmentDataAsset& A, const UEquipmentDataAsset& B)
	{
		return (A.GridWidth * A.GridHeight) > (B.GridWidth * B.GridHeight);
	});
	
	for (UEquipmentDataAsset* Item : Items)
	{
		if (!EquipmentComponent->AddItemToInventory(Item))
		{
			UE_LOG(LogTemp, Warning, TEXT("LootContainer is full. Lost item: %s"), *GetNameSafe(Item));
		}
	}
}

// Called every frame
void ALootContainerActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

FText ALootContainerActor::GetInteractionText_Implementation() const
{
	return FText::FromString(TEXT("漁る"));
}

FText ALootContainerActor::GetActorDisplayName_Implementation() const
{
	return DisplayName;
}

FLinearColor ALootContainerActor::GetDisplayNameColor_Implementation() const
{
	return FLinearColor::White;
}

void ALootContainerActor::OnInteract_Implementation(AActor* Interactor)
{
	if (ANexusPlayerbase* PlayerChar = Cast<ANexusPlayerbase>(Interactor))
	{
		// 敵の死体と同じ。W_LootScreenがコンポーネントを探して表示する
		PlayerChar->OpenLootScreen(this);
	}
}







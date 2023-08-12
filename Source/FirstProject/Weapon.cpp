// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"
#include "Main.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Sound/SoundCue.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/BoxComponent.h"
#include "Enemy.h"
#include "Engine/SkeletalMeshSocket.h"
#include"Sound/SoundCue.h"
AWeapon::AWeapon()
{
	SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>
		(TEXT("SkeletalMesh"));
	SkeletalMesh->SetupAttachment(RootComponent);

	bWeaponParticles = false;

	OnEquipSound = nullptr;

	WeaponState = EWeaponState::EWS_Pickup;

	CombatCollision = CreateDefaultSubobject<UBoxComponent>
		(TEXT("CombatCollision"));
	CombatCollision->SetupAttachment(RootComponent);

	Damage = 25.0;
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	Damage = 25.0f;

	CombatCollision->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::CombatOnOverlapBegin);
	CombatCollision->OnComponentEndOverlap.AddDynamic(this, &AWeapon::CombatOnOverlapEnd);

	CombatCollision->SetCollisionEnabled(
		ECollisionEnabled::NoCollision);
	CombatCollision->SetCollisionObjectType(
		ECollisionChannel::ECC_WorldDynamic);
	CombatCollision->SetCollisionResponseToAllChannels(
		ECollisionResponse::ECR_Ignore);
	CombatCollision->SetCollisionResponseToChannel(
		ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
}

void AWeapon::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnOverlapBegin(OverlappedComponent,
		OtherActor, OtherComp, OtherBodyIndex, 
		bFromSweep, SweepResult);

	if (WeaponState == EWeaponState::EWS_Pickup && OtherActor)
	{
	AMain* Main = Cast<AMain>(OtherActor);
		if (Main) 
		{
			Main->SetActiveOverlappingItem(this);
		}
	}


}

void AWeapon::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	Super::OnOverlapEnd(OverlappedComponent,
		OtherActor, OtherComp, OtherBodyIndex);
	UE_LOG(LogTemp, Warning, TEXT("APickup Overlap End"));

	if (OtherActor)
	{
		AMain* Main = Cast<AMain>(OtherActor);
		if (Main)
		{
			Main->SetActiveOverlappingItem(nullptr);
		}
	}

}

void AWeapon::Equip(AMain* Character)
{
	if (Character) 
	{
		SetInstigator(Character->GetController());

		SkeletalMesh->SetCollisionResponseToChannel
		(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
		SkeletalMesh->SetCollisionResponseToChannel
		(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);

		SkeletalMesh->SetSimulatePhysics(false);

		const USkeletalMeshSocket* RightHandSocket =
			Character->GetMesh()->GetSocketByName("RightHandSocket");

		if (RightHandSocket) 
		{
			RightHandSocket->AttachActor(this, Character->GetMesh());
			bRotate = false;

			if(Character->GetEquippedWeapon())
			Character->GetEquippedWeapon()->Destroy();

			Character->SetEquippedWeapon(this);
			Character->SetActiveOverlappingItem(nullptr);
		}
		if (OnEquipSound) UGameplayStatics::PlaySound2D(this, OnEquipSound);

		if (!bWeaponParticles)
		{
			IdelParticleComponent->Deactivate();
		}
	}
}

void AWeapon::CombatOnOverlapBegin(UPrimitiveComponent* OverlappedComponent, 
	AActor* OtherActor, UPrimitiveComponent* OtherComp, 
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor) 
	{
		AEnemy* Enemy = Cast<AEnemy>(OtherActor);
		if (Enemy && Enemy->HitParticle) 
		{

			const USkeletalMeshSocket* WeaponSocket =
				SkeletalMesh->GetSocketByName("WeaponSocket");

			if (WeaponSocket) 
			{
				FVector SocketLocation =
					WeaponSocket->GetSocketLocation(SkeletalMesh);

				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(),
					Enemy->HitParticle, SocketLocation, FRotator(0.0f),
					false);


			}
			if (Enemy->HitSound) 
			{
				UGameplayStatics::PlaySound2D(this, Enemy->HitSound);
			}
			if (DamageTypeClass) 
			{
				UGameplayStatics::ApplyDamage(Enemy, Damage, WeaponInstigator, 
					this, DamageTypeClass);
			}
		}
	}
}

void AWeapon::CombatOnOverlapEnd(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{

}

void AWeapon::ActivateCollision()
{
	CombatCollision->SetCollisionEnabled(
		ECollisionEnabled::QueryOnly);

}

void AWeapon::DeActivateCollision()
{
	CombatCollision->SetCollisionEnabled(
		ECollisionEnabled::NoCollision);
}

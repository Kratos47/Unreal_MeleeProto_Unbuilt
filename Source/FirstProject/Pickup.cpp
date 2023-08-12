// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickup.h"
#include "Main.h"
#include"Sound/SoundCue.h"
#include"Kismet/GameplayStatics.h"
//#include"Particles/ParticleSystemComponent.h"
#include"Engine/World.h"
APickup::APickup()
{
	//s->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic
	//	, ECollisionResponse::ECR_Ignore);
}

void APickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APickup::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnOverlapBegin(OverlappedComponent,
		OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);


	if (OtherActor)
	{
		AMain* Main = Cast<AMain>(OtherActor);
		if (Main)
		{
			OnPickupBP(Main);
			Main->PickupLocations.Add(GetActorLocation());
			if (OverlapParticles != nullptr)
			{
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(),
					OverlapParticles, GetActorLocation(), FRotator(0.0f), true);
			}
			if (OverlapSound != nullptr)
			{
				UGameplayStatics::PlaySound2D(this, OverlapSound);
			}

			Destroy();
		}
	}
}

void APickup::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	Super::OnOverlapEnd(OverlappedComponent,
		OtherActor, OtherComp, OtherBodyIndex);
	UE_LOG(LogTemp, Warning, TEXT("APickup Overlap End"));
}
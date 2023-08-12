// Fill out your copyright notice in the Description page of Project Settings.


#include "Explosive.h"
#include "Main.h"
#include"Sound/SoundCue.h"
#include"Kismet/GameplayStatics.h"
#include "Kismet/GameplayStatics.h"
#include"Engine/World.h"
#include "Enemy.h"

AExplosive::AExplosive() 
{
	Damage = 15.0f;
}

void AExplosive::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AExplosive::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnOverlapBegin(OverlappedComponent,
		OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	if(OtherActor)
	{
		AMain* Main = Cast<AMain>(OtherActor);
		AEnemy* Enemy = Cast<AEnemy>(OtherActor);
		if (Main || Enemy) 
		{
			if (OverlapParticles != nullptr)
			{
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(),
					OverlapParticles, GetActorLocation(), FRotator(0.0f), true);
			}
			if (OverlapSound != nullptr)
			{
				UGameplayStatics::PlaySound2D(this, OverlapSound);
			}
			//Main->DecrementHealth(Damage);
			if (DamageTypeClass)
			{
				UGameplayStatics::ApplyDamage(OtherActor, Damage, nullptr, this,
				DamageTypeClass);
			}
			Destroy();
		}
	}
}
 
void AExplosive::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	Super::OnOverlapEnd(OverlappedComponent,
		OtherActor, OtherComp, OtherBodyIndex);
	UE_LOG(LogTemp, Warning, TEXT("AExplosive Overlap End"));
}

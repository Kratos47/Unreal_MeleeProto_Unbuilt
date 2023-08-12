// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy.h"
#include "Components/SphereComponent.h"
#include "AIController.h"
#include "NavigationData.h"
#include "Main.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/AssertionMacros.h."
#include "Components/BoxComponent.h"
#include "Main.h"
#include"Sound/SoundCue.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Animation/AnimInstance.h"
#include "TimerManager.h"
#include"Components/CapsuleComponent.h"
#include "MainPlayerController.h"
// Sets default values
AEnemy::AEnemy()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	AgroSphere = CreateDefaultSubobject<USphereComponent>
		(TEXT("AgroSphere"));
	AgroSphere->SetupAttachment(RootComponent);

	AgroSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic
	,ECollisionResponse::ECR_Ignore);

	AgroSphere->InitSphereRadius(600.0f);


	CombatSphere = CreateDefaultSubobject<USphereComponent>
		(TEXT("CombatSphere"));

	CombatSphere->SetupAttachment(RootComponent);

	CombatSphere->InitSphereRadius(75.0f);

	CombatBoxCollision = CreateDefaultSubobject<UBoxComponent>
		(TEXT("CombatBoxCollision"));
	CombatBoxCollision->SetupAttachment(GetMesh(),
		FName("EnemySocket"));

	AIController = nullptr;

	bOverlappingCombatSphere = false;

	bEnemyAttacking = false;

	bHasValidTarget = false;

	AttackMinTime = 0.5;

	AttackMaxTime = 1.5;

	EnemyMovementStatus = EEnemyMovementStatus::EMS_Idel;

	Health = 75.0f;
	MaxHealth = 100.0f;
	Damage = 10.0f;

	DeathDelay = 3.0f;

	AcceptanceRadius = 100;
}

// Called when the game starts or when spawned
void AEnemy::BeginPlay()
{
	Super::BeginPlay();

	AgroSphere->InitSphereRadius(300.0f);

	CombatSphere->InitSphereRadius(45.0f);

	AIController = Cast<AAIController>(GetController());

	AgroSphere->OnComponentBeginOverlap.AddDynamic(
		this, &AEnemy::AgroSphereOnOverlapBegin);

	AgroSphere->OnComponentEndOverlap.AddDynamic(
		this, &AEnemy::AgroSphereOnOverlapEnd);

	CombatSphere->OnComponentBeginOverlap.AddDynamic(
		this, &AEnemy::CombatSphereOnOverlapBegin);

	CombatSphere->OnComponentEndOverlap.AddDynamic(
		this, &AEnemy::CombatSphereOnOverlapEnd);

	CombatBoxCollision->OnComponentBeginOverlap.AddDynamic(
		this, &AEnemy::CombatOnOverlapBegin);

	CombatBoxCollision->OnComponentEndOverlap.AddDynamic(
		this, &AEnemy::CombatOnOverlapEnd);

	CombatBoxCollision->AttachToComponent(GetMesh(),
		FAttachmentTransformRules::SnapToTargetIncludingScale,
		FName("EnemySocket"));

	CombatBoxCollision->SetCollisionEnabled(
		ECollisionEnabled::NoCollision);
	CombatBoxCollision->SetCollisionObjectType(
		ECollisionChannel::ECC_WorldDynamic);
	CombatBoxCollision->SetCollisionResponseToAllChannels(
		ECollisionResponse::ECR_Ignore);
	CombatBoxCollision->SetCollisionResponseToChannel(
		ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);

	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera,
		ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera,
		ECollisionResponse::ECR_Ignore);
}

// Called every frame
void AEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (EnemyMovementStatus != EEnemyMovementStatus::EMS_Attacking)
	{
		this->GetActorForwardVector();
	}
}

// Called to bind functionality to input
void AEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AEnemy::AgroSphereOnOverlapBegin(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && Alive())
	{
		UE_LOG(LogTemp, Warning, TEXT("AgroSphereOnOverlapBegin()"));
		AMain* Main = Cast<AMain>(OtherActor);
		if (Main)
		{
			MoveToTarget(Main);
		}
	}
}

void AEnemy::AgroSphereOnOverlapEnd(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("AgroSphereOnOverlapEnd()"));
		AMain* Main = Cast<AMain>(OtherActor);
		if (Main)
		{
			bHasValidTarget = false;
			if (Main->CombatTarget == this)
			{
				Main->SetCombatTarget(nullptr);
				Main->SetHasCombatTarget(false);
			}

			Main->UpdateCombatTarget();

			SetEnemyMovementStatus(EEnemyMovementStatus::EMS_Idel);
			if (AIController)
			{
				AIController->StopMovement();
			}
		}
	}
}

void AEnemy::CombatSphereOnOverlapBegin(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && Alive())
	{
		AMain* Main = Cast<AMain>(OtherActor);
		if (Main)
		{
			bHasValidTarget = true;

			Main->SetCombatTarget(this);
			Main->SetHasCombatTarget(true);
			Main->UpdateCombatTarget();
			CombatTarget = Main;
			bOverlappingCombatSphere = true;

			float AttackTime = FMath::FRandRange(AttackMinTime, AttackMaxTime);
			GetWorldTimerManager().SetTimer(AttackTimer, this, &AEnemy::Attack, AttackTime);
		}
	}

}

void AEnemy::CombatSphereOnOverlapEnd(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor && OtherComp)
	{
		AMain* Main = Cast<AMain>(OtherActor);
		if (Main)
		{
			bOverlappingCombatSphere = false;
			MoveToTarget(Main);
			CombatTarget = nullptr;
			
			if (Main->CombatTarget == this) 
			{
				Main->SetCombatTarget(nullptr);
				Main->SetHasCombatTarget(false);
				Main->UpdateCombatTarget();
			}
			if (Main->MainPlayerController)
			{
				USkeletalMeshComponent* MainMesh = Cast<USkeletalMeshComponent>(OtherComp);

				if (MainMesh) 
				{
					Main->MainPlayerController->RemoveEnemyHealthBar(); 
				}
			}

			GetWorldTimerManager().ClearTimer(AttackTimer);
		}
	}

}

void AEnemy::MoveToTarget(class AMain* Target)
{

	UE_LOG(LogTemp, Warning, TEXT("MoveToTarget()"));

	SetEnemyMovementStatus(EEnemyMovementStatus::EMS_MoveToTarget);

	if (AIController)
	{
		FAIMoveRequest AIMoveRequest;

		AIMoveRequest.SetAcceptanceRadius(AcceptanceRadius);
		AIMoveRequest.SetGoalActor(Target);

		FNavPathSharedPtr NavPath;

		AIController->MoveTo(AIMoveRequest, &NavPath);

		if (NavPath)
		{
			TArray<FNavPathPoint> PathPoints = NavPath->GetPathPoints();

			for (int32 i = 0; i < PathPoints.Num(); i++)
			{
				UKismetSystemLibrary::DrawDebugSphere(this,
					PathPoints[i].Location, 25.0f, 8.0f,
					FLinearColor::Red, 10.0f, 1.25f);
			}
		}

	}

}

void AEnemy::CombatOnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor)
	{
		AMain* Main = Cast<AMain>(OtherActor);
		if (Main && Main->HitParticle)
		{

			const USkeletalMeshSocket* TipSocket =
				GetMesh()->GetSocketByName("TipSocket");

			if (TipSocket)
			{
				FVector SocketLocation =
					TipSocket->GetSocketLocation(GetMesh());

				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(),
					Main->HitParticle, SocketLocation, FRotator(0.0f),
					false);


			}
			if (Main->HitSound)
			{
				UGameplayStatics::PlaySound2D(this, Main->HitSound);
			}
			if (DamageTypeClass)
			{
				UGameplayStatics::ApplyDamage(Main, Damage, AIController, this,
					DamageTypeClass);
			}
		}
	}
}

void AEnemy::CombatOnOverlapEnd(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
}

void AEnemy::ActivateEnemyCollision()
{
	CombatBoxCollision->SetCollisionEnabled(
		ECollisionEnabled::QueryOnly);

	if (SwingSound)
	{
		UGameplayStatics::PlaySound2D(this, SwingSound);
	}
}

void AEnemy::DeActivateEnemyCollision()
{
	CombatBoxCollision->SetCollisionEnabled(
		ECollisionEnabled::NoCollision);
}

void AEnemy::Attack()
{

	if (Alive() && bHasValidTarget)
	{
		if (AIController)
		{
			AIController->StopMovement();
			SetEnemyMovementStatus(EEnemyMovementStatus::EMS_Attacking);
		}

		if (!bEnemyAttacking)
		{
			bEnemyAttacking = true;

			UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

			if (AnimInstance && EnemyCombatMontage)
			{

				AnimInstance->Montage_Play(EnemyCombatMontage, 1.35f);
				AnimInstance->Montage_JumpToSection(FName("Attack"), EnemyCombatMontage);

			}
		}
	}
}

void AEnemy::AttackEnd()
{
	bEnemyAttacking = false;

	//if (CombatTarget)
	//{
		if (bOverlappingCombatSphere )
			//&&CombatTarget->MovementStatus != EMovementStatus::EMS_Dead)
		{
			float AttackTime = FMath::FRandRange(AttackMinTime, AttackMaxTime);
			GetWorldTimerManager().SetTimer(AttackTimer, this, &AEnemy::Attack, AttackTime);
		}
	//}

	
}

float AEnemy::TakeDamage(float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	if (Health - DamageAmount <= 0.0f)
	{
		Health -= DamageAmount;
		Die(DamageCauser);
	}
	else
	{
		Health -= DamageAmount;
	}

	return DamageAmount;
}

void AEnemy::Die(AActor* Causer)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (AnimInstance && EnemyCombatMontage)
	{
		AnimInstance->Montage_Play(EnemyCombatMontage, 1.0f);
		AnimInstance->Montage_JumpToSection(FName("Death"), EnemyCombatMontage);
	}

	SetEnemyMovementStatus(EEnemyMovementStatus::EMS_Dead);

	CombatBoxCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AgroSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CombatSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	AMain* Main = Cast<AMain>(Causer);

	if (Main) 
	{
		Main->UpdateCombatTarget();
	}
}

void AEnemy::DeathEnd()
{
	GetMesh()->bPauseAnims = true;
	GetMesh()->bNoSkeletonUpdate = true;

	GetWorldTimerManager().SetTimer(DeathTimer, this,
		&AEnemy::Disappear, DeathDelay);
}

bool AEnemy::Alive()
{
	return GetEnemyMovementStatus() != EEnemyMovementStatus::EMS_Dead;
}

void AEnemy::Disappear()
{
	Destroy();
}



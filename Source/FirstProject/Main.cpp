// Fill out your copyright notice in the Description page of Project Settings.


#include "Main.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Weapon.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include"Sound/SoundCue.h"
#include"Enemy.h"
#include "MainPlayerController.h"
#include "FirstSaveGame.h"
#include "ItemStorage.h"
// Sets default values
AMain::AMain()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create Camera Boom (pulls towards the player if there's a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->TargetArmLength = 600.0f;// Camera follows at this distance
	CameraBoom->bUsePawnControlRotation = true; // Rotate arm based on controller


	//Set size for collision capsule
	GetCapsuleComponent()->SetCapsuleSize(50.0f, 105.0f);

	// Create Follow Camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

	// Attach the camera to the end of the boom and let the boom adjust to match
	// the controller orientation
	FollowCamera->bUsePawnControlRotation = false;

	//Set our turn rates for inputs
	BaseTurnRate = 65.0f;
	BaseLookUpRate = 65.0f;

	// Don't rotate when the controller rotates
	// Let that just affect the camera.
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	InterpSpeed = 15.0f;
	bInterpToEnemy = false;
	bHasCombatTarget = false;

	bMovingForward = false;

	bMovingRight = false;

	//Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input..
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 840.0f, 0.0f);// ... at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 650.f;
	GetCharacterMovement()->AirControl = 0.2f;


	MaxHealth = 100.0f;

	Health = 65.0f;

	MaxStamina = 150.0f;

	Stamina = 120.0f;

	Coins = 0;

	RunningSpeed = 650.0f;

	SprintingSpeed = 950.0f;

	bShiftKeyDown = false;
	bLMBDown = false;
	bESCDown = false;
	bAttacking = false;

	//Initialize Enums
	MovementStatus = EMovementStatus::EMS_Normal;
	StaminaStatus = EStaminaStatus::ESS_Normal;

	StaminaDrainRate = 25.0f;

	MinSprintStamina = 50.0f;
}

void AMain::ShowPickupLocations()
{
	//for (int32 i = 0; i < PickupLocations.Num(); i++) 
	//{
	//	UKismetSystemLibrary::DrawDebugSphere(this,
	//		PickupLocations[i], 25.0f, 8.0f,
	//		FLinearColor::Green, 10.0f, 0.25f);
	//}

	for (FVector Location : PickupLocations)
	{
		UKismetSystemLibrary::DrawDebugSphere(this,
			Location, 25.0f, 8.0f,
			FLinearColor::Green, 10.0f, 0.25f);
	}
}

FRotator AMain::GetLookAtRotationYaw(FVector Target)
{
	FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation
	(GetActorLocation(), Target);

	FRotator LookAtRotationYaw(0.0f, LookAtRotation.Yaw, 0.0f);

	return LookAtRotationYaw;
}

void AMain::SetMovementStatus(EMovementStatus Status)
{
	MovementStatus = Status;

	if (MovementStatus == EMovementStatus::EMS_Sprinting)
	{
		GetCharacterMovement()->MaxWalkSpeed = SprintingSpeed;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = RunningSpeed;
	}
}

void AMain::ShiftKeyDown()
{
	bShiftKeyDown = true;
}

void AMain::ShiftKeyUp()
{
	bShiftKeyDown = false;
}

void AMain::DecrementHealth(float Amount)
{


}

float AMain::TakeDamage(float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	if (Health - DamageAmount <= 0.0f)
	{
		Health = 0;
		Die();
		if (DamageCauser)
		{
			AEnemy* Enemy = Cast<AEnemy>(DamageCauser);

			if (Enemy)
			{
				Enemy->bHasValidTarget = false;
			}
		}
	}
	else
	{
		Health -= DamageAmount;
	}

	return DamageAmount;
}

void AMain::IncrementCoins(int32 CoinCount)
{
	Coins += CoinCount;
}

void AMain::IncrementHealth(float Amount)
{
	if (Health + Amount >= MaxHealth)
	{
		Health = MaxHealth;
	}
	else
	{
		Health += Amount;
	}
}

void AMain::Die()
{
	if (MovementStatus == EMovementStatus::EMS_Dead) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (AnimInstance && CombatMontage)
	{
		AnimInstance->Montage_Play(CombatMontage, 1.0f);
		AnimInstance->Montage_JumpToSection(FName("Death"));
	}
	SetMovementStatus(EMovementStatus::EMS_Dead);
}

void AMain::DeathEnd()
{
	GetMesh()->bPauseAnims = true;
	GetMesh()->bNoSkeletonUpdate = true;
}

void AMain::UpdateCombatTarget()
{
	TArray<AActor*> OverlappingActors;

	GetOverlappingActors(OverlappingActors, EnemyFilter);

	int NumElements = OverlappingActors.Num();

	if (NumElements == 0)
	{
		if (MainPlayerController)
		{
			MainPlayerController->RemoveEnemyHealthBar();
		}
		return;
	}

	AEnemy* ClosestEnemy = Cast<AEnemy>(OverlappingActors[0]);

	if (ClosestEnemy)
	{
		FVector Location = GetActorLocation();
		float MinDistance = (ClosestEnemy->GetActorLocation() - Location).Size();
		float ClosestActorIndex = 0;

		for (int i = 1; i < NumElements - 1; i++)
		{
			int DistanceToActor = (OverlappingActors[i]->GetActorLocation()
				- Location).Size();

			if (DistanceToActor < MinDistance)
			{
				MinDistance = DistanceToActor;
				ClosestActorIndex = i;
			}
		}

		ClosestEnemy = Cast<AEnemy>(OverlappingActors[ClosestActorIndex]);
	}
	if (MainPlayerController)
	{
		MainPlayerController->DisplayEnemyHealthBar();
	}
	SetCombatTarget(ClosestEnemy);
	bHasCombatTarget = true;
}

void AMain::SwitchLevel(FName LevelName)
{
	UWorld* World = GetWorld();
	if (World) 
	{
		FString CurrentLevel = World->GetMapName();
		CurrentLevel.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);
		FName CurrentLevelName(*CurrentLevel);

	if (CurrentLevelName != LevelName) 
		{
			UGameplayStatics::OpenLevel(World, LevelName);
		}
	}
}

void AMain::SaveGame()
{
	UFirstSaveGame* SaveGameInstance = Cast<UFirstSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UFirstSaveGame::StaticClass()));

	SaveGameInstance->CharacterStats.Health = Health;
	SaveGameInstance->CharacterStats.MaxHealth = MaxHealth;
	SaveGameInstance->CharacterStats.Stamina = Stamina;
	SaveGameInstance->CharacterStats.MaxStamina = MaxStamina;
	SaveGameInstance->CharacterStats.Coins = Coins;

	SaveGameInstance->CharacterStats.Location = GetActorLocation();
	SaveGameInstance->CharacterStats.Rotation = GetActorRotation();
	FString MapName = GetWorld()->GetMapName();
	MapName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);
	SaveGameInstance->CharacterStats.LevelName = MapName;

	if (EquippedWeapon) 
	{
		SaveGameInstance->CharacterStats.WeaponName = EquippedWeapon->Name;
	}

	UGameplayStatics::SaveGameToSlot(SaveGameInstance, 
		SaveGameInstance->PlayerName,
		SaveGameInstance->UserIndex);
}

void AMain::RemoveDuplicateWeapons(AItemStorage* Weapons, FString WeaponName)
{
	if (Weapons->WeaponMap.Contains(WeaponName))
	{
		//Remove the duplicate weapon
		TSubclassOf<AWeapon> ClassToFind;
		ClassToFind = AWeapon::StaticClass();
		TArray<AActor*> FoundWeapons;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ClassToFind, FoundWeapons);

		//Remove the duplicate weapon
		for (int32 i = 0; i < FoundWeapons.Num(); i++)
		{
			AWeapon* CurrWeapon = Cast<AWeapon>(FoundWeapons[i]);
			if (CurrWeapon->Name == WeaponName)
			{
				FoundWeapons[i]->Destroy();
				break;
			}
		}
	}
}


void AMain::LoadGame(bool SetPosition)
{
	UFirstSaveGame* loadGameInstance = Cast<UFirstSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UFirstSaveGame::StaticClass()));

	loadGameInstance = Cast<UFirstSaveGame>(UGameplayStatics::LoadGameFromSlot
	(loadGameInstance->PlayerName,
		loadGameInstance->UserIndex));

	Health = loadGameInstance->CharacterStats.Health;
	MaxHealth = loadGameInstance->CharacterStats.MaxHealth;
	Stamina = loadGameInstance->CharacterStats.Stamina;
	MaxStamina = loadGameInstance->CharacterStats.MaxStamina;
	Coins = loadGameInstance->CharacterStats.Coins;

	if (WeaponStorage) 
	{
		AItemStorage* Weapons = GetWorld()->SpawnActor<AItemStorage>(WeaponStorage);

		if (Weapons) 
		{
			FString WeaponName = loadGameInstance->CharacterStats.WeaponName;

			if (WeaponName != TEXT("")) 
			{
				AWeapon* WeaponToEquip = GetWorld()->SpawnActor<AWeapon>
					(Weapons->WeaponMap[WeaponName]);

				//RemoveDuplicateWeapons(Weapons, WeaponName);
				if (WeaponToEquip) WeaponToEquip->Equip(this);
			}
			
		}
	}
		

	if (SetPosition) 
	{
		SetActorLocation(loadGameInstance->CharacterStats.Location);
		SetActorRotation(loadGameInstance->CharacterStats.Rotation);
	}

	SetMovementStatus(EMovementStatus::EMS_Normal);

	GetMesh()->bPauseAnims = false;
	GetMesh()->bNoSkeletonUpdate = false;

	if (loadGameInstance->CharacterStats.LevelName != TEXT("")) 
	{
		FName LevelName(*loadGameInstance->CharacterStats.LevelName);

		SwitchLevel(LevelName);
	}
}

void AMain::LoadGameNoSwitch()
{
	UFirstSaveGame* loadGameInstance = Cast<UFirstSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UFirstSaveGame::StaticClass()));

	loadGameInstance = Cast<UFirstSaveGame>(UGameplayStatics::LoadGameFromSlot
	(loadGameInstance->PlayerName,
		loadGameInstance->UserIndex));

	Health = loadGameInstance->CharacterStats.Health;
	MaxHealth = loadGameInstance->CharacterStats.MaxHealth;
	Stamina = loadGameInstance->CharacterStats.Stamina;
	MaxStamina = loadGameInstance->CharacterStats.MaxStamina;
	Coins = loadGameInstance->CharacterStats.Coins;

	if (WeaponStorage)
	{
		AItemStorage* Weapons = GetWorld()->SpawnActor<AItemStorage>(WeaponStorage);

		if (Weapons)
		{
			FString WeaponName = loadGameInstance->CharacterStats.WeaponName;

			if (WeaponName != TEXT(""))
			{
				AWeapon* WeaponToEquip = GetWorld()->SpawnActor<AWeapon>
					(Weapons->WeaponMap[WeaponName]);

				WeaponToEquip->Equip(this);
			}
		}
	}

	SetMovementStatus(EMovementStatus::EMS_Normal);

	GetMesh()->bPauseAnims = false;
	GetMesh()->bNoSkeletonUpdate = false;
}

// Called when the game starts or when spawned
void AMain::BeginPlay()
{
	Super::BeginPlay();

	MainPlayerController = Cast<AMainPlayerController>(GetController());

	FString Map = GetWorld()->GetMapName();
	Map.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);

	if (Map != "SunTemple") 
	{
		LoadGameNoSwitch();

		if (MainPlayerController) MainPlayerController->GameModeOnly();
	}
}

void AMain::NotSprintInPlace()
{
	//Check to prevent sprinting in place
	if (bMovingForward == true || bMovingRight == true)
	{
		SetMovementStatus(EMovementStatus::EMS_Sprinting);
	}
	else
	{
		SetMovementStatus(EMovementStatus::EMS_Normal);
	}

}

// Called every frame
void AMain::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MovementStatus == EMovementStatus::EMS_Dead) return;

	float DeltaStamina = StaminaDrainRate * DeltaTime;

	switch (StaminaStatus)
	{
	case EStaminaStatus::ESS_Normal:
		if (bShiftKeyDown)
		{
			if (bMovingForward == true || bMovingRight == true)
			{
				if (Stamina - DeltaStamina <= MinSprintStamina)
				{
					SetStaminaStatus(EStaminaStatus::ESS_BelowMinimum);
					Stamina -= DeltaStamina;
				}
				else
				{
					Stamina -= DeltaStamina;
				}
			}
			//Check to prevent sprinting in place
			NotSprintInPlace();
		}
		else //Shift Key UP
		{
			if (Stamina + DeltaStamina >= MaxStamina)
			{
				Stamina = MaxStamina;
			}
			else
			{
				Stamina += DeltaStamina;
			}
			SetMovementStatus(EMovementStatus::EMS_Normal);
		}
		break;
	case EStaminaStatus::ESS_BelowMinimum:
		if (bShiftKeyDown)
		{
			if (Stamina - DeltaStamina <= 0.0f)
			{
				SetStaminaStatus(EStaminaStatus::ESS_Exhausted);

				Stamina = 0;

				SetMovementStatus(EMovementStatus::EMS_Normal);
			}
			else
			{
				Stamina -= DeltaStamina;
				//Check to prevent sprinting in place
				NotSprintInPlace();
			}
		}
		else //Shift Key UP
		{
			if (Stamina + DeltaStamina >= MinSprintStamina)
			{
				SetStaminaStatus(EStaminaStatus::ESS_Normal);
				Stamina += DeltaStamina;
			}
			else
			{
				Stamina += DeltaStamina;
			}
			SetMovementStatus(EMovementStatus::EMS_Normal);
		}
		break;
	case EStaminaStatus::ESS_Exhausted:
		if (bShiftKeyDown)
		{
			Stamina = 0.0f;
		}
		else //Shift Key UP
		{
			SetStaminaStatus(EStaminaStatus::ESS_ExhaustedRecovering);
			Stamina += DeltaStamina;
		}
		SetMovementStatus(EMovementStatus::EMS_Normal);
		break;
	case EStaminaStatus::ESS_ExhaustedRecovering:
		if (Stamina + DeltaStamina >= MinSprintStamina)
		{
			SetStaminaStatus(EStaminaStatus::ESS_Normal);
			Stamina += DeltaStamina;
		}
		else
		{
			Stamina += DeltaStamina;
		}
		SetMovementStatus(EMovementStatus::EMS_Normal);
		break;
	default:
		break;
	}

	if (bInterpToEnemy && CombatTarget)
	{
		FRotator LookAtYaw = GetLookAtRotationYaw(CombatTarget->GetActorLocation());

		FRotator InterpRoataion = FMath::RInterpTo
		(GetActorRotation(), LookAtYaw, DeltaTime, InterpSpeed);

		SetActorRotation(InterpRoataion);
	}

	if (CombatTarget)
	{
		CombatTargetLocation = CombatTarget->GetActorLocation();
		if (MainPlayerController)
		{
			MainPlayerController->EnemyLocation = CombatTargetLocation;
		}
	}

}

// Called to bind functionality to input
void AMain::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AMain::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &AMain::ShiftKeyDown);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &AMain::ShiftKeyUp);
	
	PlayerInputComponent->BindAction("ESC", IE_Pressed, this, &AMain::ESCDown);
	PlayerInputComponent->BindAction("ESC", IE_Released, this, &AMain::ESCUp);

	PlayerInputComponent->BindAction("LMB", IE_Pressed, this, &AMain::LMBDown);
	PlayerInputComponent->BindAction("LMB", IE_Released, this, &AMain::LMBUp);

	PlayerInputComponent->BindAxis("MoveForward", this, &AMain::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMain::MoveRight);

	PlayerInputComponent->BindAxis("Turn", this, &AMain::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &AMain::LookUp);

	PlayerInputComponent->BindAxis("TurnRate", this, &AMain::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AMain::LookUpAtRate);

}

bool AMain::CanMove(float Value)
{

	if (MainPlayerController)
		return (Value != 0.0f) && (!bAttacking) &&
		MovementStatus != EMovementStatus::EMS_Dead
		&& !MainPlayerController->bPauseMenuVisible;
	else return false;
}


void AMain::MoveForward(float Value)
{
	bMovingForward = false;
	if (CanMove(Value))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0.0f, Rotation.Yaw, 0.0f);

		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);

		bMovingForward = true;
	}
}

void AMain::MoveRight(float Value)
{
	bMovingRight = false;
	if (CanMove(Value))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0.0f, Rotation.Yaw, 0.0f);

		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(Direction, Value);

		bMovingRight = true;
	}
}

void AMain::Turn(float Value)
{
	if (CanMove(Value))
	{
		AddControllerYawInput(Value);
	}
}

void AMain::LookUp(float Value)
{
	if (CanMove(Value))
	{
		AddControllerPitchInput(Value);
	}
}


void AMain::TurnAtRate(float Rate)
{
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AMain::LookUpAtRate(float Rate)
{
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AMain::LMBUp()
{
	bLMBDown = false;
}

void AMain::LMBDown()
{
	bLMBDown = true;

	if (MovementStatus == EMovementStatus::EMS_Dead) return;

	if (MainPlayerController) if (MainPlayerController->bPauseMenuVisible) return;

	if (ActiveOverlappingItem)
	{
		AWeapon* Weapon = Cast<AWeapon>(ActiveOverlappingItem);
		if (Weapon)
		{
			Weapon->Equip(this);
			SetActiveOverlappingItem(nullptr);
		}
	}
	else if (EquippedWeapon)
	{
		Attack();
	}
}

void AMain::ESCUp()
{
	bESCDown = false;
}

void AMain::ESCDown()
{
	bESCDown = true;

	if (MainPlayerController) 
	{
		MainPlayerController->TogglePauseMenu();
	}
}

void AMain::SetEquippedWeapon(AWeapon* WeaponToSet)
{
	if (EquippedWeapon) EquippedWeapon->Destroy();
	EquippedWeapon = WeaponToSet;
}

void AMain::Attack()
{
	if (!bAttacking && MovementStatus != EMovementStatus::EMS_Dead)
	{
		bAttacking = true;

		SetInterpToEnemy(true);

		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

		if (AnimInstance && CombatMontage)
		{
			int32 Selection = FMath::RandRange(0, 1);
			switch (Selection)
			{
			case 0:
				AnimInstance->Montage_Play(CombatMontage, 1.35f);
				AnimInstance->Montage_JumpToSection(FName("Attack_1"), CombatMontage);
				break;
			case 1:
				AnimInstance->Montage_Play(CombatMontage, 1.35f);
				AnimInstance->Montage_JumpToSection(FName("Attack_2"), CombatMontage);
				break;

			default:

				break;
			}

		}
	}
}

void AMain::AttackEnd()
{
	bAttacking = false;
	SetInterpToEnemy(false);
	if (bLMBDown)
	{
		Attack();
	}
}

void AMain::PlaySwingSound()
{
	if (EquippedWeapon->SwingSound)
	{
		UGameplayStatics::PlaySound2D(this, EquippedWeapon->SwingSound);
	}
}

void  AMain::SetInterpToEnemy(bool Interp)
{
	bInterpToEnemy = Interp;
}

void AMain::Jump()
{
	if (MainPlayerController) if (MainPlayerController->bPauseMenuVisible) return;

	if (MovementStatus != EMovementStatus::EMS_Dead)
	{
		Super::Jump();
	}
}


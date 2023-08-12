// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Item.generated.h"

UCLASS()
class FIRSTPROJECT_API AItem : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AItem();

	/* Base shape collision**/
	UPROPERTY(VisibleAnywhere,BlueprintReadWrite, Category = "Item | Collision")
	class USphereComponent* CollisionVolume; 

	/** Base mesh component */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite,
		Category = "Item | Mesh")
		UStaticMeshComponent* Mesh;

	/** ParticleSystemComponent */
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		Category = "Item | Particles")
	UParticleSystemComponent* IdelParticleComponent;
	
	/** ParticleSystem */
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		Category = "Item | Particles")
	UParticleSystem* OverlapParticles;


	/** SoundCue */
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		Category = "Item | Sounds")
	class USoundCue* OverlapSound;

	/** Rotate item */
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		Category = "Item | ItemProperties")
	bool bRotate;
	
	/** Rotation rate of item */
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		Category = "Item | ItemProperties")
	float RotationRate;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	virtual void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
		virtual	void OnOverlapEnd(
			UPrimitiveComponent* OverlappedComponent,
			AActor* OtherActor,
			UPrimitiveComponent* OtherComp,
			int32 OtherBodyIndex);

};

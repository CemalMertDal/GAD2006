// Fill out your copyright notice in the Description page of Project Settings.


#include "TilePlayerController.h"
#include "TileGameManager.h"
#include "GameFramework/PlayerInput.h"

void ATilePlayerController::BeginPlay()
{
	Super::BeginPlay();
	bShowMouseCursor = true;
}

void ATilePlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (GameManager == nullptr) return;
	
	FHitResult HitResult;
	FVector WorldLocation, WorldDirection;
	
	if (DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
	{	
		if (GetWorld()->LineTraceSingleByChannel(HitResult, WorldLocation, WorldLocation + WorldDirection * 50000.0f, ECC_Visibility))
		{
			AActor* HitActor = HitResult.GetActor();
			GameManager->OnActorInteraction(HitActor, HitResult.Location, PlayerInput->IsPressed(EKeys::LeftMouseButton));
		}
	}
}
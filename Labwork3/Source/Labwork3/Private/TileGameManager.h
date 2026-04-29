// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TileGameManager.generated.h"

#define MAX_MAP_GRID_SIZE 100

class ATileBase;

UCLASS()
class ATileGameManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATileGameManager();
	
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* GridSelection;

	// Homework #4: "Display current selected tile on the GridSelection. (Add a UStaticMeshComponent to ATileGameManager)"
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* TilePreviewComponent;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	void OnActorInteraction(AActor* HitActor, FVector Location, bool IsPressed);
	
	UPROPERTY(EditAnywhere)
	int GridSize;
	
	UPROPERTY(EditAnywhere)
	FVector GridOffset;

	UPROPERTY(EditAnywhere)
	TArray<ATileBase*> TileTypes;

	TArray<FVector> TileScales;

	UPROPERTY(EditAnywhere)
	int MapExtendsInGrids;

	int CurrentTileIndex;
	FRotator CurrentTileRotation;

	ATileBase* Map[MAX_MAP_GRID_SIZE][MAX_MAP_GRID_SIZE];

	void UpdateTilePreview();
};

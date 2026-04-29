// Fill out your copyright notice in the Description page of Project Settings.


#include "TileGameManager.h"
#include "TilePlayerController.h"
#include "TileBase.h"
#include "Engine/World.h"
#include "GameFramework/PlayerInput.h"

// Sets default values
ATileGameManager::ATileGameManager()
{
	PrimaryActorTick.bCanEverTick = true;

	GridSize = 100;
	GridOffset = FVector(0.0f, 0.0f, 0.5f);
	MapExtendsInGrids = 25;
	CurrentTileIndex = 0;
	CurrentTileRotation = FRotator::ZeroRotator;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	
	GridSelection = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GridMesh"));
	GridSelection->SetupAttachment(RootComponent);
	
	TilePreviewComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TilePreview"));
	TilePreviewComponent->SetupAttachment(GridSelection);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("StaticMesh'/Engine/BasicShapes/Plane.Plane'"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GridMaterial(TEXT("Material'/Game/UI/MAT_GridSlot.MAT_GridSlot'"));
	
	if (PlaneMesh.Succeeded())
	{
		GridSelection->SetStaticMesh(PlaneMesh.Object);
	}
	if (GridMaterial.Succeeded())
	{
		GridSelection->SetMaterial(0, GridMaterial.Object);
	}
	GridSelection->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	TilePreviewComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	for (int i = 0; i < MAX_MAP_GRID_SIZE; ++i)
	{
		for (int j = 0; j < MAX_MAP_GRID_SIZE; ++j)
		{
			Map[i][j] = nullptr;
		}
	}
}

// Called when the game starts or when spawned
void ATileGameManager::BeginPlay()
{
	Super::BeginPlay();
	
	if (auto PlayerController = Cast<ATilePlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		PlayerController->GameManager = this;
	}

	TileScales.Empty();
	for (ATileBase* Tile : TileTypes)
	{
		if (Tile && Tile->InstancedMesh)
		{
			FVector OldScale = Tile->InstancedMesh->GetRelativeScale3D();
			TileScales.Add(OldScale);
			
			if (!OldScale.Equals(FVector(1.0f, 1.0f, 1.0f)))
			{
				// Update existing instances so they don't break when we reset component scale
				for (int32 i = 0; i < Tile->InstancedMesh->GetInstanceCount(); ++i)
				{
					FTransform LocalTransform;
					Tile->InstancedMesh->GetInstanceTransform(i, LocalTransform, false);
					LocalTransform.SetScale3D(LocalTransform.GetScale3D() * OldScale);
					Tile->InstancedMesh->UpdateInstanceTransform(i, LocalTransform, false, false, true);
				}
				
				Tile->InstancedMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
			}
		}
		else
		{
			TileScales.Add(FVector(1.0f, 1.0f, 1.0f));
		}
	}

	UpdateTilePreview();
}

// Called every frame
void ATileGameManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ATileGameManager::UpdateTilePreview()
{
	if (TileTypes.IsValidIndex(CurrentTileIndex) && TileTypes[CurrentTileIndex])
	{
		ATileBase* SelectedTile = TileTypes[CurrentTileIndex];
		if (SelectedTile->InstancedMesh && SelectedTile->InstancedMesh->GetStaticMesh())
		{
			TilePreviewComponent->SetStaticMesh(SelectedTile->InstancedMesh->GetStaticMesh());
			if (TileScales.IsValidIndex(CurrentTileIndex))
			{
				TilePreviewComponent->SetRelativeScale3D(TileScales[CurrentTileIndex]);
			}
		}
	}
	else
	{
		TilePreviewComponent->SetStaticMesh(nullptr);
	}

	TilePreviewComponent->SetRelativeRotation(CurrentTileRotation);
}

void ATileGameManager::OnActorInteraction(AActor* HitActor, FVector Location, bool IsPressed)
{
	if (TileTypes.Num() == 0) return;

	FVector GridLoc = GridOffset;
	GridLoc.X += FMath::GridSnap(Location.X, GridSize);
	GridLoc.Y += FMath::GridSnap(Location.Y, GridSize);
	GridLoc.Z = Location.Z;

	UPlayerInput* Input = GetWorld()->GetFirstPlayerController()->PlayerInput;

	// Mouse Location Snapping (Always happen)
	GridSelection->SetWorldLocation(GridLoc + GridOffset);

	// Handle Placement
	if (Input->WasJustPressed(EKeys::LeftMouseButton))
	{
		int GridX = GridLoc.X / GridSize + MapExtendsInGrids;
		int GridY = GridLoc.Y / GridSize + MapExtendsInGrids;

		if (GridX < 0 || GridY < 0 || GridX >= MapExtendsInGrids * 2 || GridY >= MapExtendsInGrids * 2)
		{
			// Can not place out of the grid
			return;
		}

		// Already a tile here?
		if (Map[GridX][GridY] != nullptr) return;

		if (TileTypes.IsValidIndex(CurrentTileIndex))
		{
			ATileBase* SelectedTile = TileTypes[CurrentTileIndex];
			Map[GridX][GridY] = SelectedTile;

			FTransform TileTransform(CurrentTileRotation, GridLoc + GridOffset);
			FTransform InstanceTransform = SelectedTile->InstancedMesh->GetRelativeTransform() * TileTransform;
			if (TileScales.IsValidIndex(CurrentTileIndex))
			{
				InstanceTransform.SetScale3D(TileScales[CurrentTileIndex]);
			}
			SelectedTile->InstancedMesh->AddInstance(InstanceTransform, true);
		}
		
		UE_LOG(LogTemp, Warning, TEXT("Hit: %s - %f,%f,%f"), HitActor ? *HitActor->GetActorLabel() : TEXT("None"), Location.X, Location.Y, Location.Z);
	}
	else if (Input->WasJustPressed(EKeys::MouseScrollDown)) // Cycle Forward
	{
		CurrentTileIndex = (CurrentTileIndex + 1) % TileTypes.Num();
		UE_LOG(LogTemp, Warning, TEXT("TileType: %s"), *TileTypes[CurrentTileIndex]->GetActorLabel());
		UpdateTilePreview();
	}
	// Homework #4: MouseRightClick -> Rotates the tile in 90 degrees in Yaw
	else if (Input->WasJustPressed(EKeys::RightMouseButton))
	{
		CurrentTileRotation.Yaw += 90.0f;
		UpdateTilePreview();
	}
	// Homework #4: MouseScrollUp -> Cycles tiles backwards
	else if (Input->WasJustPressed(EKeys::MouseScrollUp))
	{
		CurrentTileIndex = CurrentTileIndex - 1;
		if (CurrentTileIndex < 0)
		{
			CurrentTileIndex = TileTypes.Num() - 1;
		}
		UE_LOG(LogTemp, Warning, TEXT("TileType: %s"), *TileTypes[CurrentTileIndex]->GetActorLabel());
		UpdateTilePreview();
	}
}

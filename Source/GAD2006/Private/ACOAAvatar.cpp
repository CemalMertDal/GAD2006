// Fill out your copyright notice in the Description page of Project Settings.


#include "ACOAAvatar.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values
AACOAAvatar::AACOAAvatar()
{
	
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->TargetArmLength = 300.0f;
	SpringArm->SetupAttachment(RootComponent);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("MainCamera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);

	Camera->bUsePawnControlRotation = false;
	SpringArm->bUsePawnControlRotation = true;
	bUseControllerRotationYaw = false;

	RunSpeed = 1000.0f;

	MaxStamina = 100.0f;
	Stamina = MaxStamina;
	StaminaDrainRate = 10.0f;
	StaminaGainRate = 5.0f;
	bIsStaminaDrained = false;
	bIsRunning = false;
}

// Called when the game starts or when spawned
void AACOAAvatar::BeginPlay()
{
	Super::BeginPlay();
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	
}

// Called every frame
void AACOAAvatar::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	bool bIsMoving = GetVelocity().SizeSquared() > 0.0f;
	bool bIsWalkingMode = GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Walking;

	if (bIsRunning && bIsMoving && bIsWalkingMode && !bIsStaminaDrained)
	{
		Stamina -= StaminaDrainRate * DeltaTime;

		if (Stamina <= 0.0f)
		{
			Stamina = 0.0f;
			bIsStaminaDrained = true;
			UpdateMovementParams();
		}
	}
	else
	{
		if (Stamina < MaxStamina)
		{
			Stamina += StaminaGainRate * DeltaTime;

			if (Stamina >= MaxStamina)
			{
				Stamina = MaxStamina;

				if (bIsStaminaDrained)
				{
					bIsStaminaDrained = false;
					UpdateMovementParams();
				}
			}
		}
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1, 0.0f, FColor::Yellow, FString::Printf(TEXT("Stamina: %d / %d"), FMath::RoundToInt(Stamina), FMath::RoundToInt(MaxStamina)));
		if (bIsStaminaDrained)
		{
			GEngine->AddOnScreenDebugMessage(2, 0.0f, FColor::Red, TEXT("STAMINA DRAINED!"));
		}
	}
}

// Called to bind functionality to input
void AACOAAvatar::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("Turn", this, &ACharacter::AddControllerYawInput);
	PlayerInputComponent->BindAxis("LookUp", this, &ACharacter::AddControllerPitchInput);

	PlayerInputComponent->BindAxis("MoveForward", this, &AACOAAvatar::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AACOAAvatar::MoveRight);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("Run", IE_Pressed, this, &AACOAAvatar::RunPressed);
	PlayerInputComponent->BindAction("Run", IE_Released, this, &AACOAAvatar::RunReleased);

}


void AACOAAvatar::MoveForward(float Value)
{
	FRotator Rotation = Controller->GetControlRotation();
	FRotator YawRotation(0, Rotation.Yaw, 0);
	FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	AddMovementInput(ForwardDirection, Value);
}

void AACOAAvatar::MoveRight(float Value)
{
	FRotator Rotation = Controller->GetControlRotation();
	FRotator YawRotation(0, Rotation.Yaw, 0);
	FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	AddMovementInput(RightDirection, Value);
}

void AACOAAvatar::RunPressed()
{
	bIsRunning = true;
	UpdateMovementParams();
}

void AACOAAvatar::RunReleased()
{
	bIsRunning = false;
	UpdateMovementParams();
}

void AACOAAvatar::UpdateMovementParams()
{
	if (bIsRunning && !bIsStaminaDrained)
	{
		GetCharacterMovement()->MaxWalkSpeed = RunSpeed;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	}
}

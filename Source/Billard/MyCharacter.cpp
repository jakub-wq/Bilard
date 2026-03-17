#include "MyCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "PoolBall.h"
#include "PoolGameMode.h"
#include "PoolHUDWidget.h"
#include "PoolTableManager.h"
#include "UObject/ConstructorHelpers.h"

AMyCharacter::AMyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);

	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = false;
	Movement->MaxWalkSpeed = 420.0f;
	Movement->JumpZVelocity = 420.0f;
	Movement->AirControl = 0.2f;

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(-10.0f, 0.0f, 64.0f));
	FirstPersonCamera->bUsePawnControlRotation = true;
	FirstPersonCamera->SetActive(true);

	AimCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("AimCamera"));
	AimCamera->SetupAttachment(GetRootComponent());
	AimCamera->bUsePawnControlRotation = false;
	AimCamera->SetActive(false);

	CueMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CueMesh"));
	CueMesh->SetupAttachment(GetRootComponent());
	CueMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CueMesh->SetCastShadow(true);
	CueMesh->SetHiddenInGame(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CueAsset(TEXT("/Game/ThirdPerson/Kij.Kij"));
	if (CueAsset.Succeeded())
	{
		CueMesh->SetStaticMesh(CueAsset.Object);
		CueMesh->SetRelativeScale3D(FVector(1.0f));
	}
	else
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
		if (CylinderMesh.Succeeded())
		{
			CueMesh->SetStaticMesh(CylinderMesh.Object);
			CueMesh->SetRelativeScale3D(FVector(0.03f, 0.03f, 1.7f));
		}
	}
}

void AMyCharacter::BeginPlay()
{
	Super::BeginPlay();
	AttachCueToPlayer();
	EnsureHUDWidget();
	UpdateNormalFacingToTable();
}

void AMyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	EnsureHUDWidget();

	if (bIsChargingShot)
	{
		CurrentShotPower = FMath::Clamp(CurrentShotPower + ChargeRate * DeltaTime, 0.0f, MaxShotPower);
	}

	UpdateAimMode(DeltaTime);
	UpdateCueVisual();
	UpdateHUD();
}


void AMyCharacter::EnsureHUDWidget()
{
	if (HUDWidget || !IsLocallyControlled())
	{
		return;
	}

	HUDWidget = CreateWidget<UPoolHUDWidget>(GetWorld(), UPoolHUDWidget::StaticClass());
	if (HUDWidget)
	{
		HUDWidget->AddToViewport(10);
		HUDWidget->OnResetClicked.RemoveDynamic(this, &AMyCharacter::HandleResetWidgetClicked);
		HUDWidget->OnResetClicked.AddDynamic(this, &AMyCharacter::HandleResetWidgetClicked);
		HUDWidget->SetHintText(TEXT("Podejdź do białej bili i kliknij LPM, aby wejść w tryb strzału."));
	}
}

void AMyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AMyCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AMyCharacter::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AMyCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &AMyCharacter::LookUpAtRate);
	PlayerInputComponent->BindAxis(TEXT("Move Forward / Backward"), this, &AMyCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("Move Right / Left"), this, &AMyCharacter::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("Turn Right / Left Mouse"), this, &AMyCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis(TEXT("Look Up / Down Mouse"), this, &AMyCharacter::LookUpAtRate);

	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAction(TEXT("Shoot"), IE_Pressed, this, &AMyCharacter::PrimaryActionPressed);
	PlayerInputComponent->BindAction(TEXT("Shoot"), IE_Released, this, &AMyCharacter::PrimaryActionReleased);
	PlayerInputComponent->BindAction(TEXT("CancelShot"), IE_Pressed, this, &AMyCharacter::CancelShot);
	PlayerInputComponent->BindAction(TEXT("ResetBalls"), IE_Pressed, this, &AMyCharacter::ResetBalls);
}

void AMyCharacter::MoveForward(float Value)
{
	if (Controller && !FMath::IsNearlyZero(Value) && !bIsAimMode)
	{
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AMyCharacter::MoveRight(float Value)
{
	if (Controller && !FMath::IsNearlyZero(Value) && !bIsAimMode)
	{
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AMyCharacter::TurnAtRate(float Value)
{
	if (bIsAimMode)
	{
		AimYawDegrees += Value * 1.8f;
		return;
	}
	AddControllerYawInput(Value);
}

void AMyCharacter::LookUpAtRate(float Value)
{
	if (!bIsAimMode)
	{
		AddControllerPitchInput(Value);
	}
}

void AMyCharacter::PrimaryActionPressed()
{
	if (bIsAimMode)
	{
		StartChargingShot();
		return;
	}

	TryEnterAimMode();
}

void AMyCharacter::PrimaryActionReleased()
{
	if (bIsAimMode)
	{
		ReleaseShot();
	}
}

void AMyCharacter::StartChargingShot()
{
	if (!bIsAimMode || !ActiveCueBall)
	{
		return;
	}

	if (APoolTableManager* Manager = GetPoolManager())
	{
		if (!Manager->AreBallsStopped())
		{
			ShowMessage(TEXT("Poczekaj, aż bile się zatrzymają."));
			return;
		}
	}

	bIsChargingShot = true;
}

void AMyCharacter::ReleaseShot()
{
	if (!bIsChargingShot || !bIsAimMode || !ActiveCueBall)
	{
		return;
	}

	bIsChargingShot = false;
	const FVector ShotDirection = FVector::VectorPlaneProject(AimCamera->GetForwardVector(), FVector::UpVector).GetSafeNormal();
	ActiveCueBall->ApplyShotImpulse(ShotDirection, FMath::Max(CurrentShotPower, 450.0f));
	CurrentShotPower = 0.0f;
	ExitAimMode();
}

void AMyCharacter::CancelShot()
{
	bIsChargingShot = false;
	CurrentShotPower = 0.0f;
	if (bIsAimMode)
	{
		ExitAimMode();
	}
}

void AMyCharacter::ResetBalls()
{
	if (APoolTableManager* Manager = GetPoolManager())
	{
		Manager->ResetRack();
		ExitAimMode();
		UpdateNormalFacingToTable();
		ShowMessage(TEXT("Układ bil został zresetowany."));
	}
}

void AMyCharacter::HandleResetWidgetClicked()
{
	ResetBalls();
}

void AMyCharacter::EnterAimMode(APoolBall* InCueBall)
{
	if (!InCueBall)
	{
		return;
	}

	ActiveCueBall = InCueBall;
	bIsAimMode = true;
	bIsChargingShot = false;
	CurrentShotPower = 0.0f;
	GetCharacterMovement()->StopMovementImmediately();

	if (APoolTableManager* Manager = GetPoolManager())
	{
		const FVector DesiredDir = (Manager->GetRackCenterLocation() - InCueBall->GetActorLocation()).GetSafeNormal2D();
		AimYawDegrees = DesiredDir.Rotation().Yaw;
	}
	else
	{
		AimYawDegrees = GetControlRotation().Yaw;
	}

	FirstPersonCamera->SetActive(false);
	AimCamera->SetActive(true);
	PositionAimCamera();
	ShowMessage(TEXT("Tryb uderzenia: przytrzymaj LPM, puść aby uderzyć. PPM anuluje."));
}

void AMyCharacter::ExitAimMode()
{
	bIsAimMode = false;
	bIsChargingShot = false;
	CurrentShotPower = 0.0f;
	ActiveCueBall = nullptr;
	AimCamera->SetActive(false);
	FirstPersonCamera->SetActive(true);
	AttachCueToPlayer();
}

void AMyCharacter::PositionAimCamera()
{
	if (!ActiveCueBall)
	{
		return;
	}

	const FVector AimDir = FRotator(0.0f, AimYawDegrees, 0.0f).Vector();
	const FVector BallLocation = ActiveCueBall->GetActorLocation();
	const FVector CameraLocation = BallLocation - AimDir * AimOrbitDistance + FVector(0.0f, 0.0f, AimOrbitHeight);
	AimCamera->SetWorldLocation(CameraLocation);
	AimCamera->SetWorldRotation((BallLocation - CameraLocation).Rotation() + FRotator(AimPitchDegrees, 0.0f, 0.0f));
}

void AMyCharacter::UpdateAimMode(float DeltaTime)
{
	if (!bIsAimMode || !ActiveCueBall)
	{
		return;
	}

	PositionAimCamera();
	DrawTrajectoryPreview();
}

void AMyCharacter::UpdateCueVisual()
{
	if (!CueMesh)
	{
		return;
	}

	if (!bIsAimMode || !ActiveCueBall)
	{
		AttachCueToPlayer();
		return;
	}

	CueMesh->SetHiddenInGame(false);
	const FVector ShotDirection = FVector::VectorPlaneProject(AimCamera->GetForwardVector(), FVector::UpVector).GetSafeNormal();
	const FVector BallLocation = ActiveCueBall->GetActorLocation();
	const float Pullback = bIsChargingShot ? (CuePullbackDistance * (CurrentShotPower / MaxShotPower)) : 0.0f;
	const FVector CueLocation = BallLocation - ShotDirection * (CueDistanceFromBall + Pullback) + FVector(0.0f, 0.0f, 1.0f);
	FRotator CueRotation = ShotDirection.Rotation();
	CueRotation.Pitch = 0.0f;
	CueRotation.Roll = 0.0f;
	CueMesh->SetWorldLocation(CueLocation);
	CueMesh->SetWorldRotation(CueRotation);
}

void AMyCharacter::AttachCueToPlayer()
{
	if (!CueMesh)
	{
		return;
	}

	CueMesh->SetHiddenInGame(true);
}

void AMyCharacter::ShowMessage(const FString& Message) const
{
	if (HUDWidget)
	{
		HUDWidget->SetHintText(Message);
	}
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::White, Message);
	}
}

APoolBall* AMyCharacter::FindInteractCueBall() const
{
	if (APoolTableManager* Manager = GetPoolManager())
	{
		APoolBall* CueBall = Manager->GetCueBall();
		if (CueBall)
		{
			const float Dist = FVector::Dist(GetActorLocation(), CueBall->GetActorLocation());
			if (Dist <= InteractDistance)
			{
				return CueBall;
			}
		}
	}
	return nullptr;
}

bool AMyCharacter::TryEnterAimMode()
{
	if (APoolBall* CueBall = FindInteractCueBall())
	{
		EnterAimMode(CueBall);
		return true;
	}

	ShowMessage(TEXT("Podejdź bliżej stołu i białej bili."));
	return false;
}

APoolTableManager* AMyCharacter::GetPoolManager() const
{
	if (CachedPoolManager.IsValid())
	{
		return CachedPoolManager.Get();
	}

	if (const APoolGameMode* GameMode = GetWorld() ? Cast<APoolGameMode>(GetWorld()->GetAuthGameMode()) : nullptr)
	{
		CachedPoolManager = GameMode->GetPoolManager();
		return CachedPoolManager.Get();
	}

	return nullptr;
}

void AMyCharacter::UpdateHUD()
{
	if (!HUDWidget)
	{
		return;
	}

	HUDWidget->SetAimMode(bIsAimMode);
	HUDWidget->SetShotPowerPercent(MaxShotPower > 0.0f ? CurrentShotPower / MaxShotPower : 0.0f);
}

void AMyCharacter::DrawTrajectoryPreview() const
{
	if (!bIsAimMode || !ActiveCueBall || !GetWorld())
	{
		return;
	}

	const FVector Start = ActiveCueBall->GetActorLocation();
	const FVector Direction = FVector::VectorPlaneProject(AimCamera->GetForwardVector(), FVector::UpVector).GetSafeNormal();
	const FVector End = Start + Direction * 500.0f;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PoolTrajectory), true);
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(ActiveCueBall);

	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		DrawDebugLine(GetWorld(), Start, Hit.ImpactPoint, FColor::Green, false, -1.0f, 0, 2.0f);
		const FVector Reflected = Direction.MirrorByVector(Hit.ImpactNormal).GetSafeNormal();
		DrawDebugLine(GetWorld(), Hit.ImpactPoint, Hit.ImpactPoint + Reflected * 180.0f, FColor::Yellow, false, -1.0f, 0, 2.0f);
	}
	else
	{
		DrawDebugLine(GetWorld(), Start, End, FColor::Green, false, -1.0f, 0, 2.0f);
	}
}

void AMyCharacter::UpdateNormalFacingToTable()
{
	if (APoolTableManager* Manager = GetPoolManager())
	{
		const FVector AimLocation = Manager->GetCueBallStartLocation() - Manager->GetTableLongAxis() * 120.0f;
		const FVector Desired = Manager->GetCueBallStartLocation() - AimLocation;
		SetActorLocation(FVector(AimLocation.X, AimLocation.Y, GetActorLocation().Z));
		if (Controller)
		{
			Controller->SetControlRotation(Desired.Rotation());
		}
		SetActorRotation(FRotator(0.0f, Desired.Rotation().Yaw, 0.0f));
	}
}

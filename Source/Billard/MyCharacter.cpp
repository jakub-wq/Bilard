#include "MyCharacter.h"

#include "Camera/CameraComponent.h"
#include "CollisionShape.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "PoolBall.h"
#include "PoolGameMode.h"
#include "PoolHUDWidget.h"
#include "PoolSaveGame.h"
#include "PoolTableManager.h"
#include "UObject/ConstructorHelpers.h"
#include "EngineUtils.h"

namespace
{
	constexpr TCHAR PoolSettingsSlotName[] = TEXT("PoolSettings");
	constexpr int32 PoolSettingsUserIndex = 0;

	float GetMeshLongestExtent(const UStaticMesh* Mesh)
	{
		if (!Mesh)
		{
			return 0.0f;
		}

		return Mesh->GetBoundingBox().GetExtent().GetMax();
	}

	FVector ComputeCueMeshScale(const UStaticMesh* Mesh, float DesiredCueLength)
	{
		const float LongestExtent = GetMeshLongestExtent(Mesh);
		if (LongestExtent <= KINDA_SMALL_NUMBER)
		{
			return FVector(1.0f);
		}

		const float CurrentLength = LongestExtent * 2.0f;
		if (CurrentLength >= DesiredCueLength * 0.65f)
		{
			return FVector(1.0f);
		}

		const float UniformScale = DesiredCueLength / CurrentLength;
		return FVector(UniformScale);
	}

	UMaterialInterface* LoadCueSkinMaterial(ECueSkin Skin)
	{
		const TCHAR* MaterialPath = nullptr;
		switch (Skin)
		{
		case ECueSkin::Blue:
			MaterialPath = TEXT("/Game/Billiards/Cues/Bleu.Bleu");
			break;
		case ECueSkin::Red:
			MaterialPath = TEXT("/Game/Billiards/Balls/Rouge.Rouge");
			break;
		case ECueSkin::Yellow:
			MaterialPath = TEXT("/Game/Billiards/Balls/Jaune.Jaune");
			break;
		case ECueSkin::Standard:
		default:
			MaterialPath = TEXT("/Game/Billiards/Cues/Bois_clair.Bois_clair");
			break;
		}

		return LoadObject<UMaterialInterface>(nullptr, MaterialPath);
	}
}

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
	Movement->GetNavAgentPropertiesRef().bCanCrouch = true;
	Movement->CrouchedHalfHeight = 56.0f;

	CrouchedEyeHeight = 48.0f;

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

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CueAsset(TEXT("/Game/Billiards/Cues/CueStick_A_Source.CueStick_A_Source"));
	if (CueAsset.Succeeded())
	{
		CueMesh->SetStaticMesh(CueAsset.Object);
		CueMesh->SetRelativeScale3D(ComputeCueMeshScale(CueAsset.Object, DesiredCueLength));
	}
	else
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> LegacyCueAsset(TEXT("/Game/ThirdPerson/Kij.Kij"));
		if (LegacyCueAsset.Succeeded())
		{
			CueMesh->SetStaticMesh(LegacyCueAsset.Object);
			CueMesh->SetRelativeScale3D(ComputeCueMeshScale(LegacyCueAsset.Object, DesiredCueLength));
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
}

void AMyCharacter::BeginPlay()
{
	Super::BeginPlay();
	LoadCueSkinPreference();
	AttachCueToPlayer();
	EnsureHUDWidget();
	UpdateNormalFacingToTable();
}

void AMyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	EnsureHUDWidget();
	SyncCueBallPlacementMode();

	if (bIsChargingShot)
	{
		CurrentShotPower = FMath::Clamp(CurrentShotPower + ChargeRate * DeltaTime, 0.0f, MaxShotPower);
	}

	UpdateAimMode(DeltaTime);
	UpdateCueBallPlacementMode(DeltaTime);
	UpdateCueVisual();
	UpdateHUD();
}


void AMyCharacter::EnsureHUDWidget()
{
	if (HUDWidget)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	}

	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return;
	}

	HUDWidget = CreateWidget<UPoolHUDWidget>(PlayerController, UPoolHUDWidget::StaticClass());
	if (HUDWidget)
	{
		HUDWidget->AddToPlayerScreen(10);
		HUDWidget->SetVisibility(bShowHUDWidget ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		HUDWidget->OnResetClicked.RemoveDynamic(this, &AMyCharacter::HandleResetWidgetClicked);
		HUDWidget->OnResetClicked.AddDynamic(this, &AMyCharacter::HandleResetWidgetClicked);
		HUDWidget->SetHintText(TEXT("Podejdź do stołu i kliknij celownikiem w białą bilę."));
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
	PlayerInputComponent->BindAction(TEXT("ToggleCrouch"), IE_Pressed, this, &AMyCharacter::ToggleCrouch);
	PlayerInputComponent->BindAction(TEXT("Shoot"), IE_Pressed, this, &AMyCharacter::PrimaryActionPressed);
	PlayerInputComponent->BindAction(TEXT("Shoot"), IE_Released, this, &AMyCharacter::PrimaryActionReleased);
	PlayerInputComponent->BindAction(TEXT("CancelShot"), IE_Pressed, this, &AMyCharacter::CancelShot);
	PlayerInputComponent->BindAction(TEXT("ResetBalls"), IE_Pressed, this, &AMyCharacter::ResetBalls);
}

void AMyCharacter::MoveForward(float Value)
{
	if (bIsCueBallPlacementMode)
	{
		PlacementForwardInput = Value;
		return;
	}

	if (bIsAimMode)
	{
		UpdateSpinInput(0.0f, Value * SpinAdjustSpeed);
		return;
	}

	if (Controller && !FMath::IsNearlyZero(Value) && !bIsAimMode)
	{
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AMyCharacter::MoveRight(float Value)
{
	if (bIsCueBallPlacementMode)
	{
		PlacementRightInput = Value;
		return;
	}

	if (bIsAimMode)
	{
		UpdateSpinInput(Value * SpinAdjustSpeed, 0.0f);
		return;
	}

	if (Controller && !FMath::IsNearlyZero(Value) && !bIsAimMode)
	{
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AMyCharacter::TurnAtRate(float Value)
{
	if (bIsCueBallPlacementMode)
	{
		return;
	}

	if (bIsAimMode)
	{
		AimYawDegrees += Value * 1.8f;
		return;
	}
	AddControllerYawInput(Value);
}

void AMyCharacter::LookUpAtRate(float Value)
{
	if (bIsCueBallPlacementMode)
	{
		return;
	}

	if (bIsAimMode)
	{
		UpdateSpinInput(0.0f, Value * SpinAdjustSpeed);
		return;
	}

	if (!bIsAimMode)
	{
		AddControllerPitchInput(Value);
	}
}

void AMyCharacter::PrimaryActionPressed()
{
	SyncCueBallPlacementMode();

	if (bIsCueBallPlacementMode)
	{
		if (APoolTableManager* Manager = GetPoolManager())
		{
			if (Manager->ConfirmCueBallPlacement(CueBallPlacementLocation))
			{
				ExitCueBallPlacementMode();
				ShowMessage(TEXT("Ustawiono białą bilę. Kliknij ją, aby wejść w tryb uderzenia."));
			}
			else
			{
				ShowMessage(TEXT("Nie można ustawić białej bili w tym miejscu."));
			}
		}
		return;
	}

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
	const FVector ShotDirection = GetAimDirection();
	const FVector TableUpAxis = GetPoolManager() ? GetPoolManager()->GetTableUpAxis() : FVector::UpVector;
	ActiveCueBall->ApplyShotImpulse(ShotDirection, FMath::Max(CurrentShotPower, 240.0f), SpinInput, TableUpAxis);
	CurrentShotPower = 0.0f;
	SpinInput = FVector2D::ZeroVector;
	ExitAimMode();
}

void AMyCharacter::CancelShot()
{
	if (bIsCueBallPlacementMode)
	{
		ShowMessage(TEXT("Ustaw białą bilę na stole i kliknij, aby potwierdzić."));
		return;
	}

	bIsChargingShot = false;
	CurrentShotPower = 0.0f;
	SpinInput = FVector2D::ZeroVector;
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
		ExitCueBallPlacementMode();
		UpdateNormalFacingToTable();
		ShowMessage(TEXT("Układ bil został zresetowany."));
	}
}

void AMyCharacter::ToggleCrouch()
{
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
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
	SpinInput = FVector2D::ZeroVector;
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
	ShowMessage(TEXT("Tryb uderzenia: LPM ładuje strzał, PPM anuluje, A/D nadaje boczną rotację, W/S lub ruch myszy góra/dół dodaje górną/dolną rotację."));
}

void AMyCharacter::ExitAimMode()
{
	bIsAimMode = false;
	bIsChargingShot = false;
	CurrentShotPower = 0.0f;
	SpinInput = FVector2D::ZeroVector;
	ActiveCueBall = nullptr;
	AimCamera->SetActive(false);
	FirstPersonCamera->SetActive(true);
	AttachCueToPlayer();
}

void AMyCharacter::EnterCueBallPlacementMode()
{
	if (bIsCueBallPlacementMode)
	{
		return;
	}

	ExitAimMode();
	bIsCueBallPlacementMode = true;
	PlacementForwardInput = 0.0f;
	PlacementRightInput = 0.0f;

	if (APoolTableManager* Manager = GetPoolManager())
	{
		CueBallPlacementLocation = Manager->GetCueBallInHandLocation();
		Manager->UpdateCueBallInHandPreview(CueBallPlacementLocation);
	}

	FirstPersonCamera->SetActive(false);
	AimCamera->SetActive(true);
	PositionCueBallPlacementCamera();
	UE_LOG(
		LogTemp,
		Display,
		TEXT("Character entered cue-ball placement mode: character=%s location=%s"),
		*GetNameSafe(this),
		*CueBallPlacementLocation.ToCompactString());
	ShowMessage(TEXT("Biała bila w ręce: W/S/A/D przesuwa bilę, LPM potwierdza ustawienie."));
}

void AMyCharacter::ExitCueBallPlacementMode()
{
	bIsCueBallPlacementMode = false;
	PlacementForwardInput = 0.0f;
	PlacementRightInput = 0.0f;

	if (!bIsAimMode)
	{
		AimCamera->SetActive(false);
		FirstPersonCamera->SetActive(true);
	}
}

void AMyCharacter::SyncCueBallPlacementMode()
{
	if (APoolTableManager* Manager = GetPoolManager())
	{
		if (Manager->IsCueBallInHand())
		{
			if (!bIsCueBallPlacementMode)
			{
				EnterCueBallPlacementMode();
			}
		}
		else if (bIsCueBallPlacementMode)
		{
			ExitCueBallPlacementMode();
		}
	}
	else if (bIsCueBallPlacementMode)
	{
		ExitCueBallPlacementMode();
	}
}

void AMyCharacter::PositionAimCamera()
{
	if (!ActiveCueBall)
	{
		return;
	}

	const FVector AimDir = GetAimDirection();
	const FVector UpAxis = GetPoolManager() ? GetPoolManager()->GetTableUpAxis() : FVector::UpVector;
	const FVector BallLocation = ActiveCueBall->GetActorLocation();
	const FVector CameraLocation = BallLocation - AimDir * AimOrbitDistance + UpAxis * AimOrbitHeight;
	FRotator CameraRotation = FRotationMatrix::MakeFromXZ(AimDir, UpAxis).Rotator();
	CameraRotation.Pitch += AimPitchDegrees;
	AimCamera->SetWorldLocation(CameraLocation);
	AimCamera->SetWorldRotation(CameraRotation);
}

void AMyCharacter::PositionCueBallPlacementCamera()
{
	if (APoolTableManager* Manager = GetPoolManager())
	{
		const FVector UpAxis = Manager->GetTableUpAxis();
		const FVector CameraLocation = Manager->GetTableSurfaceCenter() + UpAxis * PlacementCameraHeight;
		const FRotator CameraRotation = FRotationMatrix::MakeFromXZ(-UpAxis, Manager->GetTableLongAxis()).Rotator();
		AimCamera->SetWorldLocation(CameraLocation);
		AimCamera->SetWorldRotation(CameraRotation);
	}
}

void AMyCharacter::UpdateAimMode(float DeltaTime)
{
	if (!bIsAimMode || !ActiveCueBall)
	{
		return;
	}

	PositionAimCamera();
}

void AMyCharacter::UpdateCueBallPlacementMode(float DeltaTime)
{
	SyncCueBallPlacementMode();

	if (!bIsCueBallPlacementMode)
	{
		return;
	}

	APoolTableManager* Manager = GetPoolManager();
	if (!Manager)
	{
		return;
	}

	const FVector DeltaMove =
		Manager->GetTableLongAxis() * (PlacementForwardInput * PlacementMoveSpeed * DeltaTime)
		+ Manager->GetTableShortAxis() * (PlacementRightInput * PlacementMoveSpeed * DeltaTime);
	CueBallPlacementLocation += DeltaMove;
	Manager->UpdateCueBallInHandPreview(CueBallPlacementLocation);
	CueBallPlacementLocation = Manager->GetCueBallInHandLocation();
	PositionCueBallPlacementCamera();
}

FTransform AMyCharacter::CalculateCueVisualTransform(
	const FVector& BallLocation,
	const FVector& ShotDirection,
	const FVector& UpAxis,
	float InCueDistanceFromBall,
	float InCuePullbackDistance,
	float InCurrentShotPower,
	float InMaxShotPower,
	float InCueSideOffset,
	float InCueHeightOffset,
	const FRotator& InCueAimRotationOffset,
	bool bInIsChargingShot)
{
	const FVector AimDirection = ShotDirection.GetSafeNormal();
	const FVector TableUpAxis = UpAxis.GetSafeNormal();
	const FVector SideAxis = FVector::CrossProduct(TableUpAxis, AimDirection).GetSafeNormal();
	const float Pullback = bInIsChargingShot && InMaxShotPower > KINDA_SMALL_NUMBER
		? (InCuePullbackDistance * (InCurrentShotPower / InMaxShotPower))
		: 0.0f;

	const FVector CueLocation =
		BallLocation
		- AimDirection * (InCueDistanceFromBall + Pullback)
		+ SideAxis * InCueSideOffset
		+ TableUpAxis * InCueHeightOffset;
	const FRotator CueRotation = FRotationMatrix::MakeFromX(-AimDirection).Rotator() + InCueAimRotationOffset;
	return FTransform(CueRotation, CueLocation);
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
	CueMesh->SetVisibility(true, true);
	const FVector ShotDirection = GetAimDirection();
	const FVector UpAxis = GetPoolManager() ? GetPoolManager()->GetTableUpAxis() : FVector::UpVector;
	const FVector BallLocation = ActiveCueBall->GetActorLocation();
	const FTransform CueTransform = CalculateCueVisualTransform(
		BallLocation,
		ShotDirection,
		UpAxis,
		CueDistanceFromBall,
		CuePullbackDistance,
		CurrentShotPower,
		MaxShotPower,
		CueSideOffset,
		CueHeightOffset,
		CueAimRotationOffset,
		bIsChargingShot);
	CueMesh->SetWorldLocation(CueTransform.GetLocation());
	CueMesh->SetWorldRotation(CueTransform.Rotator());
}

void AMyCharacter::AttachCueToPlayer()
{
	if (!CueMesh)
	{
		return;
	}

	CueMesh->SetHiddenInGame(true);
	CueMesh->SetVisibility(false, true);
}

void AMyCharacter::PrepareForMenu()
{
	CancelShot();
}

void AMyCharacter::SetInGameHUDVisible(bool bVisible)
{
	bShowHUDWidget = bVisible;
	if (HUDWidget)
	{
		HUDWidget->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void AMyCharacter::SetCueSkin(ECueSkin NewSkin, bool bSavePreference)
{
	ApplyCueSkin(NewSkin);

	if (bSavePreference)
	{
		SaveCueSkinPreference();
	}
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
		if (CueBall && !CueBall->IsPocketed() && FirstPersonCamera && GetWorld())
		{
			const FVector Start = FirstPersonCamera->GetComponentLocation();
			const FVector End = Start + FirstPersonCamera->GetForwardVector() * InteractDistance;
			const FVector CueBallLocation = CueBall->GetActorLocation();
			const FVector Segment = End - Start;
			const float SegmentLengthSquared = Segment.SizeSquared();
			if (SegmentLengthSquared <= KINDA_SMALL_NUMBER)
			{
				return nullptr;
			}

			const float ProjectionAlpha = FMath::Clamp(
				FVector::DotProduct(CueBallLocation - Start, Segment) / SegmentLengthSquared,
				0.0f,
				1.0f);
			const FVector ClosestPoint = Start + Segment * ProjectionAlpha;
			const float AimTolerance = FMath::Max(8.0f, Manager->GetBallRadius() * 1.35f);
			if (FVector::DistSquared(ClosestPoint, CueBallLocation) <= FMath::Square(AimTolerance))
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

	ShowMessage(TEXT("Podejdź bliżej stołu i kliknij celownikiem w białą bilę."));
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
		if (CachedPoolManager.IsValid())
		{
			return CachedPoolManager.Get();
		}
	}

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<APoolTableManager> It(World); It; ++It)
		{
			if (IsValid(*It))
			{
				CachedPoolManager = *It;
				return CachedPoolManager.Get();
			}
		}
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
	if (APoolTableManager* Manager = GetPoolManager())
	{
		HUDWidget->SetPocketedCount(Manager->GetPocketedBallCount());
	}
}

FVector AMyCharacter::GetAimDirection() const
{
	const FVector UpAxis = GetPoolManager() ? GetPoolManager()->GetTableUpAxis() : FVector::UpVector;
	return FVector::VectorPlaneProject(FRotator(0.0f, AimYawDegrees, 0.0f).Vector(), UpAxis).GetSafeNormal();
}

void AMyCharacter::UpdateSpinInput(float SideDelta, float TopDelta)
{
	if (!bIsAimMode)
	{
		return;
	}

	SpinInput.X = FMath::Clamp(SpinInput.X + SideDelta, -1.0f, 1.0f);
	SpinInput.Y = FMath::Clamp(SpinInput.Y + TopDelta, -1.0f, 1.0f);
}

void AMyCharacter::ApplyCueSkin(ECueSkin NewSkin)
{
	SelectedCueSkin = NewSkin;

	if (!CueMesh)
	{
		return;
	}

	if (UMaterialInterface* Material = LoadCueSkinMaterial(NewSkin))
	{
		const int32 MaterialCount = FMath::Max(1, CueMesh->GetNumMaterials());
		for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
		{
			CueMesh->SetMaterial(MaterialIndex, Material);
		}
	}
}

void AMyCharacter::LoadCueSkinPreference()
{
	if (const UPoolSaveGame* SaveGame = Cast<UPoolSaveGame>(UGameplayStatics::LoadGameFromSlot(PoolSettingsSlotName, PoolSettingsUserIndex)))
	{
		ApplyCueSkin(SaveGame->SelectedCueSkin);
		return;
	}

	ApplyCueSkin(ECueSkin::Standard);
}

void AMyCharacter::SaveCueSkinPreference() const
{
	UPoolSaveGame* SaveGame = Cast<UPoolSaveGame>(UGameplayStatics::CreateSaveGameObject(UPoolSaveGame::StaticClass()));
	if (!SaveGame)
	{
		return;
	}

	SaveGame->SelectedCueSkin = SelectedCueSkin;
	UGameplayStatics::SaveGameToSlot(SaveGame, PoolSettingsSlotName, PoolSettingsUserIndex);
}

void AMyCharacter::UpdateNormalFacingToTable()
{
	if (APoolTableManager* Manager = GetPoolManager())
	{
		const FVector AimLocation = Manager->GetCueBallStartLocation() - Manager->GetTableLongAxis() * 190.0f;
		const FVector Desired = Manager->GetCueBallStartLocation() - AimLocation;
		SetActorLocation(FVector(AimLocation.X, AimLocation.Y, GetActorLocation().Z));
		if (Controller)
		{
			Controller->SetControlRotation(Desired.Rotation());
		}
		SetActorRotation(FRotator(0.0f, Desired.Rotation().Yaw, 0.0f));
	}
}

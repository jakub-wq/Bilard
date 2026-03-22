#include "PoolBall.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "UObject/ConstructorHelpers.h"

APoolBall::APoolBall()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	SetRootComponent(CollisionSphere);
	CollisionSphere->SetMobility(EComponentMobility::Movable);

	BallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BallMesh"));
	BallMesh->SetupAttachment(CollisionSphere);
	BallMesh->SetMobility(EComponentMobility::Movable);
	BallMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BallMesh->SetGenerateOverlapEvents(false);
	BallMesh->SetCastShadow(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		BallMesh->SetStaticMesh(SphereMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (BasicMaterial.Succeeded())
	{
		BaseMaterial = BasicMaterial.Object;
		BallMesh->SetMaterial(0, BaseMaterial);
	}

	CollisionSphere->OnComponentHit.AddDynamic(this, &APoolBall::HandleCollision);
	SetBallRadius(BallRadiusCm);
}

float APoolBall::CalculateShotImpulseMagnitude(float RequestedPower, float ImpulseScale, float MinImpulse, float MaxImpulse)
{
	const float ScaledImpulse = FMath::Max(0.0f, RequestedPower) * FMath::Max(0.0f, ImpulseScale);
	return FMath::Clamp(ScaledImpulse, FMath::Max(0.0f, MinImpulse), FMath::Max(FMath::Max(0.0f, MinImpulse), MaxImpulse));
}

FVector APoolBall::ClampVelocityToTablePlane(const FVector& LinearVelocity, const FVector& UpAxis, float MaxPlanarSpeed)
{
	const FVector SafeUpVector = UpAxis.GetSafeNormal().IsNearlyZero() ? FVector::UpVector : UpAxis.GetSafeNormal();
	const FVector PlanarVelocity = FVector::VectorPlaneProject(LinearVelocity, SafeUpVector);
	const float SafeMaxPlanarSpeed = FMath::Max(0.0f, MaxPlanarSpeed);
	if (SafeMaxPlanarSpeed <= KINDA_SMALL_NUMBER || PlanarVelocity.SizeSquared() <= FMath::Square(SafeMaxPlanarSpeed))
	{
		return PlanarVelocity;
	}

	return PlanarVelocity.GetSafeNormal() * SafeMaxPlanarSpeed;
}

void APoolBall::ComputeElasticCollisionResponse(
	const FVector& VelocityA,
	const FVector& VelocityB,
	const FVector& ContactNormal,
	float Restitution,
	FVector& OutVelocityA,
	FVector& OutVelocityB)
{
	const FVector SafeNormal = ContactNormal.GetSafeNormal();
	if (SafeNormal.IsNearlyZero())
	{
		OutVelocityA = VelocityA;
		OutVelocityB = VelocityB;
		return;
	}

	const float SafeRestitution = FMath::Clamp(Restitution, 0.0f, 1.0f);
	const float VelocityANormal = FVector::DotProduct(VelocityA, SafeNormal);
	const float VelocityBNormal = FVector::DotProduct(VelocityB, SafeNormal);
	const FVector VelocityATangent = VelocityA - SafeNormal * VelocityANormal;
	const FVector VelocityBTangent = VelocityB - SafeNormal * VelocityBNormal;

	const float NewVelocityANormal = ((1.0f - SafeRestitution) * VelocityANormal + (1.0f + SafeRestitution) * VelocityBNormal) * 0.5f;
	const float NewVelocityBNormal = ((1.0f + SafeRestitution) * VelocityANormal + (1.0f - SafeRestitution) * VelocityBNormal) * 0.5f;

	OutVelocityA = VelocityATangent + SafeNormal * NewVelocityANormal;
	OutVelocityB = VelocityBTangent + SafeNormal * NewVelocityBNormal;
}

void APoolBall::EnsurePhysicsMaterial()
{
	if (BallPhysicalMaterial)
	{
		return;
	}

	BallPhysicalMaterial = NewObject<UPhysicalMaterial>(this, TEXT("BallPhysicalMaterial"));
	if (!BallPhysicalMaterial)
	{
		return;
	}

	BallPhysicalMaterial->Friction = ClothFriction;
	BallPhysicalMaterial->Restitution = ClothRestitution;
	BallPhysicalMaterial->bOverrideFrictionCombineMode = true;
	BallPhysicalMaterial->bOverrideRestitutionCombineMode = true;
	BallPhysicalMaterial->FrictionCombineMode = EFrictionCombineMode::Average;
	BallPhysicalMaterial->RestitutionCombineMode = EFrictionCombineMode::Average;
}

void APoolBall::ConfigurePhysicsState()
{
	if (!CollisionSphere || HasAnyFlags(RF_ClassDefaultObject))
	{
		return;
	}

	const FVector SafeUpVector = TableUpVector.GetSafeNormal().IsNearlyZero() ? FVector::UpVector : TableUpVector.GetSafeNormal();
	EnsurePhysicsMaterial();
	CollisionSphere->SetMobility(EComponentMobility::Movable);
	if (BallMesh)
	{
		BallMesh->SetMobility(EComponentMobility::Movable);
	}

	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionSphere->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);
	CollisionSphere->SetNotifyRigidBodyCollision(true);
	CollisionSphere->SetEnableGravity(false);
	CollisionSphere->SetSimulatePhysics(true);
	CollisionSphere->SetLinearDamping(LinearDamping);
	CollisionSphere->SetAngularDamping(AngularDamping);
	CollisionSphere->SetMassOverrideInKg(NAME_None, 0.17f, true);
	CollisionSphere->SetPhysMaterialOverride(BallPhysicalMaterial);
	CollisionSphere->BodyInstance.bUseCCD = true;
	CollisionSphere->BodyInstance.PositionSolverIterationCount = 24;
	CollisionSphere->BodyInstance.VelocitySolverIterationCount = 12;
	CollisionSphere->BodyInstance.CustomDOFPlaneNormal = SafeUpVector;
	CollisionSphere->SetConstraintMode(EDOFMode::CustomPlane);
}

void APoolBall::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	ConfigurePhysicsState();
}

void APoolBall::BeginPlay()
{
	Super::BeginPlay();

	ConfigurePhysicsState();
	InitialTransform = GetActorTransform();
}

void APoolBall::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bSinkingIntoPocket)
	{
		UpdatePocketSink(DeltaTime);
		return;
	}

	if (!CollisionSphere || bPocketed)
	{
		return;
	}

	if (bHasMovementPlane)
	{
		const FVector SafeUpVector = TableUpVector.GetSafeNormal().IsNearlyZero() ? FVector::UpVector : TableUpVector.GetSafeNormal();
		const FVector CurrentLocation = GetActorLocation();
		const float PlaneOffset = FVector::DotProduct(CurrentLocation - MovementPlaneOrigin, SafeUpVector);
		if (FMath::Abs(PlaneOffset) > BallRadiusCm * 0.28f)
		{
			SetActorLocation(CurrentLocation - SafeUpVector * PlaneOffset, false, nullptr, ETeleportType::TeleportPhysics);
		}

		const FVector CorrectedVelocity = ClampVelocityToTablePlane(CollisionSphere->GetPhysicsLinearVelocity(), SafeUpVector, MaxPlanarSpeed);
		CollisionSphere->SetPhysicsLinearVelocity(CorrectedVelocity);
	}

	const FVector LinearVelocity = CollisionSphere->GetPhysicsLinearVelocity();
	const FVector AngularVelocity = CollisionSphere->GetPhysicsAngularVelocityInDegrees();
	if (LinearVelocity.SizeSquared() < FMath::Square(SleepVelocityThreshold) && AngularVelocity.SizeSquared() < FMath::Square(18.0f))
	{
		CollisionSphere->SetPhysicsLinearVelocity(FVector::ZeroVector);
		CollisionSphere->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		ActiveSpinInput = FVector2D::ZeroVector;
		ActiveSpinAxis = FVector::ZeroVector;
		return;
	}

	if (!ActiveSpinInput.IsNearlyZero() && !LinearVelocity.IsNearlyZero())
	{
		const FVector ShotDirection = LinearVelocity.GetSafeNormal();
		const FVector SideAxis = FVector::CrossProduct(TableUpVector, ShotDirection).GetSafeNormal();
		const FVector SideForce = SideAxis * (ActiveSpinInput.X * SideSpinForceScale * LinearVelocity.Size());
		const FVector FollowDrawForce = ShotDirection * (ActiveSpinInput.Y * TopSpinForceScale * LinearVelocity.Size());
		CollisionSphere->AddForce(SideForce + FollowDrawForce);
		ActiveSpinInput = FMath::Vector2DInterpTo(ActiveSpinInput, FVector2D::ZeroVector, DeltaTime, SpinDamping);
	}
}

void APoolBall::SetBallRadius(float InRadius)
{
	BallRadiusCm = FMath::Max(1.0f, InRadius);

	if (CollisionSphere)
	{
		CollisionSphere->SetSphereRadius(BallRadiusCm);
	}

	UpdateVisualMeshTransform();
}

void APoolBall::SetMovementPlane(const FVector& InTableUpAxis, const FVector& InPlaneOrigin)
{
	TableUpVector = InTableUpAxis.GetSafeNormal().IsNearlyZero() ? FVector::UpVector : InTableUpAxis.GetSafeNormal();
	MovementPlaneOrigin = InPlaneOrigin;
	bHasMovementPlane = true;

	if (CollisionSphere)
	{
		CollisionSphere->BodyInstance.CustomDOFPlaneNormal = TableUpVector;
		CollisionSphere->SetConstraintMode(EDOFMode::CustomPlane);
	}
}

FVector APoolBall::GetLinearVelocity() const
{
	return CollisionSphere ? CollisionSphere->GetPhysicsLinearVelocity() : FVector::ZeroVector;
}

void APoolBall::SetLinearVelocity(const FVector& NewVelocity)
{
	if (CollisionSphere)
	{
		CollisionSphere->SetPhysicsLinearVelocity(ClampVelocityToTablePlane(NewVelocity, TableUpVector, MaxPlanarSpeed));
		if (!NewVelocity.IsNearlyZero())
		{
			CollisionSphere->WakeRigidBody();
		}
	}
}

FVector APoolBall::GetAngularVelocityDegrees() const
{
	return CollisionSphere ? CollisionSphere->GetPhysicsAngularVelocityInDegrees() : FVector::ZeroVector;
}

void APoolBall::SetAngularVelocityDegrees(const FVector& NewVelocity)
{
	if (CollisionSphere)
	{
		CollisionSphere->SetPhysicsAngularVelocityInDegrees(NewVelocity);
		if (!NewVelocity.IsNearlyZero())
		{
			CollisionSphere->WakeRigidBody();
		}
	}
}

void APoolBall::TeleportBall(const FVector& NewLocation)
{
	SetActorLocation(NewLocation, false, nullptr, ETeleportType::TeleportPhysics);
}

void APoolBall::UpdateVisualMeshTransform()
{
	if (!BallMesh)
	{
		return;
	}

	const UStaticMesh* ActiveMesh = BallMesh->GetStaticMesh();
	const FBoxSphereBounds MeshBounds = ActiveMesh ? ActiveMesh->GetBounds() : FBoxSphereBounds(EForceInit::ForceInitToZero);
	const float MeshRadius = FMath::Max(0.01f, MeshBounds.SphereRadius);
	const float MeshScale = BallRadiusCm / MeshRadius;

	BallMesh->SetRelativeScale3D(FVector(MeshScale));
	BallMesh->SetRelativeLocation(-(MeshBounds.Origin * MeshScale));
}

void APoolBall::ConfigureBall(bool bInCueBall, int32 InBallNumber, const FLinearColor& InColor)
{
	bCueBall = bInCueBall;
	BallNumber = InBallNumber;
	bUsingImportedVisual = false;

	if (UStaticMesh* ImportedMesh = ResolveImportedBallMesh(bCueBall, BallNumber))
	{
		BallMesh->SetStaticMesh(ImportedMesh);
		BallMesh->EmptyOverrideMaterials();
		bUsingImportedVisual = true;
		SetBallRadius(BallRadiusCm);
		return;
	}

	ApplyFallbackColor(InColor);
	SetBallRadius(BallRadiusCm);
}

UStaticMesh* APoolBall::ResolveImportedBallMesh(bool bInCueBall, int32 InBallNumber) const
{
	const FString AssetName = bInCueBall
		? TEXT("CueBall_Source")
		: FString::Printf(TEXT("Ball_%02d_Source"), InBallNumber);

	const FString AssetPath = FString::Printf(
		TEXT("/Game/Billiards/Balls/%s.%s"),
		*AssetName,
		*AssetName);

	return LoadObject<UStaticMesh>(nullptr, *AssetPath);
}

void APoolBall::ApplyFallbackColor(const FLinearColor& InColor)
{
	if (!BaseMaterial)
	{
		return;
	}

	BallMesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
	BallMesh->SetMaterial(0, BaseMaterial);
	if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMaterial, this))
	{
		MID->SetVectorParameterValue(TEXT("Color"), InColor);
		MID->SetVectorParameterValue(TEXT("BaseColor"), InColor);
		BallMesh->SetMaterial(0, MID);
	}
}

void APoolBall::BeginPocketSink(const FVector& SinkTargetLocation)
{
	if (bPocketed || !CollisionSphere)
	{
		return;
	}

	bPocketed = true;
	bSinkingIntoPocket = true;
	PocketSinkAlpha = 0.0f;
	PocketSinkStart = GetActorLocation();
	PocketSinkTarget = SinkTargetLocation;
	const FVector PlanarToTarget = FVector::VectorPlaneProject(PocketSinkTarget - PocketSinkStart, TableUpVector);
	PocketSinkControlPoint = PocketSinkStart + PlanarToTarget * 0.42f - TableUpVector * (BallRadiusCm * 0.18f);
	PocketSinkSpinAxis = CollisionSphere->GetPhysicsAngularVelocityInDegrees().GetSafeNormal();
	if (PocketSinkSpinAxis.IsNearlyZero())
	{
		PocketSinkSpinAxis = FVector::CrossProduct(TableUpVector, PlanarToTarget.GetSafeNormal()).GetSafeNormal();
		if (PocketSinkSpinAxis.IsNearlyZero())
		{
			PocketSinkSpinAxis = FVector::ForwardVector;
		}
	}
	PocketSinkSpinSpeedDegrees = FMath::Max(420.0f, CollisionSphere->GetPhysicsAngularVelocityInDegrees().Size());
	CollisionSphere->SetPhysicsLinearVelocity(FVector::ZeroVector);
	CollisionSphere->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	CollisionSphere->SetSimulatePhysics(false);
	SetActorEnableCollision(false);
}

void APoolBall::UpdatePocketSink(float DeltaTime)
{
	PocketSinkAlpha = FMath::Min(1.0f, PocketSinkAlpha + (DeltaTime / FMath::Max(0.01f, PocketSinkDuration)));
	const float T = FMath::InterpEaseInOut(0.0f, 1.0f, PocketSinkAlpha, 2.0f);
	const float OneMinusT = 1.0f - T;
	const FVector SinkLocation =
		(OneMinusT * OneMinusT * PocketSinkStart)
		+ (2.0f * OneMinusT * T * PocketSinkControlPoint)
		+ (T * T * PocketSinkTarget);

	SetActorLocation(SinkLocation, false, nullptr, ETeleportType::TeleportPhysics);
	const float SpinStep = PocketSinkSpinSpeedDegrees * DeltaTime * FMath::Lerp(1.0f, 0.45f, T);
	AddActorWorldRotation(FQuat(PocketSinkSpinAxis, FMath::DegreesToRadians(SpinStep)));

	if (PocketSinkAlpha >= 1.0f)
	{
		bSinkingIntoPocket = false;
		PocketBall();
	}
}

void APoolBall::ResetBall(const FTransform& NewTransform)
{
	InitialTransform = NewTransform;
	bPocketed = false;
	bSinkingIntoPocket = false;
	PocketSinkAlpha = 0.0f;
	ActiveSpinInput = FVector2D::ZeroVector;
	ActiveSpinAxis = FVector::ZeroVector;
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	BallMesh->SetVisibility(true, true);
	CollisionSphere->SetSimulatePhysics(false);
	ConfigurePhysicsState();
	SetActorTransform(InitialTransform, false, nullptr, ETeleportType::TeleportPhysics);
	if (bHasMovementPlane)
	{
		const float PlaneOffset = FVector::DotProduct(GetActorLocation() - MovementPlaneOrigin, TableUpVector);
		SetActorLocation(GetActorLocation() - TableUpVector * PlaneOffset, false, nullptr, ETeleportType::TeleportPhysics);
	}
	CollisionSphere->SetPhysicsLinearVelocity(FVector::ZeroVector);
	CollisionSphere->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	CollisionSphere->WakeRigidBody();
}

void APoolBall::PocketBall()
{
	if (bPocketed && !bSinkingIntoPocket && IsHidden())
	{
		return;
	}

	bPocketed = true;
	bSinkingIntoPocket = false;
	ActiveSpinInput = FVector2D::ZeroVector;
	ActiveSpinAxis = FVector::ZeroVector;
	CollisionSphere->SetPhysicsLinearVelocity(FVector::ZeroVector);
	CollisionSphere->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	CollisionSphere->SetSimulatePhysics(false);
	SetActorEnableCollision(false);
	BallMesh->SetVisibility(false, true);
	SetActorHiddenInGame(true);
}

bool APoolBall::IsMoving() const
{
	if (!CollisionSphere || bPocketed)
	{
		return false;
	}

	return CollisionSphere->GetPhysicsLinearVelocity().SizeSquared() > FMath::Square(SleepVelocityThreshold)
		|| CollisionSphere->GetPhysicsAngularVelocityInDegrees().SizeSquared() > FMath::Square(12.0f);
}

void APoolBall::ApplyShotImpulse(const FVector& Direction, float Power, const FVector2D& SpinInput, const FVector& TableUpAxis)
{
	if (!CollisionSphere || bPocketed)
	{
		return;
	}

	const FVector ShotDirection = Direction.GetSafeNormal();
	const FVector UpAxis = TableUpAxis.GetSafeNormal().IsNearlyZero() ? FVector::UpVector : TableUpAxis.GetSafeNormal();
	const FVector SideAxis = FVector::CrossProduct(UpAxis, ShotDirection).GetSafeNormal();
	const float ShotImpulse = CalculateShotImpulseMagnitude(Power, ShotImpulseScale, MinShotImpulse, MaxShotImpulse);

	TableUpVector = UpAxis;
	ActiveSpinInput = FVector2D(
		FMath::Clamp(SpinInput.X, -1.0f, 1.0f),
		FMath::Clamp(SpinInput.Y, -1.0f, 1.0f));
	ActiveSpinAxis = (SideAxis * ActiveSpinInput.Y + UpAxis * ActiveSpinInput.X).GetSafeNormal();

	CollisionSphere->WakeRigidBody();
	CollisionSphere->AddImpulse(ShotDirection * ShotImpulse, NAME_None, true);

	if (!ActiveSpinInput.IsNearlyZero())
	{
		const FVector AngularImpulse = (SideAxis * ActiveSpinInput.Y + UpAxis * ActiveSpinInput.X) * (ShotImpulse * 0.11f);
		CollisionSphere->AddAngularImpulseInDegrees(AngularImpulse, NAME_None, true);
	}
}

void APoolBall::HandleCollision(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	APoolBall* OtherBall = Cast<APoolBall>(OtherActor);
	if (!OtherBall || OtherBall == this || bPocketed || bSinkingIntoPocket || OtherBall->bPocketed || OtherBall->bSinkingIntoPocket)
	{
		return;
	}

	ApplyBallCollisionResponse(OtherBall);
}

void APoolBall::ApplyBallCollisionResponse(APoolBall* OtherBall)
{
	if (!OtherBall || !CollisionSphere || !OtherBall->CollisionSphere)
	{
		return;
	}

	if (LastCollisionBall == OtherBall && LastCollisionFrame == GFrameCounter)
	{
		return;
	}

	const FVector SafeUpVector = TableUpVector.GetSafeNormal().IsNearlyZero() ? FVector::UpVector : TableUpVector.GetSafeNormal();
	const FVector Delta = FVector::VectorPlaneProject(OtherBall->GetActorLocation() - GetActorLocation(), SafeUpVector);
	const FVector ContactNormal = Delta.GetSafeNormal();
	if (ContactNormal.IsNearlyZero())
	{
		return;
	}

	const FVector VelocityA = ClampVelocityToTablePlane(GetLinearVelocity(), SafeUpVector, MaxPlanarSpeed);
	const FVector VelocityB = ClampVelocityToTablePlane(OtherBall->GetLinearVelocity(), SafeUpVector, OtherBall->MaxPlanarSpeed);
	const float ClosingSpeed = FVector::DotProduct(VelocityA - VelocityB, ContactNormal);
	if (ClosingSpeed <= BallCollisionMinClosingSpeed)
	{
		return;
	}

	FVector NewVelocityA = FVector::ZeroVector;
	FVector NewVelocityB = FVector::ZeroVector;
	ComputeElasticCollisionResponse(VelocityA, VelocityB, ContactNormal, BallCollisionRestitution, NewVelocityA, NewVelocityB);

	const bool bOtherBallWasNearlyStill = VelocityB.SizeSquared() < FMath::Square(18.0f);
	if (bOtherBallWasNearlyStill)
	{
		const float NormalSpeedB = FMath::Max(0.0f, FVector::DotProduct(NewVelocityB, ContactNormal));
		const FVector NormalVelocityB = ContactNormal * NormalSpeedB;
		const FVector TangentialVelocityB = NewVelocityB - NormalVelocityB;
		NewVelocityB = NormalVelocityB + TangentialVelocityB * FMath::Clamp(BallCollisionStruckBallTangentDamping, 0.0f, 1.0f);
		NewVelocityA += TangentialVelocityB * (1.0f - FMath::Clamp(BallCollisionStruckBallTangentDamping, 0.0f, 1.0f));
	}

	SetLinearVelocity(NewVelocityA);
	OtherBall->SetLinearVelocity(NewVelocityB);

	const float DesiredSeparation = BallRadiusCm + OtherBall->BallRadiusCm;
	const float Penetration = FMath::Max(0.0f, DesiredSeparation - Delta.Size());
	if (Penetration > KINDA_SMALL_NUMBER)
	{
		const FVector Separation = ContactNormal * (Penetration * 0.52f + 0.03f);
		TeleportBall(GetActorLocation() - Separation);
		OtherBall->TeleportBall(OtherBall->GetActorLocation() + Separation);
	}

	LastCollisionBall = OtherBall;
	LastCollisionFrame = GFrameCounter;
	OtherBall->LastCollisionBall = this;
	OtherBall->LastCollisionFrame = GFrameCounter;
	ActiveSpinInput *= 0.55f;
	OtherBall->ActiveSpinInput *= 0.2f;
}

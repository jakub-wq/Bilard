#include "PoolBall.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

APoolBall::APoolBall()
{
	PrimaryActorTick.bCanEverTick = true;

	BallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BallMesh"));
	SetRootComponent(BallMesh);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		BallMesh->SetStaticMesh(SphereMesh.Object);
	}

	BallMesh->SetSimulatePhysics(true);
	BallMesh->SetEnableGravity(true);
	BallMesh->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
	BallMesh->SetNotifyRigidBodyCollision(true);
	BallMesh->SetLinearDamping(LinearDamping);
	BallMesh->SetAngularDamping(AngularDamping);
	BallMesh->SetMassOverrideInKg(NAME_None, 0.17f, true);
	BallMesh->BodyInstance.bUseCCD = true;
	BallMesh->SetConstraintMode(EDOFMode::SixDOF);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (BasicMaterial.Succeeded())
	{
		BaseMaterial = BasicMaterial.Object;
		BallMesh->SetMaterial(0, BaseMaterial);
	}

	SetBallRadius(BallRadiusCm);
}

void APoolBall::BeginPlay()
{
	Super::BeginPlay();
	InitialTransform = GetActorTransform();
}

void APoolBall::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (BallMesh && !bPocketed)
	{
		const FVector LinearVelocity = BallMesh->GetPhysicsLinearVelocity();
		const FVector AngularVelocity = BallMesh->GetPhysicsAngularVelocityInDegrees();
		if (LinearVelocity.SizeSquared() < FMath::Square(SleepVelocityThreshold) && AngularVelocity.SizeSquared() < FMath::Square(18.0f))
		{
			BallMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
			BallMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		}
	}
}

void APoolBall::SetBallRadius(float InRadius)
{
	BallRadiusCm = FMath::Max(1.0f, InRadius);
	const float SphereScale = (BallRadiusCm * 2.0f) / 100.0f;
	BallMesh->SetWorldScale3D(FVector(SphereScale));
}

void APoolBall::ConfigureBall(bool bInCueBall, int32 InBallNumber, const FLinearColor& InColor)
{
	bCueBall = bInCueBall;
	BallNumber = InBallNumber;

	if (BaseMaterial)
	{
		if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMaterial, this))
		{
			MID->SetVectorParameterValue(TEXT("Color"), InColor);
			MID->SetVectorParameterValue(TEXT("BaseColor"), InColor);
			BallMesh->SetMaterial(0, MID);
		}
	}
}

void APoolBall::ResetBall(const FTransform& NewTransform)
{
	InitialTransform = NewTransform;
	bPocketed = false;
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	BallMesh->SetVisibility(true, true);
	BallMesh->SetSimulatePhysics(false);
	SetActorTransform(InitialTransform, false, nullptr, ETeleportType::TeleportPhysics);
	BallMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
	BallMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	BallMesh->SetSimulatePhysics(true);
	BallMesh->WakeRigidBody();
}

void APoolBall::PocketBall()
{
	if (bPocketed)
	{
		return;
	}

	bPocketed = true;
	BallMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
	BallMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	BallMesh->SetSimulatePhysics(false);
	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);
}

bool APoolBall::IsMoving() const
{
	if (!BallMesh || bPocketed)
	{
		return false;
	}

	return BallMesh->GetPhysicsLinearVelocity().SizeSquared() > FMath::Square(SleepVelocityThreshold);
}

void APoolBall::ApplyShotImpulse(const FVector& Direction, float Power)
{
	if (!BallMesh || bPocketed)
	{
		return;
	}

	BallMesh->WakeRigidBody();
	BallMesh->AddImpulse(Direction.GetSafeNormal() * Power, NAME_None, true);
}

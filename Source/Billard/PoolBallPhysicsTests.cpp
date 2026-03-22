#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "PoolBall.h"

namespace PoolBallPhysicsTests
{
	constexpr float Tolerance = 0.01f;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPoolBallShotImpulseScalingTest,
	"Billard.Physics.ShotImpulseScaling",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FPoolBallShotImpulseScalingTest::RunTest(const FString& Parameters)
{
	using namespace PoolBallPhysicsTests;

	const float MinImpulse = 52.0f;
	const float MaxImpulse = 350.0f;
	const float ImpulseScale = 0.24f;

	TestTrue(
		TEXT("Maksymalny strzał powinien mapować się do realistycznego impulsu, a nie skrajnie wysokiej wartości."),
		FMath::IsNearlyEqual(APoolBall::CalculateShotImpulseMagnitude(1400.0f, ImpulseScale, MinImpulse, MaxImpulse), 336.0f, Tolerance));
	TestTrue(
		TEXT("Słaby strzał nie powinien spaść poniżej minimalnego impulsu."),
		FMath::IsNearlyEqual(APoolBall::CalculateShotImpulseMagnitude(50.0f, ImpulseScale, MinImpulse, MaxImpulse), MinImpulse, Tolerance));
	TestTrue(
		TEXT("Impuls nie powinien przekraczać ustawionego maksimum."),
		FMath::IsNearlyEqual(APoolBall::CalculateShotImpulseMagnitude(5000.0f, ImpulseScale, MinImpulse, MaxImpulse), MaxImpulse, Tolerance));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPoolBallVelocityClampTest,
	"Billard.Physics.VelocityClamp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FPoolBallVelocityClampTest::RunTest(const FString& Parameters)
{
	using namespace PoolBallPhysicsTests;

	const FVector InputVelocity(1800.0f, 120.0f, 90.0f);
	const FVector Clamped = APoolBall::ClampVelocityToTablePlane(InputVelocity, FVector::UpVector, 900.0f);

	TestTrue(
		TEXT("Clamp prędkości powinien usuwać składową pionową."),
		FMath::IsNearlyZero(Clamped.Z, Tolerance));
	TestTrue(
		TEXT("Clamp prędkości powinien ograniczać prędkość planarą do maksimum."),
		Clamped.Size() <= 900.0f + Tolerance);
	TestTrue(
		TEXT("Clamp nie powinien odwracać kierunku ruchu."),
		FVector::DotProduct(Clamped.GetSafeNormal(), FVector(InputVelocity.X, InputVelocity.Y, 0.0f).GetSafeNormal()) > 0.999f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPoolBallAngledCollisionResponseTest,
	"Billard.Physics.AngledCollisionResponse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FPoolBallAngledCollisionResponseTest::RunTest(const FString& Parameters)
{
	using namespace PoolBallPhysicsTests;

	const FVector VelocityA(140.0f, 55.0f, 0.0f);
	const FVector VelocityB = FVector::ZeroVector;
	const FVector ContactNormal = FVector::ForwardVector;
	FVector OutVelocityA = FVector::ZeroVector;
	FVector OutVelocityB = FVector::ZeroVector;

	APoolBall::ComputeElasticCollisionResponse(VelocityA, VelocityB, ContactNormal, 1.0f, OutVelocityA, OutVelocityB);

	TestTrue(
		TEXT("W zderzeniu równych bil trafiona bila powinna przejąć praktycznie cały składnik ruchu wzdłuż normalnej kontaktu."),
		FMath::IsNearlyEqual(OutVelocityB.X, VelocityA.X, Tolerance));
	TestTrue(
		TEXT("Składnik styczny powinien pozostać głównie na bili uderzającej."),
		FMath::IsNearlyEqual(OutVelocityA.Y, VelocityA.Y, Tolerance));
	TestTrue(
		TEXT("Trafiona bila nie powinna dostawać sztucznego odchylenia bocznego w idealnym modelu zderzenia."),
		FMath::IsNearlyZero(OutVelocityB.Y, Tolerance));

	return true;
}

#endif

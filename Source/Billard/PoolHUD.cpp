#include "PoolHUD.h"

#include "Engine/Canvas.h"

void APoolHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	const FVector2D Center(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f);
	const float Gap = 2.0f;

	DrawLine(Center.X - CrosshairHalfSize, Center.Y, Center.X - Gap, Center.Y, CrosshairColor, CrosshairThickness);
	DrawLine(Center.X + Gap, Center.Y, Center.X + CrosshairHalfSize, Center.Y, CrosshairColor, CrosshairThickness);
	DrawLine(Center.X, Center.Y - CrosshairHalfSize, Center.X, Center.Y - Gap, CrosshairColor, CrosshairThickness);
	DrawLine(Center.X, Center.Y + Gap, Center.X, Center.Y + CrosshairHalfSize, CrosshairColor, CrosshairThickness);
}

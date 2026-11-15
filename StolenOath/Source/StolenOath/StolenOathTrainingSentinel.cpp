// Copyright Epic Games, Inc. All Rights Reserved.

#include "StolenOathTrainingSentinel.h"

#include "ProceduralMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "StolenOathCharacter.h"

namespace
{
	void AddAnnularStrip(
		TArray<FVector>& Vertices,
		TArray<int32>& Triangles,
		const TFunction<FVector(float, float)>& MakeVertex,
		float InnerRadius,
		float OuterRadius,
		float StartAngle,
		float EndAngle,
		int32 Segments)
	{
		const int32 SafeSegments = FMath::Max(1, Segments);
		for (int32 Index = 0; Index < SafeSegments; ++Index)
		{
			const float A0 = FMath::Lerp(StartAngle, EndAngle, static_cast<float>(Index) / SafeSegments);
			const float A1 = FMath::Lerp(StartAngle, EndAngle, static_cast<float>(Index + 1) / SafeSegments);
			const int32 Base = Vertices.Num();
			Vertices.Add(MakeVertex(InnerRadius, A0));
			Vertices.Add(MakeVertex(OuterRadius, A0));
			Vertices.Add(MakeVertex(OuterRadius, A1));
			Vertices.Add(MakeVertex(InnerRadius, A1));
			Triangles.Append({ Base, Base + 1, Base + 2, Base, Base + 2, Base + 3 });
		}
	}

	void AddRibbon(
		TArray<FVector>& Vertices,
		TArray<int32>& Triangles,
		const TFunction<FVector(float, float)>& MakeVertex,
		float StartRadius,
		float EndRadius,
		float Angle,
		float HalfWidth)
	{
		const int32 Base = Vertices.Num();
		Vertices.Add(MakeVertex(StartRadius, Angle - HalfWidth));
		Vertices.Add(MakeVertex(EndRadius, Angle - HalfWidth));
		Vertices.Add(MakeVertex(EndRadius, Angle + HalfWidth));
		Vertices.Add(MakeVertex(StartRadius, Angle + HalfWidth));
		Triangles.Append({ Base, Base + 1, Base + 2, Base, Base + 2, Base + 3 });
	}

	void CreateWarningMeshSection(
		UProceduralMeshComponent* Mesh,
		int32 SectionIndex,
		const TArray<FVector>& Vertices,
		const TArray<int32>& Triangles,
		UMaterialInterface* Material)
	{
		if (!IsValid(Mesh) || !IsValid(Material) || Vertices.Num() < 3 || Triangles.Num() < 3)
		{
			return;
		}

		TArray<FVector> Normals;
		TArray<FVector2D> UVs;
		TArray<FLinearColor> Colors;
		TArray<FProcMeshTangent> Tangents;
		TArray<int32> DoubleSidedTriangles = Triangles;
		DoubleSidedTriangles.Reserve(Triangles.Num() * 2);
		for (int32 TriangleIndex = 0; TriangleIndex + 2 < Triangles.Num(); TriangleIndex += 3)
		{
			// Keep a reverse-wound copy so the translucent surface renders from
			// either side, regardless of ground slope or view direction.
			DoubleSidedTriangles.Add(Triangles[TriangleIndex]);
			DoubleSidedTriangles.Add(Triangles[TriangleIndex + 2]);
			DoubleSidedTriangles.Add(Triangles[TriangleIndex + 1]);
		}
		Normals.Init(FVector::UpVector, Vertices.Num());
		Colors.Init(FLinearColor::White, Vertices.Num());
		Tangents.Init(FProcMeshTangent(1.0f, 0.0f, 0.0f), Vertices.Num());
		UVs.Reserve(Vertices.Num());
		for (const FVector& Vertex : Vertices)
		{
			UVs.Add(FVector2D(Vertex.X / 100.0f, Vertex.Y / 100.0f));
		}

		Mesh->CreateMeshSection_LinearColor(SectionIndex, Vertices, DoubleSidedTriangles, Normals, UVs, Colors, Tangents, false);
		Mesh->SetMaterial(SectionIndex, Material);
	}
}

AStolenOathTrainingSentinel::AStolenOathTrainingSentinel()
{
	PrimaryActorTick.bCanEverTick = true;

	SentinelMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SentinelMesh"));
	SetRootComponent(SentinelMesh);
	SentinelMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	SentinelMesh->SetMobility(EComponentMobility::Movable);
	SentinelMesh->SetRelativeScale3D(BaseMeshScale);
	SentinelMesh->bNavigationRelevant = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		SentinelMesh->SetStaticMesh(CubeMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicShapeMaterial(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial_Inst.BasicShapeMaterial_Inst"));
	if (BasicShapeMaterial.Succeeded())
	{
		SentinelBaseMaterial = BasicShapeMaterial.Object;
		SentinelMesh->SetMaterial(0, SentinelBaseMaterial);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WarningShapeMaterial(
		TEXT("/Engine/EngineDebugMaterials/M_SimpleUnlitTranslucent.M_SimpleUnlitTranslucent"));
	if (WarningShapeMaterial.Succeeded())
	{
		WarningMaterial = WarningShapeMaterial.Object;
	}

}

void AStolenOathTrainingSentinel::BeginPlay()
{
	Super::BeginPlay();
	CurrentHealth = MaxHealth;
	bPhaseTwoActive = false;
	bUseSweepNext = false;
	bCurrentAttackIsSweep = false;
	bInterruptedForSpecial = false;
	GetWorldTimerManager().ClearTimer(AttackTimerHandle);
	GetWorldTimerManager().ClearTimer(StrikeTimerHandle);
	GetWorldTimerManager().ClearTimer(ResetTimerHandle);
	SentinelMesh->SetVisibility(true, true);
	SentinelMesh->SetHiddenInGame(false, true);
	if (SentinelBaseMaterial)
	{
		SentinelDynamicMaterial = UMaterialInstanceDynamic::Create(SentinelBaseMaterial, this);
		if (SentinelDynamicMaterial)
		{
			SentinelDynamicMaterial->SetVectorParameterValue(TEXT("Color"), PhaseOneColor);
			SentinelMesh->SetMaterial(0, SentinelDynamicMaterial);
		}
	}

	CreateWarningMeshes();

	GetWorldTimerManager().SetTimer(AttackTimerHandle, this, &AStolenOathTrainingSentinel::TelegraphStrike, InitialAttackDelay, false);
}

FVector AStolenOathTrainingSentinel::ProjectWarningPointToGround(
	UProceduralMeshComponent* Mesh,
	float X,
	float Y,
	float SurfaceOffset) const
{
	if (!IsValid(Mesh) || !GetWorld())
	{
		return FVector(X, Y, 0.0f);
	}

	const FTransform MeshTransform = Mesh->GetComponentTransform();
	const FVector WorldPoint = MeshTransform.TransformPosition(FVector(X, Y, 0.0f));
	FVector SurfacePoint = WorldPoint;
	FVector SurfaceNormal = FVector::UpVector;
	FHitResult GroundHit;
	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(SentinelWarningProjection), false, this);
	if (const ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0))
	{
		TraceParams.AddIgnoredActor(Player);
	}
	const FVector TraceStart = WorldPoint + FVector::UpVector * 600.0f;
	const FVector TraceEnd = WorldPoint - FVector::UpVector * 2000.0f;
	if (GetWorld()->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_WorldStatic, TraceParams))
	{
		SurfacePoint = GroundHit.ImpactPoint;
		SurfaceNormal = GroundHit.ImpactNormal.GetSafeNormal();
	}
	SurfacePoint += SurfaceNormal * SurfaceOffset;
	return MeshTransform.InverseTransformPosition(SurfacePoint);
}

void AStolenOathTrainingSentinel::CreateWarningMeshes()
{
	if (!WarningMaterial)
	{
		return;
	}

	auto CreateWarning = [this](const TCHAR* ObjectName, TObjectPtr<UProceduralMeshComponent>& OutMesh,
		TObjectPtr<UMaterialInstanceDynamic>& OutFillMaterial,
		TObjectPtr<UMaterialInstanceDynamic>& OutEdgeMaterial,
		const FLinearColor& Color)
	{
		OutMesh = NewObject<UProceduralMeshComponent>(this, ObjectName);
		OutMesh->SetupAttachment(RootComponent);
		OutMesh->SetAbsolute(true, true, true);
		OutMesh->SetMobility(EComponentMobility::Movable);
		OutMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		OutMesh->SetGenerateOverlapEvents(false);
		OutMesh->SetCanEverAffectNavigation(false);
		OutMesh->SetCastShadow(false);
		OutMesh->bUseAsyncCooking = false;
		AddInstanceComponent(OutMesh);
		OutMesh->RegisterComponent();

		OutFillMaterial = UMaterialInstanceDynamic::Create(WarningMaterial, this);
		OutEdgeMaterial = UMaterialInstanceDynamic::Create(WarningMaterial, this);
		if (OutFillMaterial)
		{
			FLinearColor FillColor = Color;
			FillColor.A = FMath::Clamp(Color.A, 0.15f, 0.35f);
			OutFillMaterial->SetVectorParameterValue(TEXT("Color"), FillColor);
			OutMesh->SetMaterial(0, OutFillMaterial);
		}
		if (OutEdgeMaterial)
		{
			FLinearColor EdgeColor = Color;
			EdgeColor.A = 1.0f;
			OutEdgeMaterial->SetVectorParameterValue(TEXT("Color"), EdgeColor);
			OutMesh->SetMaterial(1, OutEdgeMaterial);
		}
		OutMesh->SetVisibility(false, true);
		OutMesh->SetHiddenInGame(true, true);
	};

	CreateWarning(TEXT("OverheadStrikeWarning"), OverheadWarningMesh,
		OverheadWarningFillMaterial, OverheadWarningEdgeMaterial, OverheadWarningColor);
	CreateWarning(TEXT("SweepAttackWarning"), SweepWarningMesh,
		SweepWarningFillMaterial, SweepWarningEdgeMaterial, SweepWarningColor);
}

void AStolenOathTrainingSentinel::SetWarningMeshesVisible(bool bVisible)
{
	if (IsValid(OverheadWarningMesh))
	{
		OverheadWarningMesh->SetVisibility(bVisible, true);
		OverheadWarningMesh->SetHiddenInGame(!bVisible, true);
	}
	if (IsValid(SweepWarningMesh))
	{
		SweepWarningMesh->SetVisibility(bVisible, true);
		SweepWarningMesh->SetHiddenInGame(!bVisible, true);
	}
}

void AStolenOathTrainingSentinel::BuildPolarGrid(
	UProceduralMeshComponent* Mesh,
	float InnerRadius,
	float OuterRadius,
	float StartAngle,
	float EndAngle,
	bool bWrapAngle,
	UMaterialInterface* Material)
{
	if (!IsValid(Mesh))
	{
		return;
	}

	const int32 AngularSegments = FMath::Clamp(WarningArcSegments, 24, 128);
	const int32 RadialSegments = FMath::Clamp(WarningRadialSubdivisions, 2, 32);
	const int32 AngularPoints = AngularSegments + (bWrapAngle ? 0 : 1);
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	Vertices.Reserve((RadialSegments + 1) * AngularPoints);
	Triangles.Reserve(RadialSegments * AngularSegments * 6);

	for (int32 RadialIndex = 0; RadialIndex <= RadialSegments; ++RadialIndex)
	{
		const float Alpha = static_cast<float>(RadialIndex) / RadialSegments;
		const float Radius = FMath::Lerp(InnerRadius, OuterRadius, Alpha);
		for (int32 AngularIndex = 0; AngularIndex < AngularPoints; ++AngularIndex)
		{
			const float AngleAlpha = static_cast<float>(AngularIndex) / AngularSegments;
			const float Angle = FMath::Lerp(StartAngle, EndAngle, AngleAlpha);
			Vertices.Add(ProjectWarningPointToGround(
				Mesh,
				Radius * FMath::Cos(Angle),
				Radius * FMath::Sin(Angle),
				WarningGroundOffset));
		}
	}

	for (int32 RadialIndex = 0; RadialIndex < RadialSegments; ++RadialIndex)
	{
		for (int32 AngularIndex = 0; AngularIndex < AngularSegments; ++AngularIndex)
		{
			const int32 NextAngularIndex = bWrapAngle ? (AngularIndex + 1) % AngularPoints : AngularIndex + 1;
			const int32 InnerStart = RadialIndex * AngularPoints + AngularIndex;
			const int32 OuterStart = (RadialIndex + 1) * AngularPoints + AngularIndex;
			const int32 OuterNext = (RadialIndex + 1) * AngularPoints + NextAngularIndex;
			const int32 InnerNext = RadialIndex * AngularPoints + NextAngularIndex;
			Triangles.Append({ InnerStart, OuterStart, OuterNext, InnerStart, OuterNext, InnerNext });
		}
	}

	CreateWarningMeshSection(Mesh, 0, Vertices, Triangles, Material);
}

void AStolenOathTrainingSentinel::RebuildOverheadWarning()
{
	if (!IsValid(OverheadWarningMesh))
	{
		return;
	}

	OverheadWarningMesh->ClearAllMeshSections();
	const float Radius = FMath::Max(1.0f, OverheadAttackRadius);
	const float EdgeWidth = FMath::Clamp(WarningEdgeWidth, 2.0f, Radius * 0.2f);
	auto MakeVertex = [this](UProceduralMeshComponent* Mesh, float R, float Angle, float Offset)
	{
		return ProjectWarningPointToGround(Mesh, R * FMath::Cos(Angle), R * FMath::Sin(Angle), Offset);
	};

	BuildPolarGrid(OverheadWarningMesh, 0.0f, Radius - EdgeWidth * 0.5f,
		0.0f, 2.0f * PI, true, OverheadWarningFillMaterial);

	TArray<FVector> EdgeVertices;
	TArray<int32> EdgeTriangles;
	AddAnnularStrip(EdgeVertices, EdgeTriangles,
		[&](float R, float A) { return MakeVertex(OverheadWarningMesh, R, A, WarningGroundOffset + 2.0f); },
		Radius - EdgeWidth, Radius, 0.0f, 2.0f * PI, FMath::Clamp(WarningArcSegments, 24, 128));
	const int32 DashCount = 10;
	const float DashArc = 2.0f * PI / DashCount * 0.52f;
	for (int32 Dash = 0; Dash < DashCount; ++Dash)
	{
		const float CenterAngle = (2.0f * PI * Dash) / DashCount;
		AddAnnularStrip(EdgeVertices, EdgeTriangles,
			[&](float R, float A) { return MakeVertex(OverheadWarningMesh, R, A, WarningGroundOffset + 2.0f); },
			Radius * 0.63f - 2.5f, Radius * 0.63f + 2.5f,
			CenterAngle - DashArc * 0.5f, CenterAngle + DashArc * 0.5f, 3);
	}
	CreateWarningMeshSection(OverheadWarningMesh, 1, EdgeVertices, EdgeTriangles, OverheadWarningEdgeMaterial);
}

void AStolenOathTrainingSentinel::RebuildSweepWarning()
{
	if (!IsValid(SweepWarningMesh))
	{
		return;
	}

	SweepWarningMesh->ClearAllMeshSections();
	const float Range = FMath::Max(1.0f, SweepAttackRange);
	const float HalfFanAngle = FMath::Acos(FMath::Clamp(SweepAttackDotThreshold, -0.99f, 0.99f));
	const float EdgeWidth = FMath::Clamp(WarningEdgeWidth, 2.0f, Range * 0.15f);
	const float InnerRadius = FMath::Clamp(SweepAttackInnerRadius, 0.0f, Range);
	auto MakeVertex = [this](UProceduralMeshComponent* Mesh, float R, float Angle, float Offset)
	{
		return ProjectWarningPointToGround(Mesh, R * FMath::Cos(Angle), R * FMath::Sin(Angle), Offset);
	};

	// The sweep is a curved rectangular band (an annular sector), not a pie
	// wedge. Its polar grid has subdivisions in both directions and every grid
	// vertex is projected independently onto the floor.
	BuildPolarGrid(SweepWarningMesh, InnerRadius, Range,
		-HalfFanAngle, HalfFanAngle, false, SweepWarningFillMaterial);

	TArray<FVector> EdgeVertices;
	TArray<int32> EdgeTriangles;
	AddAnnularStrip(EdgeVertices, EdgeTriangles,
		[&](float R, float A) { return MakeVertex(SweepWarningMesh, R, A, WarningGroundOffset + 2.0f); },
		Range - EdgeWidth, Range, -HalfFanAngle, HalfFanAngle, FMath::Clamp(WarningArcSegments, 24, 128));
	AddRibbon(EdgeVertices, EdgeTriangles,
		[&](float R, float A) { return MakeVertex(SweepWarningMesh, R, A, WarningGroundOffset + 2.0f); },
		InnerRadius, Range, -HalfFanAngle, EdgeWidth * 0.5f / Range);
	AddRibbon(EdgeVertices, EdgeTriangles,
		[&](float R, float A) { return MakeVertex(SweepWarningMesh, R, A, WarningGroundOffset + 2.0f); },
		InnerRadius, Range, HalfFanAngle, EdgeWidth * 0.5f / Range);
	AddAnnularStrip(EdgeVertices, EdgeTriangles,
		[&](float R, float A) { return MakeVertex(SweepWarningMesh, R, A, WarningGroundOffset + 2.0f); },
		FMath::Max(0.0f, InnerRadius - EdgeWidth), InnerRadius,
		-HalfFanAngle, HalfFanAngle, FMath::Clamp(WarningArcSegments, 24, 128));
	const int32 DashCount = 8;
	const float DashStep = (HalfFanAngle * 2.0f) / DashCount;
	const float DashArc = DashStep * 0.55f;
	for (int32 Dash = 0; Dash < DashCount; ++Dash)
	{
		const float CenterAngle = -HalfFanAngle + (Dash + 0.5f) * DashStep;
		AddAnnularStrip(EdgeVertices, EdgeTriangles,
			[&](float R, float A) { return MakeVertex(SweepWarningMesh, R, A, WarningGroundOffset + 2.0f); },
			FMath::Lerp(InnerRadius, Range, 0.58f) - 2.5f, FMath::Lerp(InnerRadius, Range, 0.58f) + 2.5f,
			CenterAngle - DashArc * 0.5f, CenterAngle + DashArc * 0.5f, 2);
	}
	CreateWarningMeshSection(SweepWarningMesh, 1, EdgeVertices, EdgeTriangles, SweepWarningEdgeMaterial);
}

void AStolenOathTrainingSentinel::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (CurrentHealth <= 0.0f || bInterruptedForSpecial || GetWorldTimerManager().IsTimerActive(StrikeTimerHandle))
	{
		return;
	}

	const AStolenOathCharacter* Player = Cast<AStolenOathCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!IsValid(Player))
	{
		return;
	}

	const FVector ToPlayer = (Player->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	if (ToPlayer.IsNearlyZero())
	{
		return;
	}

	const float DistanceToPlayer = FVector::Dist2D(GetActorLocation(), Player->GetActorLocation());
	if (DistanceToPlayer > FollowStopDistance)
	{
		const float StepDistance = FMath::Min(FollowSpeed * DeltaSeconds, DistanceToPlayer - FollowStopDistance);
		const FVector CurrentLocation = GetActorLocation();
		FVector DesiredLocation = CurrentLocation + ToPlayer * StepDistance;

		// Sweep with a sphere around the Sentinel's middle rather than its tall,
		// floor-touching root box. The old swept SetActorLocation was getting
		// blocked by floor contact, so the actor could rotate but never translate.
		const FVector MeshExtent = IsValid(SentinelMesh) ? SentinelMesh->Bounds.BoxExtent : FVector(100.0f);
		const float TravelRadius = FMath::Max(40.0f, FMath::Min(MeshExtent.X, MeshExtent.Y) * 0.9f);
		FHitResult ObstacleHit;
		FCollisionQueryParams MoveParams(SCENE_QUERY_STAT(SentinelFollowSweep), false, this);
		bool bBlocked = GetWorld()->SweepSingleByChannel(
			ObstacleHit,
			CurrentLocation,
			DesiredLocation,
			FQuat::Identity,
			ECC_WorldStatic,
			FCollisionShape::MakeSphere(TravelRadius),
			MoveParams);

		if (bBlocked)
		{
			const FVector SlideDirection = FVector::VectorPlaneProject(ToPlayer, ObstacleHit.ImpactNormal).GetSafeNormal2D();
			if (!SlideDirection.IsNearlyZero())
			{
				const FVector SlideTarget = CurrentLocation + SlideDirection * StepDistance;
				FHitResult SlideHit;
				if (!GetWorld()->SweepSingleByChannel(
					SlideHit, CurrentLocation, SlideTarget, FQuat::Identity, ECC_WorldStatic,
					FCollisionShape::MakeSphere(TravelRadius), MoveParams))
				{
					DesiredLocation = SlideTarget;
					bBlocked = false;
				}
			}
		}

		if (!bBlocked)
		{
			FHitResult GroundHit;
			FCollisionQueryParams GroundParams(SCENE_QUERY_STAT(SentinelFollowGround), false, this);
			if (GetWorld()->LineTraceSingleByChannel(
				GroundHit,
				DesiredLocation + FVector::UpVector * 1000.0f,
				DesiredLocation - FVector::UpVector * 2000.0f,
				ECC_WorldStatic,
				GroundParams))
			{
				DesiredLocation.Z = GroundHit.ImpactPoint.Z + MeshExtent.Z;
			}
			SetActorLocation(DesiredLocation, false);
		}
	}

	const FRotator TargetRotation(0.0f, ToPlayer.Rotation().Yaw, 0.0f);
	SetActorRotation(FMath::RInterpConstantTo(GetActorRotation(), TargetRotation, DeltaSeconds, TurnSpeed));
}

void AStolenOathTrainingSentinel::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(AttackTimerHandle);
	GetWorldTimerManager().ClearTimer(StrikeTimerHandle);
	GetWorldTimerManager().ClearTimer(ResetTimerHandle);
	GetWorldTimerManager().ClearTimer(HitFlashTimerHandle);
	GetWorldTimerManager().ClearTimer(StunTimerHandle);
	Super::EndPlay(EndPlayReason);
}

void AStolenOathTrainingSentinel::TelegraphStrike()
{
	if (bInterruptedForSpecial || CurrentHealth <= 0.0f || GetWorldTimerManager().IsTimerActive(StrikeTimerHandle))
	{
		return;
	}

	AStolenOathCharacter* Player = Cast<AStolenOathCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!IsValid(Player) || FVector::DistSquared2D(Player->GetActorLocation(), GetActorLocation()) > FMath::Square(AttackRange))
	{
		GetWorldTimerManager().SetTimer(AttackTimerHandle, this, &AStolenOathTrainingSentinel::TelegraphStrike, GetCurrentAttackInterval(), false);
		return;
	}

	const uint32 AttackId = ++NextAttackId;
	bCurrentAttackIsSweep = bUseSweepNext;
	bUseSweepNext = !bUseSweepNext;
	AttackTargetLocation = Player->GetActorLocation();
	AttackForward = (AttackTargetLocation - GetActorLocation()).GetSafeNormal2D();
	if (!AttackForward.IsNearlyZero())
	{
		SetActorRotation(AttackForward.Rotation());
		AttackForward = GetActorForwardVector().GetSafeNormal2D();
	}
	if (IsValid(OverheadWarningMesh))
	{
		OverheadWarningMesh->SetVisibility(false, true);
		OverheadWarningMesh->SetHiddenInGame(true, true);
	}
	if (IsValid(SweepWarningMesh))
	{
		SweepWarningMesh->SetVisibility(false, true);
		SweepWarningMesh->SetHiddenInGame(true, true);
	}
	if (bCurrentAttackIsSweep)
	{
		if (IsValid(SweepWarningMesh))
		{
			SweepWarningMesh->SetWorldLocation(GetActorLocation());
			SweepWarningMesh->SetWorldRotation(AttackForward.Rotation());
			RebuildSweepWarning();
			SweepWarningMesh->SetVisibility(true, true);
			SweepWarningMesh->SetHiddenInGame(false, true);
		}
	}
	else if (IsValid(OverheadWarningMesh))
	{
		FVector MarkerLocation = AttackTargetLocation;
		FHitResult GroundHit;
		FCollisionQueryParams GroundTraceParams(SCENE_QUERY_STAT(SentinelWarningGroundTrace), false, this);
		if (GetWorld()->LineTraceSingleByChannel(
			GroundHit,
			MarkerLocation + FVector::UpVector * 500.0f,
			MarkerLocation - FVector::UpVector * 1500.0f,
			ECC_WorldStatic,
			GroundTraceParams))
		{
			MarkerLocation = GroundHit.ImpactPoint;
		}
		OverheadWarningMesh->SetWorldLocation(MarkerLocation);
		OverheadWarningMesh->SetWorldRotation(FRotator::ZeroRotator);
		RebuildOverheadWarning();
		OverheadWarningMesh->SetVisibility(true, true);
		OverheadWarningMesh->SetHiddenInGame(false, true);
	}

	const float CurrentTelegraphDuration = bPhaseTwoActive ? PhaseTwoTelegraphDuration : TelegraphDuration;
	const TCHAR* AttackName = bCurrentAttackIsSweep ? TEXT("LATERAL SWEEP") : TEXT("OVERHEAD STRIKE");
	Player->ShowCombatMessage(FString::Printf(TEXT("SENTINEL %s  |  DODGE NOW (LEFT SHIFT)"), AttackName), FColor(255, 100, 72), CurrentTelegraphDuration + 0.2f);
	GetWorldTimerManager().SetTimer(StrikeTimerHandle, FTimerDelegate::CreateUObject(this, &AStolenOathTrainingSentinel::ResolveStrike, AttackId), CurrentTelegraphDuration, false);
}

void AStolenOathTrainingSentinel::ResolveStrike(uint32 AttackId)
{
	SetWarningMeshesVisible(false);

	AStolenOathCharacter* Player = Cast<AStolenOathCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!IsValid(Player) || bInterruptedForSpecial)
	{
		return;
	}

	const FVector PlayerLocation = Player->GetActorLocation();
	bool bHitAttackArea = false;
	if (bCurrentAttackIsSweep)
	{
		const FVector PlayerDirection = (PlayerLocation - GetActorLocation()).GetSafeNormal2D();
		const float PlayerDistance = FVector::Dist2D(PlayerLocation, GetActorLocation());
		const float EffectiveRange = FMath::Max(1.0f, SweepAttackRange);
		const float EffectiveInnerRadius = FMath::Clamp(SweepAttackInnerRadius, 0.0f, EffectiveRange);
		bHitAttackArea = PlayerDistance >= EffectiveInnerRadius
			&& PlayerDistance <= EffectiveRange
			&& FVector::DotProduct(AttackForward, PlayerDirection) >= SweepAttackDotThreshold;
	}
	else
	{
		bHitAttackArea = FVector::DistSquared2D(PlayerLocation, AttackTargetLocation) <= FMath::Square(OverheadAttackRadius);
	}

	if (bHitAttackArea)
	{
		Player->ReceiveSentinelAttack(AttackId, AttackDamage * (bPhaseTwoActive ? PhaseTwoDamageMultiplier : 1.0f));
	}
	else
	{
		Player->ShowCombatMessage(TEXT("SENTINEL MISSED"), FColor(145, 215, 240), 0.8f);
	}

	if (CurrentHealth > 0.0f && !bInterruptedForSpecial)
	{
		const float NextTelegraphDuration = bPhaseTwoActive ? PhaseTwoTelegraphDuration : TelegraphDuration;
		const float RecoveryDuration = FMath::Max(0.1f, GetCurrentAttackInterval() - NextTelegraphDuration);
		GetWorldTimerManager().SetTimer(AttackTimerHandle, this, &AStolenOathTrainingSentinel::TelegraphStrike, RecoveryDuration, false);
	}
}

bool AStolenOathTrainingSentinel::ReceivePlayerAttack(uint32 AttackId, float Damage, const FVector& HitLocation)
{
	if (AttackId == 0 || AttackId == LastPlayerAttackId || CurrentHealth <= 0.0f)
	{
		return false;
	}
	LastPlayerAttackId = AttackId;
	CurrentHealth = FMath::Max(0.0f, CurrentHealth - Damage);
	if (CurrentHealth > 0.0f)
	{
		UpdateCombatPhase();
	}
	UpdateDamageAppearance();
	if (SentinelDynamicMaterial)
	{
		SentinelDynamicMaterial->SetVectorParameterValue(TEXT("Color"), HitFlashColor);
		GetWorldTimerManager().SetTimer(HitFlashTimerHandle, this, &AStolenOathTrainingSentinel::ClearHitFlash, HitFlashDuration, false);
	}
	if (CurrentHealth <= 0.0f)
	{
		HandleDeath();
	}
	return true;
}

void AStolenOathTrainingSentinel::ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse)
{
	if (Damage > 0.0f && CurrentHealth > 0.0f)
	{
		CurrentHealth = FMath::Max(0.0f, CurrentHealth - Damage);
		if (CurrentHealth > 0.0f)
		{
			UpdateCombatPhase();
		}
		UpdateDamageAppearance();
		if (CurrentHealth <= 0.0f)
		{
			HandleDeath();
		}
	}
}

void AStolenOathTrainingSentinel::HandleDeath()
{
	if (GetWorldTimerManager().IsTimerActive(ResetTimerHandle))
	{
		return;
	}
	SetWarningMeshesVisible(false);

	GetWorldTimerManager().ClearTimer(AttackTimerHandle);
	GetWorldTimerManager().ClearTimer(StrikeTimerHandle);
	GetWorldTimerManager().ClearTimer(HitFlashTimerHandle);
	GetWorldTimerManager().ClearTimer(StunTimerHandle);
	bInterruptedForSpecial = false;
	SentinelMesh->SetVisibility(false, true);
	SentinelMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetWorldTimerManager().SetTimer(ResetTimerHandle, this, &AStolenOathTrainingSentinel::ResetAfterDefeat, DefeatRespawnDelay, false);
}

void AStolenOathTrainingSentinel::ApplyHealing(float Healing, AActor* Healer)
{
	if (Healing > 0.0f && CurrentHealth > 0.0f)
	{
		CurrentHealth = FMath::Min(MaxHealth, CurrentHealth + Healing);
		UpdateDamageAppearance();
	}
}

void AStolenOathTrainingSentinel::NotifyDanger(const FVector& DangerLocation, AActor* DangerSource)
{
	// The prototype has no AI reaction yet; its scheduled strike telegraph remains authoritative.
}

void AStolenOathTrainingSentinel::ResetAfterDefeat()
{
	GetWorldTimerManager().ClearTimer(AttackTimerHandle);
	GetWorldTimerManager().ClearTimer(StrikeTimerHandle);
	GetWorldTimerManager().ClearTimer(ResetTimerHandle);
	GetWorldTimerManager().ClearTimer(HitFlashTimerHandle);
	GetWorldTimerManager().ClearTimer(StunTimerHandle);
	SetWarningMeshesVisible(false);

	CurrentHealth = MaxHealth;
	bPhaseTwoActive = false;
	bUseSweepNext = false;
	bCurrentAttackIsSweep = false;
	bInterruptedForSpecial = false;
	LastPlayerAttackId = 0;
	SentinelMesh->SetRelativeScale3D(BaseMeshScale);
	SentinelMesh->SetVisibility(true, true);
	SentinelMesh->SetHiddenInGame(false, true);
	SentinelMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	UpdateDamageAppearance();
	GetWorldTimerManager().SetTimer(AttackTimerHandle, this, &AStolenOathTrainingSentinel::TelegraphStrike, InitialAttackDelay, false);
}

void AStolenOathTrainingSentinel::UpdateDamageAppearance()
{
	const float HealthFraction = MaxHealth > 0.0f ? FMath::Clamp(CurrentHealth / MaxHealth, 0.2f, 1.0f) : 1.0f;
	SentinelMesh->SetRelativeScale3D(FVector(BaseMeshScale.X, BaseMeshScale.Y, BaseMeshScale.Z * HealthFraction));
	if (SentinelDynamicMaterial)
	{
		SentinelDynamicMaterial->SetVectorParameterValue(TEXT("Color"), bPhaseTwoActive ? PhaseTwoColor : PhaseOneColor);
	}
}

void AStolenOathTrainingSentinel::ClearHitFlash()
{
	UpdateDamageAppearance();
}

void AStolenOathTrainingSentinel::UpdateCombatPhase()
{
	if (!bPhaseTwoActive && MaxHealth > 0.0f && CurrentHealth <= MaxHealth * PhaseTwoHealthFraction)
	{
		bPhaseTwoActive = true;
		if (!GetWorldTimerManager().IsTimerActive(StrikeTimerHandle))
		{
			GetWorldTimerManager().ClearTimer(AttackTimerHandle);
			GetWorldTimerManager().SetTimer(AttackTimerHandle, this, &AStolenOathTrainingSentinel::TelegraphStrike, PhaseTwoEntryDelay, false);
		}
	}
}

void AStolenOathTrainingSentinel::InterruptForDirectorCut(float StunDuration)
{
	if (CurrentHealth <= 0.0f || StunDuration <= 0.0f)
	{
		return;
	}

	bInterruptedForSpecial = true;
	SetWarningMeshesVisible(false);

	GetWorldTimerManager().ClearTimer(AttackTimerHandle);
	GetWorldTimerManager().ClearTimer(StrikeTimerHandle);
	GetWorldTimerManager().SetTimer(StunTimerHandle, this, &AStolenOathTrainingSentinel::EndSpecialInterrupt, StunDuration, false);
}

void AStolenOathTrainingSentinel::EndSpecialInterrupt()
{
	bInterruptedForSpecial = false;
	if (CurrentHealth > 0.0f)
	{
		const float CurrentInterval = GetCurrentAttackInterval();
		GetWorldTimerManager().SetTimer(AttackTimerHandle, this, &AStolenOathTrainingSentinel::TelegraphStrike, CurrentInterval, false);
	}
}

float AStolenOathTrainingSentinel::GetCurrentAttackInterval() const
{
	return bPhaseTwoActive ? PhaseTwoAttackInterval : AttackInterval;
}

// Copyright Epic Games, Inc. All Rights Reserved.

#include "StolenOathTrainingRelic.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AStolenOathTrainingRelic::AStolenOathTrainingRelic()
{
	PrimaryActorTick.bCanEverTick = false;

	RelicBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RelicBody"));
	SetRootComponent(RelicBody);
	RelicBody->SetRelativeScale3D(RelicBodyScale);
	RelicBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	RelicCore = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RelicCore"));
	RelicCore->SetupAttachment(RelicBody);
	RelicCore->SetRelativeRotation(RelicCoreRotation);
	RelicCore->SetRelativeScale3D(RelicCoreScale);
	RelicCore->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicShapeMaterial(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial_Inst.BasicShapeMaterial_Inst"));

	if (SphereMesh.Succeeded())
	{
		RelicBody->SetStaticMesh(SphereMesh.Object);
	}
	if (CubeMesh.Succeeded())
	{
		RelicCore->SetStaticMesh(CubeMesh.Object);
	}
	if (BasicShapeMaterial.Succeeded())
	{
		RelicBody->SetMaterial(0, BasicShapeMaterial.Object);
		RelicCore->SetMaterial(0, BasicShapeMaterial.Object);
	}
}

void AStolenOathTrainingRelic::BeginPlay()
{
	Super::BeginPlay();
	if (RelicBody->GetMaterial(0))
	{
		RelicMaterial = UMaterialInstanceDynamic::Create(RelicBody->GetMaterial(0), this);
		if (RelicMaterial)
		{
			RelicMaterial->SetVectorParameterValue(TEXT("Color"), RelicColor);
			RelicBody->SetMaterial(0, RelicMaterial);
			RelicCore->SetMaterial(0, RelicMaterial);
		}
	}
}

void AStolenOathTrainingRelic::Collect()
{
	Destroy();
}

// Copyright Epic Games, Inc. All Rights Reserved.

#include "StolenOathCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Variant_Combat/Interfaces/CombatDamageable.h"
#include "StolenOathTrainingSentinel.h"
#include "StolenOathTrainingRelic.h"
#include "StolenOathHUD.h"
#include "StolenOath.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AStolenOathCharacter::AStolenOathCharacter()
{
	TrainingRelicClass = AStolenOathTrainingRelic::StaticClass();
	TrainingSentinelClass = AStolenOathTrainingSentinel::StaticClass();

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	PrototypeSwordBlade = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrototypeSwordBlade"));
	PrototypeSwordBlade->SetupAttachment(GetMesh(), TEXT("HandGrip_R"));
	PrototypeSwordBlade->SetRelativeLocation(FVector(43.0f, 0.0f, 0.0f));
	PrototypeSwordBlade->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	PrototypeSwordBlade->SetRelativeScale3D(FVector(0.045f, 0.045f, 0.58f));
	PrototypeSwordBlade->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PrototypeSwordHilt = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrototypeSwordHilt"));
	PrototypeSwordHilt->SetupAttachment(GetMesh(), TEXT("HandGrip_R"));
	PrototypeSwordHilt->SetRelativeLocation(FVector(-8.0f, 0.0f, 0.0f));
	PrototypeSwordHilt->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	PrototypeSwordHilt->SetRelativeScale3D(FVector(0.18f, 0.055f, 0.055f));
	PrototypeSwordHilt->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PrototypeSwordGuard = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrototypeSwordGuard"));
	PrototypeSwordGuard->SetupAttachment(GetMesh(), TEXT("HandGrip_R"));
	PrototypeSwordGuard->SetRelativeLocation(FVector(9.0f, 0.0f, 0.0f));
	PrototypeSwordGuard->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	PrototypeSwordGuard->SetRelativeScale3D(FVector(0.035f, 0.18f, 0.035f));
	PrototypeSwordGuard->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PrototypeCube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (PrototypeCube.Succeeded())
	{
		PrototypeSwordBlade->SetStaticMesh(PrototypeCube.Object);
		PrototypeSwordHilt->SetStaticMesh(PrototypeCube.Object);
		PrototypeSwordGuard->SetStaticMesh(PrototypeCube.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PrototypeMaterial(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial_Inst.BasicShapeMaterial_Inst"));
	if (PrototypeMaterial.Succeeded())
	{
		PrototypeSwordBlade->SetMaterial(0, PrototypeMaterial.Object);
		PrototypeSwordHilt->SetMaterial(0, PrototypeMaterial.Object);
		PrototypeSwordGuard->SetMaterial(0, PrototypeMaterial.Object);
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequence> BasicAttackAsset(
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01.MM_Attack_01"));
	if (BasicAttackAsset.Succeeded())
	{
		BasicAttackAnimation = BasicAttackAsset.Object;
	}

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void AStolenOathCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (!PlayerInputComponent)
	{
		return;
	}

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AStolenOathCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AStolenOathCharacter::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AStolenOathCharacter::Look);
	}
	else
	{
		UE_LOG(LogStolenOath, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}

	// Prototype controls stay independent of the starter template's input assets.
	PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AStolenOathCharacter::PerformBasicAttack);
	PlayerInputComponent->BindKey(EKeys::LeftShift, IE_Pressed, this, &AStolenOathCharacter::StartDodge);
	PlayerInputComponent->BindKey(EKeys::E, IE_Pressed, this, &AStolenOathCharacter::TryCollectRelic);
	PlayerInputComponent->BindKey(EKeys::Q, IE_Pressed, this, &AStolenOathCharacter::PerformDirectorCut);
}

void AStolenOathCharacter::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;
	PrototypeSpawnLocation = GetActorLocation();
	bHasRelic = false;

	if (HasAuthority() && bSpawnTrainingSentinel && GetWorld())
	{
		const FVector SpawnForward = GetActorForwardVector().GetSafeNormal2D();
		FVector RelicLocation = GetActorLocation() + SpawnForward * TrainingRelicSpawnDistance;
		const FVector TraceStart = RelicLocation + FVector::UpVector * 1000.0f;
		const FVector TraceEnd = RelicLocation - FVector::UpVector * 2000.0f;
		FCollisionQueryParams GroundTraceParams(SCENE_QUERY_STAT(StolenOathRelicGroundTrace), false, this);
		FHitResult GroundHit;
		if (GetWorld()->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_WorldStatic, GroundTraceParams))
		{
			RelicLocation.Z = GroundHit.ImpactPoint.Z + 50.0f;
		}
		const FTransform RelicTransform(FRotator::ZeroRotator, RelicLocation);
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = this;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		TrainingRelic = GetWorld()->SpawnActor<AStolenOathTrainingRelic>(TrainingRelicClass, RelicTransform, SpawnParameters);
	}
	UMaterialInterface* WeaponBaseMaterial = PrototypeSwordBlade->GetMaterial(0);
	if (WeaponBaseMaterial)
	{
		PrototypeSwordBladeMaterial = UMaterialInstanceDynamic::Create(WeaponBaseMaterial, this);
		if (PrototypeSwordBladeMaterial)
		{
			PrototypeSwordBlade->SetMaterial(0, PrototypeSwordBladeMaterial);
			PrototypeSwordHilt->SetMaterial(0, PrototypeSwordBladeMaterial);
			PrototypeSwordGuard->SetMaterial(0, PrototypeSwordBladeMaterial);
		}
	}
	// Keep the sword on the right grip and attach a separate dagger assembly to
	// the left grip so both follow their own hand pose.
	UStaticMesh* DaggerPrimitive = PrototypeSwordBlade->GetStaticMesh();
	if (DaggerPrimitive)
	{
		auto CreateDaggerPart = [this, DaggerPrimitive](FName Name, FVector Location, FVector Scale)
		{
			UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this, Name);
			Part->SetupAttachment(GetMesh(), TEXT("HandGrip_L"));
			Part->SetStaticMesh(DaggerPrimitive);
			Part->SetRelativeLocation(Location);
			Part->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
			Part->SetRelativeScale3D(Scale);
			Part->SetMobility(EComponentMobility::Movable);
			Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Part->SetGenerateOverlapEvents(false);
			Part->ComponentTags.Add(FName(TEXT("PrototypeDagger")));
			AddInstanceComponent(Part);
			Part->RegisterComponent();
			return Part;
		};

		UStaticMeshComponent* DaggerBlade = CreateDaggerPart(TEXT("PrototypeDaggerBlade"), FVector(25.0f, 0.0f, 0.0f), FVector(0.035f, 0.035f, 0.34f));
		UStaticMeshComponent* DaggerHilt = CreateDaggerPart(TEXT("PrototypeDaggerHilt"), FVector(-5.0f, 0.0f, 0.0f), FVector(0.12f, 0.045f, 0.045f));
		UStaticMeshComponent* DaggerGuard = CreateDaggerPart(TEXT("PrototypeDaggerGuard"), FVector(6.0f, 0.0f, 0.0f), FVector(0.025f, 0.12f, 0.025f));
		if (WeaponBaseMaterial)
		{
			if (UMaterialInstanceDynamic* DaggerMaterial = UMaterialInstanceDynamic::Create(WeaponBaseMaterial, this))
			{
				DaggerMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.48f, 0.56f, 0.66f, 1.0f));
				DaggerBlade->SetMaterial(0, DaggerMaterial);
				DaggerHilt->SetMaterial(0, DaggerMaterial);
				DaggerGuard->SetMaterial(0, DaggerMaterial);
			}
		}
	}
	UpdateSwordAppearance();

	ShowCombatMessage(TEXT("MOVE CLOSE TO THE RELIC  |  LMB: ATTACK  |  SHIFT: DODGE  |  Q: DIRECTOR'S CUT"), FColor::White, 2.5f);
}

void AStolenOathCharacter::SpawnTrainingSentinel()
{
	if (!HasAuthority() || !GetWorld() || !bSpawnTrainingSentinel || TrainingSentinel.IsValid())
	{
		return;
	}

	const FVector SpawnForward = GetActorForwardVector().GetSafeNormal2D();
	FVector SentinelLocation = GetActorLocation() + SpawnForward * TrainingSentinelSpawnDistance;
	const FVector TraceStart = SentinelLocation + FVector::UpVector * 1000.0f;
	const FVector TraceEnd = SentinelLocation - FVector::UpVector * 2000.0f;
	FCollisionQueryParams GroundTraceParams(SCENE_QUERY_STAT(StolenOathSentinelGroundTrace), false, this);
	FHitResult GroundHit;
	if (GetWorld()->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_WorldStatic, GroundTraceParams))
	{
		SentinelLocation.Z = GroundHit.ImpactPoint.Z + TrainingSentinelGroundOffset;
	}

	const FTransform SentinelTransform(FRotator(0.0f, (-SpawnForward).Rotation().Yaw, 0.0f), SentinelLocation);
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	TrainingSentinel = GetWorld()->SpawnActor<AStolenOathTrainingSentinel>(TrainingSentinelClass, SentinelTransform, SpawnParameters);
}

void AStolenOathCharacter::TryCollectRelic()
{
	if (bHasRelic)
	{
		ShowCombatMessage(TEXT("RELIC ALREADY CLAIMED"), FColor(255, 210, 120));
		return;
	}

	AStolenOathTrainingRelic* Relic = TrainingRelic.Get();
	if (!IsValid(Relic))
	{
		ShowCombatMessage(TEXT("NO RELIC NEARBY"), FColor(200, 210, 220));
		return;
	}

	if (FVector::DistSquared2D(GetActorLocation(), Relic->GetActorLocation()) > FMath::Square(RelicInteractionRange))
	{
		ShowCombatMessage(TEXT("MOVE CLOSER TO CLAIM THE RELIC"), FColor(200, 210, 220));
		return;
	}

	Relic->Collect();
	TrainingRelic.Reset();
	bHasRelic = true;
	ShowCombatMessage(TEXT("RELIC CLAIMED  |  THE SENTINEL AWAKENS"), FColor(255, 205, 100), 2.0f);
	SpawnTrainingSentinel();
}

bool AStolenOathCharacter::IsRelicInteractionAvailable() const
{
	const AStolenOathTrainingRelic* Relic = TrainingRelic.Get();
	return !bHasRelic && IsValid(Relic)
		&& FVector::DistSquared2D(GetActorLocation(), Relic->GetActorLocation()) <= FMath::Square(RelicInteractionRange);
}

void AStolenOathCharacter::PlayBasicAttackAnimation(float PlayRate)
{
	if (!BasicAttackAnimation || !GetMesh())
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}

	if (!bAttackRootMotionSuppressed)
	{
		PreAttackRootMotionMode = AnimInstance->RootMotionMode;
		AttackAnimInstance = AnimInstance;
		bAttackRootMotionSuppressed = true;
	}
	AnimInstance->SetRootMotionMode(ERootMotionMode::NoRootMotionExtraction);

	const float SafePlayRate = FMath::Max(PlayRate, 0.1f);
	UAnimMontage* AttackMontage = AnimInstance->PlaySlotAnimationAsDynamicMontage(BasicAttackAnimation, TEXT("UpperBody"), 0.08f, 0.12f, SafePlayRate);
	if (AttackMontage)
	{
		float RootMotionRestoreDelay = AttackMontage->GetPlayLength() / SafePlayRate + 0.12f;
		const float ExistingRestoreDelay = GetWorld()->GetTimerManager().GetTimerRemaining(AttackRootMotionRestoreTimerHandle);
		if (ExistingRestoreDelay > 0.0f)
		{
			RootMotionRestoreDelay = FMath::Max(RootMotionRestoreDelay, ExistingRestoreDelay);
		}
		GetWorld()->GetTimerManager().SetTimer(AttackRootMotionRestoreTimerHandle, this, &AStolenOathCharacter::RestoreAttackRootMotionMode, RootMotionRestoreDelay, false);
	}
	else
	{
		RestoreAttackRootMotionMode();
	}
}

void AStolenOathCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(DodgeTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(AttackRootMotionRestoreTimerHandle);
	}
	if (bAttackRootMotionSuppressed)
	{
		RestoreAttackRootMotionMode();
	}
	Super::EndPlay(EndPlayReason);
}

void AStolenOathCharacter::PerformBasicAttack()
{
	if (!GetWorld())
	{
		return;
	}

	PlayBasicAttackAnimation(BasicAttackAnimationPlayRate);

	const uint32 AttackId = ++NextPlayerAttackId;
	const float ResonanceDamageMultiplier = 1.0f + MaxResonanceDamageBonus * (Resonance / 100.0f);
	const float AttackDamage = BasicAttackDamage * ResonanceDamageMultiplier;
	const FVector TraceStart = GetActorLocation() + FVector::UpVector * BasicAttackTraceHeight;
	const FVector TraceEnd = TraceStart + GetActorForwardVector() * BasicAttackRange;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(StolenOathPrototypeAttack), false, this);
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	TArray<FHitResult> Hits;
	const bool bHitSomething = GetWorld()->SweepMultiByObjectType(
		Hits,
		TraceStart,
		TraceEnd,
		FQuat::Identity,
		ObjectParams,
		FCollisionShape::MakeSphere(BasicAttackRadius),
		QueryParams);

	TSet<TWeakObjectPtr<AActor>> HitActors;
	if (bHitSomething)
	{
		for (const FHitResult& Hit : Hits)
		{
			AActor* HitActor = Hit.GetActor();
			if (!IsValid(HitActor) || HitActors.Contains(HitActor))
			{
				continue;
			}
			HitActors.Add(HitActor);

			if (AStolenOathTrainingSentinel* Sentinel = Cast<AStolenOathTrainingSentinel>(HitActor))
			{
				if (Sentinel->ReceivePlayerAttack(AttackId, AttackDamage, Hit.ImpactPoint))
				{
					const float PreviousResonance = Resonance;
					Resonance = FMath::Clamp(Resonance + AttackResonanceGain, 0.0f, 100.0f);
					UpdateSwordAppearance();
					const float ResonanceGained = Resonance - PreviousResonance;
					ShowCombatMessage(FString::Printf(TEXT("SENTINEL HIT  |  HP %.0f / %.0f  |  RESONANCE +%.0f (%0.f%%)"), Sentinel->GetHealth(), Sentinel->GetMaxHealth(), ResonanceGained, Resonance), FColor(255, 205, 100));
				}
				else
				{
					ShowCombatMessage(TEXT("ATTACK MISSED"), FColor(195, 205, 220));
				}
				return;
			}

			if (ICombatDamageable* Damageable = Cast<ICombatDamageable>(HitActor))
			{
				Damageable->ApplyDamage(AttackDamage, this, Hit.ImpactPoint, GetActorForwardVector() * BasicAttackImpulse);
				const float PreviousResonance = Resonance;
				Resonance = FMath::Clamp(Resonance + AttackResonanceGain, 0.0f, 100.0f);
				UpdateSwordAppearance();
				ShowCombatMessage(FString::Printf(TEXT("HIT CONFIRMED  |  RESONANCE +%.0f (%0.f%%)"), Resonance - PreviousResonance, Resonance), FColor(255, 205, 100));
				return;
			}
		}
	}

	ShowCombatMessage(TEXT("ATTACK MISSED"), FColor(195, 205, 220));
}

void AStolenOathCharacter::PerformDirectorCut()
{
	if (!bHasRelic)
	{
		ShowCombatMessage(TEXT("CLAIM THE RELIC FIRST"), FColor(255, 205, 100));
		return;
	}
	if (Resonance < 100.0f)
	{
		ShowCombatMessage(TEXT("DIRECTOR'S CUT NEEDS FULL RESONANCE"), FColor(100, 220, 255));
		return;
	}
	if (bIsDodging)
	{
		ShowCombatMessage(TEXT("WAIT UNTIL THE DODGE ENDS"), FColor(195, 205, 220));
		return;
	}

	AStolenOathTrainingSentinel* Sentinel = TrainingSentinel.Get();
	if (!IsValid(Sentinel) || Sentinel->GetHealth() <= 0.0f)
	{
		ShowCombatMessage(TEXT("NO ACTIVE TARGET"), FColor(195, 205, 220));
		return;
	}
	if (FVector::DistSquared2D(GetActorLocation(), Sentinel->GetActorLocation()) > FMath::Square(DirectorCutRange))
	{
		ShowCombatMessage(TEXT("MOVE CLOSER TO THE SENTINEL"), FColor(195, 205, 220));
		return;
	}

	const float SpecialDamage = DirectorCutDamage * (1.0f + MaxResonanceDamageBonus);
	const uint32 AttackId = ++NextPlayerAttackId;
	if (!Sentinel->ReceivePlayerAttack(AttackId, SpecialDamage, Sentinel->GetActorLocation()))
	{
		ShowCombatMessage(TEXT("DIRECTOR'S CUT FAILED"), FColor(255, 110, 90));
		return;
	}

	Sentinel->InterruptForDirectorCut(DirectorCutStunDuration);
	Resonance = 0.0f;
	UpdateSwordAppearance();
	PlayBasicAttackAnimation(DirectorCutAnimationPlayRate);
	ShowCombatMessage(FString::Printf(TEXT("DIRECTOR'S CUT  |  %.0f DAMAGE  |  RESONANCE SPENT"), SpecialDamage), FColor(100, 225, 255), 2.0f);
}

void AStolenOathCharacter::StartDodge()
{
	if (bIsDodging || !GetWorld())
	{
		return;
	}

	FVector DodgeDirection = GetLastMovementInputVector().GetSafeNormal2D();
	if (DodgeDirection.IsNearlyZero())
	{
		DodgeDirection = GetActorForwardVector().GetSafeNormal2D();
	}

	bIsDodging = true;
	DodgeStartTime = GetWorld()->GetTimeSeconds();
	UAnimMontage* DodgeMontage = nullptr;
	const bool bWasRootMotionSuppressed = bAttackRootMotionSuppressed;
	if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		if (!DodgeInPlaceAnimation)
		{
			UAnimSequence* SourceDodgeAnimation = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jump/MM_Dash.MM_Dash"));
			if (SourceDodgeAnimation)
			{
				// Keep the authored dash pose but prevent its root translation from moving the
				// rendered mesh independently of the capsule's controlled dodge movement.
				DodgeInPlaceAnimation = DuplicateObject<UAnimSequence>(SourceDodgeAnimation, this);
				if (DodgeInPlaceAnimation)
				{
					DodgeInPlaceAnimation->bForceRootLock = true;
					DodgeInPlaceAnimation->RootMotionRootLock = ERootMotionRootLock::Zero;
				}
			}
		}
		UAnimSequence* DodgeAnimation = DodgeInPlaceAnimation;
		if (DodgeAnimation)
		{
			if (!bAttackRootMotionSuppressed)
			{
				PreAttackRootMotionMode = AnimInstance->RootMotionMode;
				AttackAnimInstance = AnimInstance;
				bAttackRootMotionSuppressed = true;
			}
			AnimInstance->SetRootMotionMode(ERootMotionMode::NoRootMotionExtraction);
			DodgeMontage = AnimInstance->PlaySlotAnimationAsDynamicMontage(DodgeAnimation, TEXT("DefaultSlot"), 0.03f, 0.06f, DodgeAnimationPlayRate);
		}
	}
	if (DodgeMontage)
	{
		float RootMotionRestoreDelay = DodgeMontage->GetPlayLength() / DodgeAnimationPlayRate + 0.08f;
		const float ExistingRestoreDelay = GetWorld()->GetTimerManager().GetTimerRemaining(AttackRootMotionRestoreTimerHandle);
		if (ExistingRestoreDelay > 0.0f)
		{
			RootMotionRestoreDelay = FMath::Max(RootMotionRestoreDelay, ExistingRestoreDelay);
		}
		GetWorld()->GetTimerManager().SetTimer(AttackRootMotionRestoreTimerHandle, this, &AStolenOathCharacter::RestoreAttackRootMotionMode, RootMotionRestoreDelay, false);
	}
	else if (!bWasRootMotionSuppressed)
	{
		RestoreAttackRootMotionMode();
	}

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		const float VerticalVelocity = Movement->Velocity.Z;
		Movement->StopMovementImmediately();
		Movement->Velocity.Z = VerticalVelocity;
	}
	ConsumeMovementInputVector();

	const float SafeDodgeDuration = FMath::Max(DodgeDuration, KINDA_SMALL_NUMBER);
	const float DodgeDistance = FMath::Clamp(DodgeSpeed, 100.0f, 1000.0f);
	GetWorld()->GetTimerManager().SetTimer(
		DodgeTimerHandle,
		FTimerDelegate::CreateWeakLambda(this, [this, DodgeDirection, SafeDodgeDuration, DodgeDistance, LastProgress = 0.0f]() mutable
		{
			if (!GetWorld() || !bIsDodging)
			{
				EndDodge();
				return;
			}

			const float Elapsed = GetWorld()->GetTimeSeconds() - DodgeStartTime;
			const float Progress = FMath::Clamp(Elapsed / SafeDodgeDuration, 0.0f, 1.0f);
			const FVector DodgeStep = DodgeDirection * DodgeDistance * (Progress - LastProgress);
			if (UCharacterMovementComponent* Movement = GetCharacterMovement())
			{
				FHitResult MoveHit;
				Movement->SafeMoveUpdatedComponent(DodgeStep, GetActorQuat(), true, MoveHit);
				if (MoveHit.IsValidBlockingHit())
				{
					// Project the unconsumed movement onto the contact plane so the dodge
					// slides along walls or sloped surfaces without ending on first contact.
					const FVector RemainingStep = DodgeStep * (1.0f - MoveHit.Time);
					const FVector SlideStep = FVector::VectorPlaneProject(RemainingStep, MoveHit.Normal);
					if (!SlideStep.IsNearlyZero())
					{
						FHitResult SlideHit;
						Movement->SafeMoveUpdatedComponent(SlideStep, GetActorQuat(), true, SlideHit);
					}
				}
			}
			LastProgress = Progress;

			if (Progress >= 1.0f)
			{
				EndDodge();
			}
		}),
		0.01f,
		true);
	ShowCombatMessage(TEXT("DODGE"), FColor(120, 220, 255), 0.7f);
}

void AStolenOathCharacter::EndDodge()
{
	bIsDodging = false;
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(DodgeTimerHandle);
	}
}

void AStolenOathCharacter::RestoreAttackRootMotionMode()
{
	if (bAttackRootMotionSuppressed)
	{
		if (UAnimInstance* AnimInstance = AttackAnimInstance.Get())
		{
			AnimInstance->SetRootMotionMode(PreAttackRootMotionMode);
		}
		AttackAnimInstance.Reset();
		bAttackRootMotionSuppressed = false;
	}
}

void AStolenOathCharacter::ReceiveSentinelAttack(uint32 AttackId, float Damage)
{
	if (AttackId == 0 || AttackId == LastHandledSentinelAttackId)
	{
		return;
	}
	LastHandledSentinelAttackId = AttackId;

	if (bIsDodging)
	{
		const float DodgeElapsed = GetWorld() ? GetWorld()->GetTimeSeconds() - DodgeStartTime : DodgeDuration;
		float ResonanceReward = NormalDodgeResonance;
		FString DodgeQuality = TEXT("DODGE SUCCESS");
		if (DodgeElapsed <= PerfectDodgeWindow)
		{
			ResonanceReward = PerfectDodgeResonance;
			DodgeQuality = TEXT("PERFECT DODGE");
		}
		else if (DodgeElapsed <= PreciseDodgeWindow)
		{
			ResonanceReward = PreciseDodgeResonance;
			DodgeQuality = TEXT("PRECISE DODGE");
		}

		const float PreviousResonance = Resonance;
		Resonance = FMath::Clamp(Resonance + ResonanceReward, 0.0f, 100.0f);
		UpdateSwordAppearance();
		ShowCombatMessage(FString::Printf(TEXT("%s  |  +%.0f RESONANCE  |  %.0f%%"), *DodgeQuality, Resonance - PreviousResonance, Resonance), FColor(80, 235, 255));
		return;
	}

	CurrentHealth = FMath::Max(0.0f, CurrentHealth - Damage);
	if (CurrentHealth <= 0.0f)
	{
		CurrentHealth = MaxHealth;
		Resonance = 0.0f;
		UpdateSwordAppearance();
		SetActorLocation(PrototypeSpawnLocation);
		ShowCombatMessage(TEXT("KAEL DOWN  |  HEALTH RESTORED"), FColor(255, 100, 100));
		return;
	}

	ShowCombatMessage(FString::Printf(TEXT("YOU WERE HIT  |  HEALTH %.0f / %.0f"), CurrentHealth, MaxHealth), FColor(255, 80, 80));
}

float AStolenOathCharacter::GetHealthFraction() const
{
	return MaxHealth > 0.0f ? FMath::Clamp(CurrentHealth / MaxHealth, 0.0f, 1.0f) : 0.0f;
}

float AStolenOathCharacter::GetResonanceFraction() const
{
	return FMath::Clamp(Resonance / 100.0f, 0.0f, 1.0f);
}

void AStolenOathCharacter::UpdateSwordAppearance()
{
	if (PrototypeSwordBladeMaterial)
	{
		const float ChargeFraction = FMath::Clamp(Resonance / 100.0f, 0.0f, 1.0f);
		const FLinearColor UnchargedColor(0.34f, 0.38f, 0.44f, 1.0f);
		const FLinearColor ChargedColor(0.1f, 0.78f, 1.0f, 1.0f);
		PrototypeSwordBladeMaterial->SetVectorParameterValue(TEXT("Color"), FMath::Lerp(UnchargedColor, ChargedColor, ChargeFraction));
		PrototypeSwordBlade->SetRelativeScale3D(FVector(0.045f + 0.02f * ChargeFraction, 0.045f + 0.02f * ChargeFraction, 0.58f + 0.18f * ChargeFraction));
	}
}

void AStolenOathCharacter::ShowCombatMessage(const FString& Message, const FColor& Color, float Duration) const
{
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (AStolenOathHUD* StolenOathHUD = Cast<AStolenOathHUD>(PlayerController->GetHUD()))
		{
			StolenOathHUD->SetCombatCue(Message, FLinearColor(Color), Duration);
			return;
		}
	}
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(12701, Duration, Color, Message);
	}
}

void AStolenOathCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void AStolenOathCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AStolenOathCharacter::DoMove(float Right, float Forward)
{
	if (bIsDodging)
	{
		return;
	}

	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void AStolenOathCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AStolenOathCharacter::DoJumpStart()
{
	// signal the character to jump
	Jump();
}

void AStolenOathCharacter::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}

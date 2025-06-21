// Copyright Epic Games, Inc. All Rights Reserved.

#include "PantherJamGameCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

APantherJamGameCharacter::APantherJamGameCharacter()
{
	GetCharacterMovement()->bUseControllerDesiredRotation = false;


	// Enable ticking for this character
	PrimaryActorTick.bCanEverTick = true;

	// Disable character rotation for custom rotation control
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void APantherJamGameCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &APantherJamGameCharacter::HandleJump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &APantherJamGameCharacter::OnJumpReleased);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APantherJamGameCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &APantherJamGameCharacter::Look);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APantherJamGameCharacter::Look);

		// Sliding
		/*EnhancedInputComponent->BindAction(SlideAction, ETriggerEvent::Started, this, &APantherJamGameCharacter::StartSlide);*/
	}
}


void APantherJamGameCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);

	//Get Direction for double jump
	FVector2D Input = Value.Get<FVector2D>();
	LastInputVector2D = Input;

	if (Controller && (Input != FVector2D::ZeroVector))
	{
		const FRotator YawRotation(0, Controller->GetControlRotation().Yaw, 0);
		const FVector ForwardDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDir, Input.Y);
		AddMovementInput(RightDir, Input.X);
	}
}

void APantherJamGameCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void APantherJamGameCharacter::DoMove(float Right, float Forward)
{
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

void APantherJamGameCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}


//void APantherJamGameCharacter::EndSlide()
//{
//	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
//	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("Sliding ended!"));
//}


//void ApantherJamGameCharacter::StartSlide()
//{
//	if (GetCharacterMovement()->IsMovingOnGround() && !GetCharacterMovement()->IsFalling())
//	{
//		GetCharacterMovement()->Velocity *= 1.2f;
//		GetCharacterMovement()->SetMovementMode(MOVE_Swimming);
//		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("Sliding started!"));
//		// bIsSliding = true;
//	}
//	else
//	{
//		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("Cannot slide while in the air!"));
//	}
//}

void APantherJamGameCharacter::OnJumpReleased()
{
	bWallRunning = false;
	bHoldingJump = false;
	WallRunTime = 0.f;

	// This function is called when the jump input is released
	//GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Blue, TEXT("space released"));
	if (GetCharacterMovement()->IsFalling() && bCanWallJump)
	{
        // Cast ray on both sides to check for walls
        FVector Start = GetActorLocation();
        FVector RightVector = GetActorRightVector();
        FVector LeftEnd = Start - RightVector * 100.f;
        FVector RightEnd = Start + RightVector * 100.f;

        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(this);

        FHitResult LeftHit, RightHit;
        bool bLeftWall = GetWorld()->LineTraceSingleByChannel(LeftHit, Start, LeftEnd, ECC_Visibility, QueryParams);
        bool bRightWall = GetWorld()->LineTraceSingleByChannel(RightHit, Start, RightEnd, ECC_Visibility, QueryParams);

        FVector WallNormal = FVector::ZeroVector;
        if (bLeftWall)
        {
			WallNormal = LeftHit.ImpactNormal;
        }
        else if (bRightWall)
        {
			WallNormal = RightHit.ImpactNormal;
        }
		//GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Blue, TEXT("try Wall Jump"));
        if (!WallNormal.IsNearlyZero())
        {
			UCharacterMovementComponent* MoveComp = GetCharacterMovement();
			FVector Velocity = MoveComp->Velocity;

			// Reflect velocity against wall normal
			FVector ReflectedVelocity = FVector::VectorPlaneProject(Velocity, WallNormal) - Velocity.ProjectOnTo(WallNormal);
			ReflectedVelocity += WallNormal * 600.f; // Add some force away from wall

			ReflectedVelocity.Z = 800.f; // Give upward boost for wall jump

			LaunchCharacter(ReflectedVelocity, true, true);

			//GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Blue, TEXT("Wall Jump!"));
        }

	}
}

void APantherJamGameCharacter::HandleJump()
{
	bHoldingJump = true;

	if (GetCharacterMovement()->IsFalling() && bCanDoubleJump)
	{

		UCharacterMovementComponent* MoveComp = GetCharacterMovement();
		
		// Input direction
		if (LastInputVector2D.IsNearlyZero())
			return;

		// Desired direction based on input and control rotation
		FVector Input = FVector(LastInputVector2D.Y, LastInputVector2D.X, 0.f);
		FRotator YawRot(0.f, Controller->GetControlRotation().Yaw, 0.f);
		FVector DesiredDir = FRotationMatrix(YawRot).TransformVector(Input).GetSafeNormal();

		bCanDoubleJump = false;

		// Current horizontal velocity
		FVector CurrentVel = MoveComp->Velocity;
		FVector CurrentVel2D = FVector(CurrentVel.X, CurrentVel.Y, 0.f);
		float CurrentSpeed = CurrentVel2D.Size();

		// Angle between current velocity and desired direction
		float Dot = FVector::DotProduct(CurrentVel2D.GetSafeNormal(), DesiredDir);
		float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.f, 1.f)));

		if (AngleDeg > 135.f)
		{
			//GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("Double jump angle too sharp"));
			return;
		}

		// Apply speed loss based on angle (0degree = 15% loss, 90degree = 50% loss)
		float SpeedLoss = 0.f;
		if (AngleDeg <= 45.f)
		{
			SpeedLoss = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 45.f), FVector2D(0.f, 0.15f), AngleDeg);
		}
		else
		{
			SpeedLoss = FMath::GetMappedRangeValueClamped(FVector2D(45.f, 135.f), FVector2D(0.15f, 0.8f), AngleDeg);
		}
		float NewSpeed = CurrentSpeed * (1.f - SpeedLoss);

		FVector FinalVelocity = DesiredDir * NewSpeed;
		FinalVelocity.Z = 1000;

		LaunchCharacter(FinalVelocity, true, true);

		//GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
			//FString::Printf(TEXT("Double jump %.0fdegree loss: %.0f%%"), AngleDeg, SpeedLoss * 100.f));
		
	}
	else if (GetCharacterMovement()->IsMovingOnGround())
	{
		Jump();
		//GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, TEXT("Ground Jump"));
	}
}


void APantherJamGameCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	bCanDoubleJump = true;

	//GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("Landed"));
}


void APantherJamGameCharacter::Tick(float DeltaSeconds)  
{  
    Super::Tick(DeltaSeconds);


	// Get current XY Speed, ignoring Z
	float CurrentSpeed = GetVelocity().Size2D();

	if (bWallRunning)
	{
		// immediately cancel if jump is no longer held
		if (!bHoldingJump)
		{
			bWallRunning = false;
			WallRunTime = 0.f;
			return;
		}

		// Wall detection code
		FVector Start = GetActorLocation();
		FVector RightVector = GetActorRightVector();
		FVector LeftEnd = Start - RightVector * 100.f;
		FVector RightEnd = Start + RightVector * 100.f;

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);

		FHitResult LeftHit, RightHit;
		bool bLeftWall = GetWorld()->LineTraceSingleByChannel(LeftHit, Start, LeftEnd, ECC_Visibility, QueryParams);
		bool bRightWall = GetWorld()->LineTraceSingleByChannel(RightHit, Start, RightEnd, ECC_Visibility, QueryParams);

		if (bLeftWall || bRightWall)
		{
			WallRunTime += DeltaSeconds;

			// Maintain or reduce vertical velocity
			FVector Velocity = GetCharacterMovement()->Velocity;

			const float predropLength = 1.f; // Time before Z falloff
			const float dropLength = 1.f; // Duration of Z falloff (0% Gravity -> 100% Gravity)
			const float dropExponent = .5f; // Drop falloff easing exponent

			float dropTime = FMath::Clamp((WallRunTime - predropLength) / dropLength, 0.f, 1.f);
			float drop = FMath::InterpEaseIn(0.f, 1.f, dropTime, dropExponent);

			Velocity.Z = FMath::Max(Velocity.Z,  Velocity.Z * drop);
			GetCharacterMovement()->Velocity = Velocity;

			// Align velocity along wall
			FVector WallNormal = bLeftWall ? LeftHit.ImpactNormal : RightHit.ImpactNormal;
			FVector Forward = FVector::CrossProduct(WallNormal, FVector::UpVector);
			if (FVector::DotProduct(Forward, GetActorForwardVector()) < 0)
			{
				Forward *= -1.f;
			}
			FVector NewVel = Forward * CurrentSpeed;
			NewVel.Z = GetCharacterMovement()->Velocity.Z;
			GetCharacterMovement()->Velocity = NewVel;

			// Maintain 50 units from wall
			FVector ImpactPoint = bLeftWall ? LeftHit.ImpactPoint : RightHit.ImpactPoint;
			FVector DesiredOffset = ImpactPoint + (WallNormal * 50.f);
			FVector CurrentLocation = GetActorLocation();
			FVector NewLocation = FMath::VInterpTo(CurrentLocation, DesiredOffset, DeltaSeconds, 20.f); // smooth slide
			SetActorLocation(NewLocation, true);
		}
		else
		{
			bWallRunning = false;
			WallRunTime = 0.f;
		}
	}
	else if (bHoldingJump)
	{
		// Check for wall on left or right to start wall run
		FVector Start = GetActorLocation();
		FVector RightVector = GetActorRightVector();
		FVector LeftEnd = Start - RightVector * 100.f;
		FVector RightEnd = Start + RightVector * 100.f;

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);

		FHitResult LeftHit, RightHit;
		bool bLeftWall = GetWorld()->LineTraceSingleByChannel(LeftHit, Start, LeftEnd, ECC_Visibility, QueryParams);
		bool bRightWall = GetWorld()->LineTraceSingleByChannel(RightHit, Start, RightEnd, ECC_Visibility, QueryParams);

		if (bLeftWall || bRightWall)
		{
			bWallRunning = true;
			WallRunTime = 0.f;

			// Align velocity along wall
			FVector WallNormal = bLeftWall ? LeftHit.ImpactNormal : RightHit.ImpactNormal;
			FVector Forward = FVector::CrossProduct(WallNormal, FVector::UpVector);
			if (FVector::DotProduct(Forward, GetActorForwardVector()) < 0)
			{
				Forward *= -1.f;
			}
			FVector NewVel = Forward * CurrentSpeed;

			NewVel.Z = FMath::Max(GetCharacterMovement()->Velocity.Z, 0.f);
			GetCharacterMovement()->Velocity = NewVel;
		}
	}



	float NewAcceleration = 10000.f;
	
	// GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, FString::Printf(TEXT("Speed: %.2f"), CurrentSpeed));

	// Increase movement speed up to a cap
	if (CurrentSpeed >= 300.f)
	{
		NewAcceleration = FMath::GetMappedRangeValueClamped(
			FVector2D(300.f, 500.f),
			FVector2D(1500.f, 250.f),
			CurrentSpeed
		);
	}

	GetCharacterMovement()->MaxAcceleration = NewAcceleration;


	// Custom rotation logic based on direction speed
	if (!GetVelocity().IsNearlyZero())
	{
		FRotator NewRotation = GetVelocity().Rotation();
		NewRotation.Pitch = 0.0f; // Keep pitch zero to avoid tilting
		NewRotation.Roll = 0.0f; // Keep roll zero to avoid tilting
		SetActorRotation(NewRotation);
	}

}

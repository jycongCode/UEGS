#include "UEGS_Component.h"
#include "GSAsset.h"
#include "UEGS_RenderPass.h"

FGSAsset* U_UEGS_Component::GetAsset()
{
	IPlatformFile&PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (PlatformFile.FileExists(*GSFilePath))
	{
		FGSAsset* NewAsset = new FGSAsset();
		if (!NewAsset->LoadFromFile(*GSFilePath))
		{
		}else
		{
			Asset = NewAsset;
			Name = FPaths::GetBaseFilename(GSFilePath);
		}
	}else return nullptr;
	return Asset;
}

void U_UEGS_Component::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Warning, TEXT("U_UEGS_Component::BeginPlay"));
}

TSubclassOf<U_EGP_RenderPass> U_UEGS_Component::GetPassType() const
{
	return U_UEGS_RenderPass::StaticClass();
}

void U_UEGS_Component::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	// 2. Get the name of the property that was changed
	FName PropertyName = (PropertyChangedEvent.Property != nullptr) 
						 ? PropertyChangedEvent.Property->GetFName() 
						 : NAME_None;

	// 3. Check if it's the specific property you care about
	// GET_MEMBER_NAME_CHECKED is safe because it will throw a compile error if you rename the variable later
	auto* world = GetWorld();
	auto* subsystem = (IsValid(world)) ? world->GetSubsystem<U_EGP_RenderPassSubsystem>() : nullptr;
	auto* pass = IsValid(subsystem) ? subsystem->GetPass(GetPassType(), true) : nullptr;
	if (IsValid(pass))
	{
		if (PropertyName == GET_MEMBER_NAME_CHECKED(U_UEGS_Component, GSFilePath))
		{
			UE_LOG(LogTemp, Log, TEXT("MyCustomFloat was changed! Updating component..."));
			pass->RegisterPassComponent(this);
		}else
		{
			U_UEGS_RenderPass* targetPass = Cast<U_UEGS_RenderPass>(pass);
			check(targetPass != nullptr);
			if (auto* simResource = targetPass->SplatAssets.Find(Name))
			{
				simResource->OpacityScale = this->OpacityScale;
				simResource->SplatScale = this->SplatScale;
				simResource->SH = this->SH;;
				simResource->WorldTransform = FMatrix44f(this->GetComponentTransform().ToMatrixNoScale());
				simResource->Scale = FVector3f(this->GetComponentScale());
				
			}
		}
	}else
	{
		UE_LOG(LogEGP, Error,
			   TEXT("%s component created but there's no world/subsystem for custom render passes! No custom rendering can happen"),
			   *GetName());
	}
		
	
}

void U_UEGS_Component::PreEditChange(FProperty* PropertyAboutToChange)
{
	FName PropertyName = (PropertyAboutToChange != nullptr) 
						 ? PropertyAboutToChange->GetFName() 
						 : NAME_None;
	
	if (PropertyName == GET_MEMBER_NAME_CHECKED(U_UEGS_Component, GSFilePath))
	{
		UE_LOG(LogTemp, Log, TEXT("MyCustomFloat is about changed! Updating component..."));
        
		auto* world = GetWorld();
		auto* subsystem = (IsValid(world)) ? world->GetSubsystem<U_EGP_RenderPassSubsystem>() : nullptr;
		auto* pass = IsValid(subsystem) ? subsystem->GetPass(GetPassType(), true) : nullptr;
		if (IsValid(pass))
			pass->UnregisterPassComponent(this);
		else
			UE_LOG(LogEGP, Error,
				   TEXT("%s component created but there's no world/subsystem for custom render passes! No custom rendering can happen"),
				   *GetName());
	}
	
	Super::PreEditChange(PropertyAboutToChange);
}

#include "UEGS_Component.h"
#include "GSAsset.h"
#include "UEGS_RenderPass.h"

FGSAsset* U_UEGS_Component::GetAsset()
{
	IPlatformFile&PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	FGSAsset* NewAsset = nullptr;
	if (PlatformFile.FileExists(*GSFilePath))
	{
		NewAsset = new FGSAsset();
		if (NewAsset->LoadFromFile(*GSFilePath))
		{
			Name = FPaths::GetBaseFilename(GSFilePath);
		}else
		{
			delete(NewAsset);
			NewAsset = nullptr;
		}
		
	}
	return NewAsset;
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


#if WITH_EDITOR
void U_UEGS_Component::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	// 2. Get the name of the property that was changed
	FName PropertyName = (PropertyChangedEvent.Property != nullptr) 
						 ? PropertyChangedEvent.Property->GetFName() 
						 : NAME_None;
	
	if (TargetPass)
	{
		if (IsValid(TargetPass))
		{
			if (PropertyName == GET_MEMBER_NAME_CHECKED(U_UEGS_Component, GSFilePath))
			{
				UE_LOG(LogTemp, Log, TEXT("MyCustomFloat was changed! Updating component..."));
				ENQUEUE_RENDER_COMMAND(RegisterComp)([pass = TargetPass,this](FRHICommandListImmediate&)
				{
					pass->RegisterPassComponent(this);
				});
			}else
			{
				ENQUEUE_RENDER_COMMAND(UpdateComp)([this,pass=TargetPass](FRHICommandListImmediate&)
				{
					U_UEGS_RenderPass* targetPass = Cast<U_UEGS_RenderPass>(pass);
					check(targetPass != nullptr);
					if (auto* simResource = targetPass->SplatAssets.Find(this->Name))
					{
						simResource->OpacityScale = this->OpacityScale;
						simResource->SplatScale = this->SplatScale;
						simResource->SH = this->SH;;
						simResource->WorldTransform = FMatrix44f(this->GetComponentTransform().ToMatrixWithScale());
						simResource->Scale = FVector3f(this->GetComponentTransform().GetScale3D());
					}
				});
			}
		}
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

void U_UEGS_Component::PostEditComponentMove(bool bFinished)
{
	if (TargetPass)
	{
		if (IsValid(TargetPass))
		{
			ENQUEUE_RENDER_COMMAND(UpdateComp)([this,pass=TargetPass](FRHICommandListImmediate&)
			{
				U_UEGS_RenderPass* targetPass = Cast<U_UEGS_RenderPass>(pass);
				check(targetPass != nullptr);
				if (auto* simResource = targetPass->SplatAssets.Find(this->Name))
				{
					simResource->WorldTransform = FMatrix44f(this->GetComponentTransform().ToMatrixWithScale());
					FVector3f scale = FVector3f( this->GetComponentTransform().GetScale3D());
					simResource->Scale = scale;
				}
			});
		}
	}
}
#endif




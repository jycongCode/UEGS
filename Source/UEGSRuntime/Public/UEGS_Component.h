#pragma once
#include "EGP_CustomRenderPasses.h"
#include "UEGS_Component.generated.h"

UCLASS()
class U_UEGS_Component : public U_EGP_RenderPassComponent
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere,Category="GaussianSplatting")
	FString Name;
	
	UPROPERTY(BlueprintReadWrite,EditAnywhere, Category="GaussianSplatting")
	int SH = 0;
	
	UPROPERTY(BlueprintReadWrite,EditAnywhere, Category="GaussianSplatting")
	int SplatScale = 1.0f;
	
	UPROPERTY(BlueprintReadWrite,EditAnywhere, Category="GaussianSplatting")
	int OpacityScale = 1.0f;
	
	UPROPERTY(BlueprintReadOnly,VisibleAnywhere, Category="GaussianSplatting")
	int NumGS = 0.0;
	
	UPROPERTY(BlueprintReadWrite,EditAnywhere, Category="GaussianSplatting")
	FString GSFilePath;
	
	class FGSAsset* GetAsset();
	
	virtual void BeginPlay() override;
	
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PreEditChange(FProperty* PropertyAboutToChange) override;
	virtual void PostEditComponentMove(bool bFinished) override;
#endif
	
	virtual TSubclassOf<U_EGP_RenderPass> GetPassType() const override;
	
	class U_UEGS_RenderPass* TargetPass = nullptr;
protected:
	EGP_PASS_COMPONENT_SIMPLE_PROXY_IMPL(int,0)
	
};
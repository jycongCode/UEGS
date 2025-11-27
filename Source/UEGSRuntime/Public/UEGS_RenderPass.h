#pragma once
#include "EGP_CustomRenderPasses.h"
#include "GSAsset.h"
#include "UEGS_RenderPass.generated.h"
struct FUEGSRenderData final: public F_EGP_ViewPersistentData
{
	
	static FRHITextureCreateDesc SimStateDesc(const FInt32Point& viewportSize);

	// todo : add buffers needed for gpu sorting
	TRefCountPtr<FRHIBuffer> PreprocessDataBuffer, VertexAttributeBuffer, DepthKeyBufferPing, IndexValueBufferPing, DepthKeyBufferPong,IndexValueBufferPong;

	int NumGS = 0;
	
	FUEGSRenderData(FRDGBuilder&GraphBuilder,const FViewInfo& ViewInfo,const FIntRect& viewportSubset, UGSAsset* GSAssetData);

	virtual void Resample(FRDGBuilder&, const FViewInfo&, const FInt32Point& oldResolution, const FInt32Point& newResolution, const FInt32Point& oldToNewPixelOffset) {}
};

UCLASS(BlueprintType)
class U_UEGS_RenderPass : public U_EGP_RenderPass
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void LoadGSData(const FString& DataFilePath)
	{
		GSAssetData = NewObject<UGSAsset>();
		GSAssetData->LoadFromFile(DataFilePath,"TestGS");
	};

	UFUNCTION(BlueprintCallable)
	void Test()
	{
		GSAssetData = NewObject<UGSAsset>();
		GSAssetData->Test();
		
	}
	
	UGSAsset* GSAssetData;
	T_EGP_PerViewData<FUEGSRenderData> PerViewData;
protected:
	virtual TSharedRef<F_EGP_RenderPassSceneViewExtension> InitThisPass_GameThread(UWorld& thisWorld) override;
	virtual void Tick_GameThread(UWorld& thisWorld, float deltaSeconds) override;
	virtual void Tick_RenderThread(const FSceneInterface& thisScene, float gameThreadDeltaSeconds) override;
};

struct F_UEGS_PassSVE : public T_EGP_RenderPassSceneViewExtension<U_UEGS_RenderPass>
{
	using T_EGP_RenderPassSceneViewExtension::T_EGP_RenderPassSceneViewExtension;
	
	virtual void PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& InView, const FPostProcessingInputs& Inputs) override;
};



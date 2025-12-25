#pragma once
#include "EGP_CustomRenderPasses.h"
#include "GSAsset.h"
#include "UEGS_RenderPass.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogUEGS, Log, All);
struct GSResource
{
	FGSAsset* Asset;
	int SH = 0;
	FMatrix44f WorldTransform = FMatrix44f::Identity;
	FVector3f Scale = FVector3f::One();
	float SplatScale = 1.0f;
	float OpacityScale = 1.0f;
};

struct FUEGSRenderData final: public F_EGP_ViewPersistentData
{
	static FRHITextureCreateDesc SimStateDesc(const FInt32Point& viewportSize);

	// todo : add buffers needed for gpu sorting
	TRefCountPtr<FRHIBuffer> PreprocessDataBuffer, VertexAttributeBuffer, DepthKeyBufferPing, IndexValueBufferPing, DepthKeyBufferPong,IndexValueBufferPong;
	TRefCountPtr<FRDGPooledBuffer> PreprocessBufferPooled, VertexAttributeBufferPooled, DepthKeyBufferPingPooled, IndexValueBufferPingPooled, DepthKeyBufferPongPooled,IndexValueBufferPongPooled;
	int NumGS = 0;
	int SH = 0;
	FMatrix44f WorldTransform = FMatrix44f::Identity;
	FVector3f Scale = FVector3f::One();
	float SplatScale = 1.0f;
	float OpacityScale = 1.0f;
	
	FUEGSRenderData(FRDGBuilder&GraphBuilder,
		const FViewInfo& ViewInfo,
		const FIntRect& viewportSubset, 
		GSResource resource);

	virtual void Resample(FRDGBuilder&, const FViewInfo&, const FInt32Point& oldResolution, const FInt32Point& newResolution, const FInt32Point& oldToNewPixelOffset) {}
};

UCLASS(BlueprintType)
class U_UEGS_RenderPass : public U_EGP_RenderPass
{
	GENERATED_BODY()

public:
	TMap<FString,T_EGP_PerViewData<FUEGSRenderData>> SplatData;
	TMap<FString,GSResource> SplatAssets;
	FGSAsset* TargetAsset = nullptr;
	
	virtual void RegisterPassComponent(U_EGP_RenderPassComponent*) override;
	virtual void UnregisterPassComponent(U_EGP_RenderPassComponent*) override;
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



#include "UEGS_RenderPass.h"
#include "GPUSort.h"
#include "RHI.h"
#include "ClearQuad.h"
#include "Materials/MaterialIR.h"

#include "Operations/EmbedSurfacePath.h"
#include "PostProcess/PostProcessInputs.h"

static TAutoConsoleVariable<int32> CVarBlendDebug(
	TEXT("r.UEGS.BlendDiable"), // 控制台变量名，建议用“模块.”前缀
	0, // 默认值
	TEXT("控制自定义调试模式的开关：\n")
	TEXT("  0: 开启 (默认)\n")
	TEXT("  1: 关闭GS blend\n"),
	ECVF_Cheat); 

struct FGSPreprocessCS : FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FGSPreprocessCS);
	SHADER_USE_PARAMETER_STRUCT(FGSPreprocessCS,FGlobalShader)

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters,)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<float4>,PreprocessBuffer)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<float4>,VertexAttributeBuffer)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<uint>,DepthKeyBuffer)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<uint>,IndexValueBuffer)
		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters,View)
		SHADER_PARAMETER(FMatrix44f, WorldMatrix)
		SHADER_PARAMETER(FMatrix44f, ViewMatrix)
		SHADER_PARAMETER(FMatrix44f,ViewProjectionMatrix)
		SHADER_PARAMETER(int, NumGS)
	END_SHADER_PARAMETER_STRUCT()
};
IMPLEMENT_GLOBAL_SHADER(FGSPreprocessCS,"/UEGS/GSPreprocess.usf","Main",SF_Compute);

BEGIN_SHADER_PARAMETER_STRUCT(FGSRenderParameters,)
	SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<float4>, VertexAttributeBuffer)
	SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<uint>,IndexValueBuffer)
	SHADER_PARAMETER(int, NumGS)
	SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters,View)
	RENDER_TARGET_BINDING_SLOTS()
END_SHADER_PARAMETER_STRUCT()

BEGIN_SHADER_PARAMETER_STRUCT(FGSSortParameters,)
	SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<uint>, RemoteKeySRV1)
	SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<uint>, RemoteKeySRV2)
	SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<uint>, RemoteKeyUAV1)
	SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<uint>, RemoteKeyUAV2)
	
	SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<uint>, RemoteValueSRV1)
	SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<uint>, RemoteValueSRV2)
	SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<uint>, RemoteValueUAV1)
	SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<uint>, RemoteValueUAV2)
END_SHADER_PARAMETER_STRUCT()

struct FGSRenderVS : FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FGSRenderVS);
	SHADER_USE_PARAMETER_STRUCT(FGSRenderVS,FGlobalShader)

	using FParameters = FGSRenderParameters;
};
IMPLEMENT_GLOBAL_SHADER(FGSRenderVS,"/UEGS/GSRender.usf","MainVS",SF_Vertex);

struct FGSRenderPS : FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FGSRenderPS);
	SHADER_USE_PARAMETER_STRUCT(FGSRenderPS,FGlobalShader)

	using FParameters = FGSRenderParameters;
};
IMPLEMENT_GLOBAL_SHADER(FGSRenderPS,"/UEGS/GSRender.usf","MainPS",SF_Pixel);


TSharedRef<F_EGP_RenderPassSceneViewExtension> U_UEGS_RenderPass::InitThisPass_GameThread(UWorld& thisWorld)
{
	ViewFilter->FilterByPlayerIdx(0);
	return FSceneViewExtensions::NewExtension<F_UEGS_PassSVE>(this);
}

void U_UEGS_RenderPass::Tick_GameThread(UWorld& thisWorld, float deltaSeconds)
{
	Super::Tick_GameThread(thisWorld, deltaSeconds);
	// PerViewData.Tick();
}

void U_UEGS_RenderPass::Tick_RenderThread(const FSceneInterface& thisScene, float gameThreadDeltaSeconds)
{
	Super::Tick_RenderThread(thisScene, gameThreadDeltaSeconds);
	PerViewData.Tick();
}

void F_UEGS_PassSVE::PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& InView, const FPostProcessingInputs& Inputs)
{
	check(InView.bIsViewInfo);
	const auto& view = reinterpret_cast<const FViewInfo&>(InView);
	
	if (!Pass->ViewFilter->ShouldRenderFor(view))return;
	
	auto& simData = Pass->PerViewData.DataForView(GraphBuilder, view, Pass->GSAssetData);

	// Preprocess Pass
	auto PreprocessParams = GraphBuilder.AllocParameters<FGSPreprocessCS::FParameters>();
	
	auto PreprocessBufferRDG = GraphBuilder.RegisterExternalBuffer(simData.PreprocessBufferPooled);
	PreprocessParams->PreprocessBuffer = GraphBuilder.CreateSRV(PreprocessBufferRDG,PF_A32B32G32R32F);

	
	auto VertexAttributeRDG = GraphBuilder.RegisterExternalBuffer(simData.VertexAttributeBufferPooled);
	PreprocessParams->VertexAttributeBuffer = GraphBuilder.CreateUAV(VertexAttributeRDG,PF_A32B32G32R32F);
	
	auto depthKeyBufferPingRDG = GraphBuilder.RegisterExternalBuffer(simData.DepthKeyBufferPingPooled);
	PreprocessParams->DepthKeyBuffer = GraphBuilder.CreateUAV(depthKeyBufferPingRDG,PF_R32_UINT);
	
	auto IndexValuePingRDG = GraphBuilder.RegisterExternalBuffer(simData.IndexValueBufferPingPooled);
	PreprocessParams->IndexValueBuffer = GraphBuilder.CreateUAV(IndexValuePingRDG,PF_R32_UINT);
	
	PreprocessParams->View = InView.ViewUniformBuffer;
	// GS model is placed at world origin by default
	PreprocessParams->WorldMatrix = FMatrix44f(FTransform::Identity.ToMatrixWithScale());
	PreprocessParams->ViewMatrix = FMatrix44f(view.ViewMatrices.GetViewMatrix());
	PreprocessParams->ViewProjectionMatrix = FMatrix44f(view.ViewMatrices.GetViewProjectionMatrix());
	PreprocessParams->NumGS = simData.NumGS;
	TShaderMapRef<FGSPreprocessCS> PreprocessShader(view.ShaderMap);

	FIntVector GroupCount((simData.NumGS + 255)%256,1,1);
	GraphBuilder.AddPass(RDG_EVENT_NAME("Preprocess GS"),
		PreprocessParams,
		ERDGPassFlags::Compute,
		[PreprocessShader,PreprocessParams,GroupCount](FRHIComputeCommandList& RHICmdList)
		{
			FComputeShaderUtils::Dispatch(RHICmdList,PreprocessShader,*PreprocessParams,GroupCount);
		});
	
	// TODO : Add GPU Sort Pass to update IndexValueBuffer
	
	auto depthKeyBufferPongRDG = GraphBuilder.RegisterExternalBuffer(simData.DepthKeyBufferPongPooled);
	
	auto IndexValuePongRDG = GraphBuilder.RegisterExternalBuffer(simData.IndexValueBufferPongPooled);
	
	auto GpuSortParameters = GraphBuilder.AllocParameters<FGSSortParameters>();
	GpuSortParameters->RemoteKeySRV1 = GraphBuilder.CreateSRV(depthKeyBufferPingRDG,PF_R32_UINT);
	GpuSortParameters->RemoteKeySRV2 = GraphBuilder.CreateSRV(depthKeyBufferPongRDG,PF_R32_UINT);
	GpuSortParameters->RemoteKeyUAV1 = GraphBuilder.CreateUAV(depthKeyBufferPingRDG,PF_R32_UINT);
	GpuSortParameters->RemoteKeyUAV2 = GraphBuilder.CreateUAV(depthKeyBufferPongRDG,PF_R32_UINT);
	
	GpuSortParameters->RemoteValueSRV1 = GraphBuilder.CreateSRV(IndexValuePingRDG,PF_R32_UINT);
	GpuSortParameters->RemoteValueSRV2 = GraphBuilder.CreateSRV(IndexValuePongRDG,PF_R32_UINT);
	GpuSortParameters->RemoteValueUAV1 = GraphBuilder.CreateUAV(IndexValuePingRDG,PF_R32_UINT);
	GpuSortParameters->RemoteValueUAV2 = GraphBuilder.CreateUAV(IndexValuePongRDG,PF_R32_UINT);
	
	GraphBuilder.AddPass(RDG_EVENT_NAME("Gpu Sort GS"),
		GpuSortParameters,
		ERDGPassFlags::Compute,
		[GpuSortParameters,CurrentFeatureLevel=InView.FeatureLevel,Count=simData.NumGS](FRHICommandList& RHICmdList)
		{
			FGPUSortBuffers SortBuffers;
			SortBuffers.RemoteKeySRVs[0] = GpuSortParameters->RemoteKeySRV1->GetRHI();
			SortBuffers.RemoteKeySRVs[1] = GpuSortParameters->RemoteKeySRV2->GetRHI();
			SortBuffers.RemoteKeyUAVs[0] = GpuSortParameters->RemoteKeyUAV1->GetRHI();
			SortBuffers.RemoteKeyUAVs[1] = GpuSortParameters->RemoteKeyUAV2->GetRHI();
			
			SortBuffers.RemoteValueSRVs[0] = GpuSortParameters->RemoteValueSRV1->GetRHI();
			SortBuffers.RemoteValueSRVs[1] = GpuSortParameters->RemoteValueSRV2->GetRHI();
			SortBuffers.RemoteValueUAVs[0] = GpuSortParameters->RemoteValueUAV1->GetRHI();
			SortBuffers.RemoteValueUAVs[1] = GpuSortParameters->RemoteValueUAV2->GetRHI();
	
			SortBuffers.FirstValuesSRV = GpuSortParameters->RemoteValueSRV1->GetRHI();
			SortBuffers.FinalValuesUAV = GpuSortParameters->RemoteValueUAV1->GetRHI();
			SortGPUBuffers(RHICmdList,SortBuffers,0,0xFFFFFFFF,Count,CurrentFeatureLevel);
		});
	
	// Instance Rendering GS
	// AddClearRenderTargetPass(
	// 	GraphBuilder, 
	// 	Inputs.SceneTextures->GetContents()->SceneColorTexture, 
	// 	FLinearColor(0.0f, 0.0f, 0.0f, 0.0f) // 清除颜色
	// );
	auto RenderParameters = GraphBuilder.AllocParameters<FGSRenderParameters>();
	RenderParameters->VertexAttributeBuffer = GraphBuilder.CreateSRV(VertexAttributeRDG,PF_A32B32G32R32F);
	RenderParameters->IndexValueBuffer = GraphBuilder.CreateSRV(IndexValuePingRDG,PF_R32_UINT);
	RenderParameters->View = InView.ViewUniformBuffer;
	RenderParameters->NumGS = simData.NumGS;
	RenderParameters->RenderTargets[0] = {
		Inputs.SceneTextures->GetContents()->SceneColorTexture,
		ERenderTargetLoadAction::EClear
	};
	
	RenderParameters->RenderTargets.DepthStencil = FDepthStencilBinding(
		Inputs.SceneTextures->GetContents()->SceneDepthTexture,
		ERenderTargetLoadAction::ELoad,
		ERenderTargetLoadAction::ELoad,
		FExclusiveDepthStencil::DepthRead_StencilNop
	);
	
	TShaderMapRef<FGSRenderVS> VertexShader(view.ShaderMap);
	TShaderMapRef<FGSRenderPS> PixelShader(view.ShaderMap);

	GraphBuilder.AddPass(RDG_EVENT_NAME("GS Instance Draw"),
		RenderParameters,
		ERDGPassFlags::Raster,
		[VertexShader,PixelShader,RenderParameters,NumGS=simData.NumGS](FRHICommandList& RHICmdList)
		{
			
			// Set new states
			FRHIBlendState* BlendStateRHI = TStaticBlendState<
				CW_RGBA,
				BO_Add,BF_DestAlpha,BF_One,
				BO_Add,BF_Zero,BF_InverseSourceAlpha>::GetRHI();
			
			if (CVarBlendDebug.GetValueOnRenderThread() == 1)
			{
				BlendStateRHI = TStaticBlendState<CW_RGBA,
				BO_Add,BF_One,BF_Zero,BO_Add,BF_One,BF_Zero>::GetRHI();
			}
			
			
			FRHIDepthStencilState* DepthStencilStateRHI = TStaticDepthStencilState<false, CF_Always>::GetRHI();

			FGraphicsPipelineStateInitializer GraphicsPSOInit;
			RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

			GraphicsPSOInit.RasterizerState = TStaticRasterizerState<FM_Solid, CM_None>::GetRHI();
			GraphicsPSOInit.BlendState = BlendStateRHI;
			GraphicsPSOInit.DepthStencilState = DepthStencilStateRHI;

			GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GetVertexDeclarationFVector4();
			GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
			GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
			GraphicsPSOInit.PrimitiveType = PT_TriangleStrip;

			SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, 0);
			
			SetShaderParameters(RHICmdList,VertexShader,VertexShader.GetVertexShader(),*RenderParameters);
			SetShaderParameters(RHICmdList,PixelShader,PixelShader.GetPixelShader(),*RenderParameters);

			RHICmdList.SetStreamSource(0, GClearVertexBuffer.VertexBufferRHI, 0);
			RHICmdList.DrawPrimitive(0, 2, NumGS);
		});
}

FUEGSRenderData::FUEGSRenderData(FRDGBuilder& GraphBuilder, const FViewInfo& view, const FIntRect& viewportSubset, UGSAsset* GSAssetData)
	:F_EGP_ViewPersistentData(GraphBuilder,view,viewportSubset)
{
	this->NumGS = GSAssetData->NumGS;
	// Create preprocess data buffer, upload gs data from cpu side
	{
		FRHIResourceCreateInfo PreprocessDataBufferCreateInfo(TEXT("GS Preprocess Buffer"));
		const int BufferStride = sizeof(FVector4f);
		const int BufferNum = GSAssetData->NumGS * (sizeof(FGSPoint) / sizeof(FVector4f));
		PreprocessDataBuffer = FRHICommandListImmediate::Get().CreateBuffer(
			BufferStride * BufferNum,
			BUF_Static | BUF_ShaderResource | BUF_StructuredBuffer,
			BufferStride,
			ERHIAccess::SRVCompute,
			PreprocessDataBufferCreateInfo);
		void* Dest = FRHICommandListImmediate::Get().LockBuffer(
			PreprocessDataBuffer,
			0,
			BufferStride * BufferNum,
			RLM_WriteOnly);
		FMemory::Memcpy(Dest, GSAssetData->GetData() , BufferStride * BufferNum);
		FRHICommandListImmediate::Get().UnlockBuffer(PreprocessDataBuffer);
		
		FRDGBufferDesc preprocessDesc = FRDGBufferDesc::CreateBufferDesc(sizeof(FVector4f), NumGS *  (sizeof(FGSPoint) / sizeof(FVector4f)));
		PreprocessBufferPooled = new FRDGPooledBuffer(
		PreprocessDataBuffer,
		preprocessDesc,
		NumGS *  (sizeof(FGSPoint) / sizeof(FVector4f)),
		TEXT("Preprocess Buffer RDG"));
	}

	// Create rest of the buffer (We want to manage the buffer ourselves instead of leaving them to rdg)
	{
		FRHIResourceCreateInfo VertexAttributeBufferCreateInfo(TEXT("GS Vertex Attribute Buffer"));
		const int BufferStride = sizeof(FVector4f);
		const int BufferNum = GSAssetData->NumGS * (sizeof(FVertexAttribute) / sizeof(FVector4f));
		VertexAttributeBuffer = FRHICommandListImmediate::Get().CreateBuffer(
			BufferStride * BufferNum,
			BUF_UnorderedAccess | BUF_ShaderResource | BUF_StructuredBuffer,
			BufferStride,
			ERHIAccess::SRVMask | ERHIAccess::UAVMask,
			VertexAttributeBufferCreateInfo);
		
		FRDGBufferDesc vertexAttrDesc = FRDGBufferDesc::CreateBufferDesc(sizeof(FVector4f), NumGS * (sizeof(FVertexAttribute) / sizeof(FVector4f)));
		VertexAttributeBufferPooled = new FRDGPooledBuffer(
			VertexAttributeBuffer,
			vertexAttrDesc,
			NumGS * (sizeof(FVertexAttribute) / sizeof(FVector4f)),
			TEXT("Vertex Attr Buffer RDG"));
	}

	{
		FRHIResourceCreateInfo DepthKeyBufferCreateInfo(TEXT("GS Depth Key Buffer Ping"));
		const int BufferStride = sizeof(uint32);
		const int BufferNum = GSAssetData->NumGS;
		DepthKeyBufferPing = FRHICommandListImmediate::Get().CreateBuffer(
			BufferStride * BufferNum,
			BUF_UnorderedAccess | BUF_ShaderResource,
			BufferStride,
			ERHIAccess::SRVMask | ERHIAccess::UAVMask,
			DepthKeyBufferCreateInfo);
		
		FRDGBufferDesc depthKeyDesc = FRDGBufferDesc::CreateBufferDesc(sizeof(uint32), NumGS);
		DepthKeyBufferPingPooled = new FRDGPooledBuffer(
			DepthKeyBufferPing,
			depthKeyDesc,
			NumGS,
			TEXT("Depth Key Buffer Ping RDG"));
	}
	
	{
		FRHIResourceCreateInfo DepthKeyBufferCreateInfo(TEXT("GS Depth Key Buffer Pong"));
		const int BufferStride = sizeof(uint32);
		const int BufferNum = GSAssetData->NumGS;
		DepthKeyBufferPong = FRHICommandListImmediate::Get().CreateBuffer(
			BufferStride * BufferNum,
			BUF_UnorderedAccess | BUF_ShaderResource,
			BufferStride,
			ERHIAccess::SRVMask | ERHIAccess::UAVMask,
			DepthKeyBufferCreateInfo);
		
		FRDGBufferDesc depthKeyPongDesc = FRDGBufferDesc::CreateBufferDesc(sizeof(uint32), NumGS);
		DepthKeyBufferPongPooled = new FRDGPooledBuffer(
			DepthKeyBufferPong,
			depthKeyPongDesc,
			NumGS,
			TEXT("Depth Key Buffer Pong RDG"));
		
	
	}
	
	{
		FRHIResourceCreateInfo IndexValueBufferCreateInfo(TEXT("GS Index Value Buffer Ping"));
		const int BufferStride = sizeof(uint32);
		const int BufferNum = GSAssetData->NumGS;
		IndexValueBufferPing = FRHICommandListImmediate::Get().CreateBuffer(
			BufferStride * BufferNum,
			BUF_UnorderedAccess | BUF_ShaderResource,
			BufferStride,
			ERHIAccess::SRVMask | ERHIAccess::UAVMask,
			IndexValueBufferCreateInfo);
		
		FRDGBufferDesc indexValueDesc = FRDGBufferDesc::CreateBufferDesc(sizeof(uint32), NumGS);
		IndexValueBufferPingPooled = new FRDGPooledBuffer(
			IndexValueBufferPing,
			indexValueDesc,
			NumGS,
			TEXT("Index Value Buffer Ping RDG"));
	}

	{
		FRHIResourceCreateInfo IndexValueBufferCreateInfo(TEXT("GS Index Value Buffer Pong"));
		const int BufferStride = sizeof(uint32);
		const int BufferNum = GSAssetData->NumGS;
		IndexValueBufferPong = FRHICommandListImmediate::Get().CreateBuffer(
			BufferStride * BufferNum,
			BUF_UnorderedAccess | BUF_ShaderResource,
			BufferStride,
			ERHIAccess::SRVMask | ERHIAccess::UAVMask,
			IndexValueBufferCreateInfo);
		
		FRDGBufferDesc indexValuePongDesc = FRDGBufferDesc::CreateBufferDesc(sizeof(uint32), NumGS);
		IndexValueBufferPongPooled = new FRDGPooledBuffer(
			IndexValueBufferPong,
			indexValuePongDesc,
			NumGS,
			TEXT("Index Value Buffer Pong RDG"));
	}
}









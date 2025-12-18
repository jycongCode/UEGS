#pragma once
#include "GSAsset.generated.h"

struct FGSPoint
{
	FVector4f Position;
	FVector4f Scale;
	FVector4f Rotation;
	FVector4f DCA;
	FVector4f SH[15];
};

struct FVertexAttribute
{
	FVector4f NdcPosition;
	FVector4f Axis;
	FVector4f DcAlpha;
};

UCLASS(Blueprintable, BlueprintType, EditInlineNew, CollapseCategories)
class UGSAsset : public UObject
{
	GENERATED_BODY()
	
public:
	
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)
	FString AssetName;
	
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)
	int MaxSHDegree = 0;

	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)
	int NumGS = 0;
	
	UFUNCTION(BlueprintCallable)
	bool LoadFromFile(FString FilePath,FString TargetAssetName);

	UFUNCTION(BlueprintCallable)
	void Test()
	{
		for (int i = 0;i<8;++i)
		{
			float ix = (i&1)*2 - 1,iy = ((i>>1)&1)*2 - 1,iz = ((i>>2)&1)*2 - 1;
			FVector3f pos = {ix * 10,iy * 10,iz * 10};
			FGSPoint pt;
			pt.Position = pos;
			Points.Add(pt);
		}
		NumGS = 8;
	}
	
	void* GetData(){return Points.GetData();}
private:
	TArray<FGSPoint> Points;
};

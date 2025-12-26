#pragma once
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

class FGSAsset
{
	
public:
	int MaxSHDegree = 0;

	int NumGS = 0;
	
	bool LoadFromFile(FString FilePath);
	
	void* GetData(){return Points.GetData();}
private:
	TArray<FGSPoint> Points;
};

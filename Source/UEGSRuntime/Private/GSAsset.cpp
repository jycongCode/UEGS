#include "GSAsset.h"
#include <fstream>
#include <string>
#include <unordered_map>
static FVector4f Vector4fAbs(FVector4f v)
{
	return FVector4f(FMath::Abs(v.X),FMath::Abs(v.Y),FMath::Abs(v.Z),FMath::Abs(v.W));
}
static FQuat4f PackSmallest3Rotation(FVector4f q)
{
	// find biggest component
	FVector4f absQ = Vector4fAbs(q);
	int index = 0;
	float maxV = absQ.X;
	if (absQ.Y > maxV)
	{
		index = 1;
		maxV = absQ.Y;
	}
	if (absQ.Z > maxV)
	{
		index = 2;
		maxV = absQ.Z;
	}
	if (absQ.W > maxV)
	{
		index = 3;
		maxV = absQ.W;
	}

	if (index == 0) q = FVector4f(q.Y,q.Z,q.W,q.X);
	if (index == 1) q = FVector4f(q.X,q.Z,q.W,q.Y);
	if (index == 2) q = FVector4f(q.X,q.Y,q.W,q.Z);

	FVector3f three = FVector3f(q.X,q.Y,q.Z) * (q.W >= 0 ? 1 : -1); // -1/sqrt2..+1/sqrt2 range
	three = (three * FMath::Sqrt(2.0)) * 0.5f + 0.5f; // 0..1 range

	return FQuat4f(three.X,three.Y,three.Z, index / 3.0f);
}

bool FGSAsset::LoadFromFile(FString FilePath)
{
	std::ifstream IStream(TCHAR_TO_UTF8(*FilePath), std::ios::binary);
	if (!IStream.is_open()) {
		UE_LOG(LogTemp, Warning, TEXT("Unable to open: %s"), *FilePath);
		return false;
	}
	if (!IStream.good()) {
		UE_LOG(LogTemp, Warning, TEXT("Unable to read from input stream."));
		return false;
	}

	std::string line;
	std::getline(IStream, line);
	if (line != "ply") {
		UE_LOG(LogTemp, Warning, TEXT("Input data is not a .ply file."));
		return false;
	}

	std::getline(IStream, line);
	if (line != "format binary_little_endian 1.0") {
		UE_LOG(LogTemp, Warning, TEXT("Unsupported .ply format."));
		return false;
	}

	std::getline(IStream, line);
	if (line.find("element vertex ") != 0) {
		UE_LOG(LogTemp, Warning, TEXT("Missing vertex count."));
		return false;
	}

	int numPoints = std::stoi(line.substr(std::strlen("element vertex ")));

	if (numPoints <= 0 || numPoints > 10 * 1024 * 1024) {
		UE_LOG(LogTemp, Warning, TEXT("Invalid vertex count: %d"), numPoints);
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("Loading %d points"), numPoints);
	std::unordered_map<std::string, int> fields;
	for (int i = 0;i<numPoints; i++) {
		if (!std::getline(IStream, line)) {
			UE_LOG(LogTemp, Warning, TEXT("Unexpected end of header."));
			return false;
		}

		if (line == "end_header")
			break;

		if (line.find("property float ") != 0) {
			UE_LOG(LogTemp, Warning, TEXT("Unsupported property data type"));
			return false;
		}
		std::string name = line.substr(std::strlen("property float "));
		fields[name] = i;
	}

	// Returns the index for a given field name, ensuring the name exists.
	const auto index = [&fields](const std::string& name) {
		const auto& itr = fields.find(name);
		if (itr == fields.end()) {
			UE_LOG(LogTemp, Warning, TEXT("Missing field"));
			return 0;
		}
		return itr->second;
		};

	const std::vector<int> positionIdx = { index("x"), index("y"), index("z") };
	const std::vector<int> scaleIdx = { index("scale_0"), index("scale_1"),
									   index("scale_2") };
	const std::vector<int> rotIdx = { index("rot_1"), index("rot_2"),
									 index("rot_3"), index("rot_0") };
	const std::vector<int> alphaIdx = { index("opacity") };
	const std::vector<int> colorIdx = { index("f_dc_0"), index("f_dc_1"),
									   index("f_dc_2") };
	// Check that only valid indices were returned.
	auto checkIndices = [&](const std::vector<int>& idxVec) -> bool {
		for (auto idx : idxVec) {
			if (idx < 0) {
				return false;
			}
		}
		return true;
		};

	if (!checkIndices(positionIdx) || !checkIndices(scaleIdx) ||
		!checkIndices(rotIdx) || !checkIndices(alphaIdx) ||
		!checkIndices(colorIdx)) {
		return false;
	}

	// Spherical harmonics are optional and variable IStream size (depending on degree)
	std::vector<int> shIdx;
	for (int i = 0; i < 15; i++) {
		const auto& itr = fields.find("f_rest_" + std::to_string(i));
		if (itr == fields.end())
			break;
		shIdx.push_back(itr->second);
	}
	
	// If spherical harmonics fields are present, ensure they are complete
	if (shIdx.size() % 3 != 0) {
		UE_LOG(LogTemp, Warning, TEXT("Incomplete spherical harmonics fields."));
		return false;
	}
	
	std::vector<float> values;
	values.resize(numPoints * fields.size());

	IStream.read(reinterpret_cast<char*>(values.data()),
		values.size() * sizeof(float));
	
	if (!IStream.good()) {
		UE_LOG(LogTemp, Warning, TEXT("Unable to load data from input stream."));
		return false;
	}

	std::vector<float> Position(numPoints * 3);
	std::vector<float> Scale(numPoints * 3);
	std::vector<float> Rotation(numPoints * 4);
	std::vector<float> Alpha(numPoints);
	std::vector<float> Color(numPoints * 3);
	std::vector<float> SH(numPoints * shIdx.size() * 3);
	for (size_t i = 0; i < static_cast<size_t>(numPoints); i++)
	{
		size_t vertexOffset = i * fields.size();
		// Position
		Position[i * 3 + 0] = 100.0f * values[vertexOffset + positionIdx[0]];
		Position[i * 3 + 1] = 100.0f * values[vertexOffset + positionIdx[2]];
		Position[i * 3 + 2] = 100.0f * values[vertexOffset + positionIdx[1]];
		// Scale
		Scale[i * 3 + 0] = 100.0f * FMath::Exp(values[vertexOffset + scaleIdx[0]]);
		Scale[i * 3 + 1] = 100.0f * FMath::Exp(values[vertexOffset + scaleIdx[2]]);
		Scale[i * 3 + 2] = 100.0f * FMath::Exp(values[vertexOffset + scaleIdx[1]]);
		// Rotation
		FQuat4f Quat = FQuat4f(
			values[vertexOffset + rotIdx[1]],
			values[vertexOffset + rotIdx[2]],
			values[vertexOffset + rotIdx[3]],
			values[vertexOffset + rotIdx[0]]
		);
		Quat.Normalize();
		Rotation[i * 4 + 0] = Quat.X;
		Rotation[i * 4 + 1] = Quat.Y;
		Rotation[i * 4 + 2] = Quat.Z;
		Rotation[i * 4 + 3] = Quat.W;
		// Alpha
		Alpha[i] = 1.0f / (1.0f + FMath::Exp(-values[vertexOffset + alphaIdx[0]]));
		Alpha[i] = FMath::Clamp(Alpha[i],0.0f,1.0f);
		// Color
		Color[i * 3 + 0] = values[vertexOffset + colorIdx[0]];
		Color[i * 3 + 1] = values[vertexOffset + colorIdx[1]];
		Color[i * 3 + 2] = values[vertexOffset + colorIdx[2]];

		// SH Coeffs
		for (size_t j = 0; j < shIdx.size(); j+=3)
		{
			SH[(i * shIdx.size() + j) * 3 + 0] = FMath::Clamp(values[vertexOffset + shIdx[j+0]] * 0.25f,-1.0f,1.0f);
			SH[(i * shIdx.size() + j) * 3 + 0] = FMath::Clamp(values[vertexOffset + shIdx[j+1]] * 0.25f,-1.0f,1.0f);
			SH[(i * shIdx.size() + j) * 3 + 0] = FMath::Clamp(values[vertexOffset + shIdx[j+2]] * 0.25f,-1.0f,1.0f);
		}
	}
	// Asset->SetAsset(Position,Scale,Rotation,Alpha,Color,SH,numPoints,DegreeFromDim(shIdx.size()));

	Points.SetNum(numPoints);
	for (size_t i = 0; i < numPoints; i++)
	{
		FVector3f Positions = {Position[i * 3 + 0],Position[i * 3 + 1],Position[i * 3 + 2]}; 
		FVector3f Scales = {Scale[i * 3 + 0],Scale[i * 3 + 1],Scale[i * 3 + 2]};
		FVector4f Rotations = {{Rotation[i * 4 + 1],Rotation[i * 4 + 2],Rotation[i * 4 + 3],Rotation[i * 4 + 0]},Rotation[i * 4 + 3]};
		float Alphas = Alpha[i];
		FVector3f F_Dc = {Color[i * 3 + 0]*0.28209479177387814f+0.5f,Color[i * 3 + 1]*0.28209479177387814f+0.5f,Color[i * 3 + 2] * 0.28209479177387814f+0.5f};
		const int ShDim = shIdx.size() / 3 + 1;
		
		for (size_t j = 0; j < FMath::Min(ShDim,15); j++)
		{
			FVector3f Sh = {SH[(i * ShDim + j) * 3 + 0],SH[(i * ShDim + j) * 3 + 1],SH[(i * ShDim + j) * 3 + 2]};
			Points[i].SH[j] = Sh;
		}
		Points[i].Position = {Positions,1.0f};
		Points[i].Rotation = Rotations;
		Points[i].Scale = Scales;
		Points[i].DCA = {F_Dc,Alphas};
	}
	NumGS = numPoints;
	MaxSHDegree = FMath::Min(sqrt((shIdx.size() / 3 + 1)) - 1,3);
	return true;
}




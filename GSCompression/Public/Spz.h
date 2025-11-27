#pragma once
#include <vector>

struct GSCOMPRESSION_API GaussianCloudSim
{
	int32_t numPoints = 0;
	int32_t shDegree = 0;
	std::vector<float> positions;
	std::vector<float> scales;
	std::vector<float> rotations;
	std::vector<float> alphas;
	std::vector<float> colors;
	std::vector<float> sh;
};

namespace GSCompress_Spz
{
	void GSCOMPRESSION_API Compress(const GaussianCloudSim& gaussians,std::vector<uint8_t> *output);
	void GSCOMPRESSION_API Decompress(GaussianCloudSim &gaussians,const std::vector<uint8_t> &input);
}
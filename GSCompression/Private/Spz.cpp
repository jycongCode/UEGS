#include "Spz.h"
#include "Spz/load-spz.h"
spz::GaussianCloud ConvertToSpz(const GaussianCloudSim& gaussians)
{
	return {
		.numPoints = gaussians.numPoints,
		.shDegree = gaussians.shDegree,
		.antialiased = false,
		.positions = gaussians.positions,
		.scales = gaussians.scales,
		.rotations = gaussians.rotations,
		.alphas = gaussians.alphas,
		.colors = gaussians.colors,
		.sh = gaussians.sh,
	};
}

GaussianCloudSim ConvertToSim(const spz::GaussianCloud&gaussians)
{
	return {
		.numPoints = gaussians.numPoints,
		.shDegree = gaussians.shDegree,
		.positions = gaussians.positions,
		.scales = gaussians.scales,
		.rotations = gaussians.rotations,
		.alphas = gaussians.alphas,
		.colors = gaussians.colors,
		.sh = gaussians.sh,
	};
}

void GSCompress_Spz::Compress(const GaussianCloudSim &gaussians,std::vector<uint8_t> *output)
{
	spz::PackOptions packOptions{.from = spz::CoordinateSystem::UNSPECIFIED};
	auto gaussianspz = ConvertToSpz(gaussians);
	spz::saveSpz(gaussianspz,packOptions,output);
}

void GSCompress_Spz::Decompress(GaussianCloudSim &gaussians, const std::vector<uint8_t> &input)
{
	spz::UnpackOptions unpackOptions{.to = spz::CoordinateSystem::RUF};
	auto gaussianspz = spz::loadSpz(input,unpackOptions);
	gaussians = ConvertToSim(gaussianspz);
}

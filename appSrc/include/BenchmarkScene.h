#pragma once
#include <random>

#include "ResourcePaths.h"
#include "EngineAPI.h"

constexpr uint32_t NUM_OF_SPHERES = 2000;
constexpr uint32_t NUM_OF_LIGHTS = 10;


class BenchmarkScene
{
public:

	BenchmarkScene();

	void init(EngineAPI* engine, int numSpheres = NUM_OF_SPHERES,int numLights = NUM_OF_LIGHTS);


	void update();


private:
	void generateFloor(Scene& scene, const MeshHandle& mesh, const MaterialHandle& mat, int titerations);
	
	EngineAPI* _engine = nullptr;

	//random tools
	std::mt19937 _gen;
	std::random_device _rd;  // a seed source for the random number engine
};


#include "BenchmarkScene.h"
#include "EngineAPI.h"

BenchmarkScene::BenchmarkScene()
{
}

void BenchmarkScene::generateFloor(Scene& scene, const MeshHandle& mesh, const MaterialHandle& mat, int iterations)
{
	int planeScale = 15;
	for (int i = -iterations; i < iterations; i++) {
		for (int j = -iterations; j < iterations; j++) {
			const auto floorEntity = scene.createEntity(
				mesh,
				mat,
				Transform{
					glm::vec3(static_cast<float>(i) * -planeScale, -1, static_cast<float>(j) * planeScale),
					glm::vec3(0.0f),
					glm::vec3(planeScale)
				}
			);
		}
	}
}

void BenchmarkScene::init(EngineAPI* engine, int numSpheres, int numLights)
{
	float limiter = (numSpheres + numLights) / 2.f; //std::max(numSpheres, numLights);

	_engine = engine;
	assert(_engine != nullptr);
	Scene& scene = _engine->getScene();
	ResourceManager& resources = _engine->getResourceManager();


	TextureHandle whiteTextureHandle = resources.createTexture("tex_white", TEXTURE2_PATH);
	TextureHandle wallColor = resources.createTexture("tex_wall_basecolor", WALL_TEXTURE_PATH);
	TextureHandle wallNormal = resources.createTexture("tex_wall_normal", WALL_NORMAL_TEXTURE_PATH);

	MaterialCreateInfo mat3;
	mat3.metallic = 0.01f;
	mat3.roughness = 0.8f;
	mat3.baseColorTexture = wallColor;
	mat3.normalTexture = wallNormal;

	MaterialCreateInfo planeMat;
	planeMat.metallic = 0.0f;
	planeMat.roughness = 0.5f;
	planeMat.baseColorTexture = whiteTextureHandle;

	MaterialHandle mat3Handle = resources.createMaterial("mat_wall", mat3);
	MaterialHandle planeMatHandle = resources.createMaterial("mat_plane", planeMat);

	MeshHandle esfera = resources.createMesh("mesh_sphere", MODEL_PATH3);
	MeshHandle planoSincolor = resources.createMesh("mesh_plane_no_color", MODEL_PATH4);

	std::normal_distribution<float> roughnessRan(0.5, 0.5);
	std::normal_distribution<float> metallicRan(0.5, 0.2);

	std::normal_distribution<float> scaleRan(0.6, 0.3);

	std::uniform_real_distribution<float> positionXRan( - std::max(limiter / 100,5.f), std::max(limiter / 100, 5.f));
	std::uniform_real_distribution<float> positionZRan(-std::max(limiter / 100, 5.f), std::max(limiter / 100, 5.f));
	std::uniform_real_distribution<float> positionYRan(0, std::clamp(limiter / 500,5.f,200.f));


	for (int i = 0; i < numSpheres; i++) {
		MaterialCreateInfo dynamicMat = mat3;
		dynamicMat.roughness = std::clamp(roughnessRan(_gen), 0.001f, 1.f); //0.5f; // std::min( static_cast<float>( i ) * 0.1f, 1.0f );
		dynamicMat.metallic = std::clamp(metallicRan(_gen), 0.001f, 1.f);//0.5f; //std::min( static_cast<float>( j ) * 0.1f, 1.0f );
		MaterialHandle dynamicMatHandle = resources.createMaterial("mat_grid_" + std::to_string(i) /*+ "_" + std::to_string(j)*/, dynamicMat);

		float aux = std::max(scaleRan(_gen), 0.1f);

		const auto gridEntity = scene.createEntity(
			esfera,
			dynamicMatHandle,
			Transform{
				glm::vec3(positionXRan(_gen), aux+ positionYRan(_gen) , positionZRan(_gen)),//glm::vec3( -5.0f + static_cast<float>( i ) * -2.0f, k*2.f, static_cast<float>( j ) * 2.0f ),
				glm::vec3(0.0f),
				glm::vec3(aux)
			}
		);
	}

	//generateFloor(scene, planoSincolor, planeMatHandle, 5 );


	//std::uniform_real_distribution<float> positionXRan(-50, 50);
	//std::uniform_real_distribution<float> positionZRan(-50, 50);

	std::normal_distribution<float> intensity(0.3, 0.001);
	std::normal_distribution<float> r(0.5, 0.5);
	std::normal_distribution<float> g(0.5, 0.5);
	std::normal_distribution<float> b(0.5, 0.5);

	float inten = intensity(_gen);

	for (int k = 0; k < numLights; k++) {
		LightEntityHandle gridLight = scene.createLight(LightType::Point, glm::vec3(positionXRan(_gen), 1 + positionYRan(_gen), positionZRan(_gen)), glm::vec3(r(_gen), g(_gen), b(_gen)), inten, 100);
	}


	LightEntityHandle mainLight = scene.createLight(LightType::Directional, glm::vec3(-0.1, -0.9,0.1), glm::vec3(1, 1, 1), 0.1);
	scene.setMainLight(mainLight);

	scene.getCamera().setPosition(glm::vec3(0.f, std::clamp(limiter / 500, 5.f, 200.f) /2.f,0.f));
}

void BenchmarkScene::update()
{
}

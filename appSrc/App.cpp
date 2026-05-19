#include "App.h"
#include <fstream>
#include <chrono>
#include<algorithm>
#include <glm/gtc/matrix_transform.hpp>

void App::safeCleanup(bool& initialized) noexcept {
	if (!initialized) {
		return;
	}

	try {
		_engine.wait();
		freeObjects();
		_engine.cleanup();
	}
	catch (...) {
	}

	initialized = false;
}

void App::createCandleGrid(Scene& scene, const MeshHandle& mesh, const MaterialHandle& mat)
{

	for (int i = 0; i < 30; i++) {
		for (int j = 0; j < 30; j++) {
			_candleEntity.push_back(scene.createEntity(
				mesh,
				mat,
				Transform{
					glm::vec3(3 + j * 0.5,4,-4 + i * 0.5),
					glm::vec3(0.0f),
					glm::vec3(0.1f)
				}
			));
		}
	}
}


bool App::run() {
	bool initialized = false;

	try {
        _engine.init( "AppExample" );
		initialized = true;

		mainLoop();

        safeCleanup( initialized );
		return true;
	}
	catch (const SceneException& e) {
        std::cerr << "Scene error [" << static_cast<int>( e.code() ) << "]: " << e.what() << "\n";
        safeCleanup( initialized );
	}
	catch (const ResourceException& e) {
        std::cerr << "Resource error [" << static_cast<int>( e.code() ) << "]: " << e.what() << "\n";
        safeCleanup( initialized );
	}
	catch (const std::exception& e) {
		std::cerr << "Engine runtime error: " << e.what() << "\n";
        safeCleanup( initialized );
	}
	catch (...) {
		std::cerr << "Unknown fatal error\n";
        safeCleanup( initialized );
	}

	return false;
}

void App::start()
{

	_mainCamera = &_engine.getScene().getCamera();

	//auto startTime = std::chrono::high_resolution_clock::now();
	//loadModels();
	//auto endTime = std::chrono::high_resolution_clock::now();
	//std::cout << "Tiempo en cargar los modelos: " <<
	//	std::chrono::duration<float, std::chrono::seconds::period>(endTime - startTime).count() << "\n";


	//startTime = std::chrono::high_resolution_clock::now();
	//addLighting();
	//endTime = std::chrono::high_resolution_clock::now();
	//std::cout << "Tiempo en cargar las luces: " <<
	//	std::chrono::duration<float, std::chrono::seconds::period>(endTime - startTime).count() << "\n";
	_benchScene.init(&_engine,10000,1800);
}

void App::mainLoop() {
	static auto startTime = std::chrono::high_resolution_clock::now();
	start();
	SDL_SetRelativeMouseMode(SDL_TRUE);

	bool running = true;
	while (running)
	{
		auto frameStart = std::chrono::high_resolution_clock::now();

		SDL_Event ev;
		while (SDL_PollEvent(&ev)) {

			if (ev.type == SDL_KEYDOWN) {
				if (ev.key.keysym.scancode == SDL_SCANCODE_D) {
					_axisInput[0] = true;
				}
				if (ev.key.keysym.scancode == SDL_SCANCODE_A) {
					_axisInput[1] = true;
				}
				if (ev.key.keysym.scancode == SDL_SCANCODE_W) {
					_axisInput[2] = true;
				}
				if (ev.key.keysym.scancode == SDL_SCANCODE_S) {
					_axisInput[3] = true;
				}

				if (ev.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
					SDL_bool b = SDL_GetRelativeMouseMode();
					SDL_SetRelativeMouseMode((SDL_bool)(((int)b + 1) % 2));
				}
			}

			if (ev.type == SDL_KEYUP) {
				if (ev.key.keysym.scancode == SDL_SCANCODE_D) {
					_axisInput[0] = false;
				}
				if (ev.key.keysym.scancode == SDL_SCANCODE_A) {
					_axisInput[1] = false;
				}
				if (ev.key.keysym.scancode == SDL_SCANCODE_W) {
					_axisInput[2] = false;
				}
				if (ev.key.keysym.scancode == SDL_SCANCODE_S) {
					_axisInput[3] = false;
				}
			}

			if (ev.type == SDL_MOUSEMOTION) {
				_mainCamera->rotateY(glm::radians(static_cast<float>(ev.motion.xrel) * 0.1f));
				_mainCamera->rotateX(glm::radians(static_cast<float>(ev.motion.yrel) * 0.1f));
			}

			if (ev.type == SDL_MOUSEWHEEL) {
				_movementSpeed += ev.wheel.preciseY;
				_movementSpeed = std::max(0.f, _movementSpeed);
			}

			if (ev.type == SDL_QUIT) {
				running = false;
			}
			if (ev.type == SDL_WINDOWEVENT) {
				WindowEvent out;
				out.type = WindowEventType::Unknown;
				out.width = ev.window.data1;
				out.height = ev.window.data2;

				if (ev.window.event == SDL_WINDOWEVENT_RESIZED) {
					out.type = WindowEventType::Resized;
				}

				_engine.handleWindowEvent(out);
			}
		}

		_moveDir = glm::vec3(0);
		if (_axisInput[0]) {
			_moveDir += glm::vec3(-1, 0, 0);
		}
		if (_axisInput[1]) {
			_moveDir += glm::vec3(1, 0, 0);
		}
		if (_axisInput[2]) {
			_moveDir += glm::vec3(0, 0, 1);
		}
		if (_axisInput[3]) {
			_moveDir += glm::vec3(0, 0, -1);
		}

		if (_moveDir != glm::vec3(0)) {
			_moveDir = glm::normalize(_moveDir);
		}



		update();

		_engine.drawFrame();

		auto endFrame = std::chrono::high_resolution_clock::now();

		_counter++;

		_deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(endFrame - frameStart).count();

	}
}

void App::update()
{
	//static auto startTime = std::chrono::high_resolution_clock::now();

	//auto currentTime = std::chrono::high_resolution_clock::now();
	//float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	//if (_triangleEntity.isValid()) {
	//	_triangleEntity.setTransform(
	//		glm::vec3(sin(time) * 1.5f, 0.0f, 0.0f),
	//		glm::vec3(0.0f, glm::radians(-10.0f), 0.0f)
	//	);
	//}

	//if (_characterEntity.isValid()) {
	//	_characterEntity.setTransform(
	//		glm::vec3(0.0f, -1.0f, 0.0f),
	//		glm::vec3(0.0f, time * glm::radians(180.0f), 0.0f)
	//	);
	//}

	_benchScene.update();

	glm::vec3 right = glm::cross(glm::vec3(0, 1, 0), _mainCamera->getForward());


	glm::vec3 move = normalize(right) * _moveDir.x + normalize(_mainCamera->getForward()) * _moveDir.z;

	glm::vec3 newPos = _mainCamera->getPosition() + move * _deltaTime * _movementSpeed;


	_mainCamera->setPosition(newPos);
}



void App::loadModels()
{
	Scene& scene = _engine.getScene();
	ResourceManager& resources = _engine.getResourceManager();

	TextureHandle characterTextureHandle = resources.createTexture("tex_character_basecolor", TEXTURE_PATH);
	TextureHandle whiteTextureHandle = resources.createTexture("tex_white2", TEXTURE2_PATH);
	TextureHandle koreanoTextureHandle = resources.createTexture("tex_koreano", TEXTURE3_PATH);
	TextureHandle normalTexture = resources.createTexture("tex_character_normal", NORMAL_TEXTURE_PATH);
	TextureHandle wallColor = resources.createTexture("tex_wall_basecolor2", WALL_TEXTURE_PATH);
	TextureHandle wallNormal = resources.createTexture("tex_wall_normal2", WALL_NORMAL_TEXTURE_PATH);
	TextureHandle candleBaseColor = resources.createTexture("tex_candle_baseCol", "./textures/Candle_Color.png");
	TextureHandle candleNormal = resources.createTexture("tex_candle_normal", "./textures/Candle_Normal.png");

	std::vector<Vertex> vertices;
	std::vector<Vertex> vertices2;

	Vertex v1;
	Vertex v2;
	Vertex v3;

	int dim = 1;

	v1.pos = glm::vec3(-dim, 0, 0);
	v2.pos = glm::vec3(0, dim, 0);
	v3.pos = glm::vec3(dim, 0, 0);

	v1.color = glm::vec3(1.f, 1.0f, 1.f);
	v2.color = glm::vec3(1.f, 1.f, 1.f);
	v3.color = glm::vec3(1.f, 1.f, 1.f);

	v1.normal = glm::vec3(0.f, 0.0f, -1.f);
	v2.normal = glm::vec3(0.f, 0.0f, -1.f);
	v3.normal = glm::vec3(0.f, 0.0f, -1.f);

	v1.texCoord = glm::vec2(0, 0);
	v2.texCoord = glm::vec2(2, 0);
	v3.texCoord = glm::vec2(0, 2);

	v1.tangent = glm::vec3(normalize(v1.pos - v2.pos));
	v2.tangent = glm::vec3(normalize(v1.pos - v2.pos));
	v3.tangent = glm::vec3(normalize(v1.pos - v2.pos));


	vertices.push_back(v1);
	vertices.push_back(v2);
	vertices.push_back(v3);

	MaterialCreateInfo mat1;
	mat1.metallic = 0.0f;
	mat1.roughness = 0.95f;
	mat1.baseColorTexture = characterTextureHandle;
	mat1.normalTexture = normalTexture;

	MaterialCreateInfo mat2;
	mat2.metallic = 0.1f;
	mat2.roughness = 0.25f;
	mat2.baseColorTexture = koreanoTextureHandle;
	mat2.normalTexture = wallNormal;

	MaterialCreateInfo mat3;
	mat3.metallic = 0.01f;
	mat3.roughness = 0.8f;
	mat3.baseColorTexture = wallColor;
	mat3.normalTexture = wallNormal;

	MaterialCreateInfo planeMat;
	planeMat.metallic = 0.0f;
	planeMat.roughness = 0.5f;
	planeMat.baseColorTexture = whiteTextureHandle;

	MaterialCreateInfo candleMat;
	candleMat.metallic = 0;
	candleMat.roughness = 0.2;
	candleMat.baseColorTexture = candleBaseColor;
	//candleMat.baseColor = glm::vec4(1);
	candleMat.normalTexture = candleNormal;

	MaterialHandle mat1Handle = resources.createMaterial("mat_character", mat1);
	MaterialHandle mat2Handle = resources.createMaterial("mat_koreano", mat2);
	MaterialHandle mat3Handle = resources.createMaterial("mat_wall2", mat3);
	MaterialHandle planeMatHandle = resources.createMaterial("mat_plane2", planeMat);
	MaterialHandle candleMaterial = resources.createMaterial("mat_candle", candleMat);


	MeshHandle triangle = resources.createMesh("mesh_triangle", vertices);
	MeshHandle character = resources.createMesh("mesh_character", MODEL_PATH);
	MeshHandle esfera = resources.createMesh("mesh_sphere2", MODEL_PATH3);
	MeshHandle planoSincolor = resources.createMesh("mesh_plane_no_color2", MODEL_PATH4);
	MeshHandle candle = resources.createMesh("candle", "./mesh/candle-low.obj");

	_triangleEntity = scene.createEntity(
	    triangle,
	    mat3Handle,
	    Transform{
	        glm::vec3( 0.0f ),
	        glm::vec3( 0.0f ),
	        glm::vec3( 1.0f )
	    }
	);

	_characterEntity = scene.createEntity(
	    character,
	    mat1Handle,
	    Transform{
	        glm::vec3( 0.0f ),
	        glm::vec3( 0.0f ),
	        glm::vec3( 1.0f )
	    }
	);

	_sphereLeftEntity = scene.createEntity(
	    esfera,
	    mat2Handle,
	    Transform{
	        glm::vec3( 2.5f, 0.0f, 0.0f ),
	        glm::vec3( 0.0f ),
	        glm::vec3( 1.0f )
	    }
	);

	_sphereRightEntity = scene.createEntity(
	    esfera,
	    mat2Handle,
	    Transform{
	        glm::vec3( -2.5f, 0.0f, 0.0f ),
	        glm::vec3( 0.0f ),
	        glm::vec3( 1.0f )
	    }
	);

	//for (int i = -4; i < 4; i++) {
	//	for (int j = -4; j < 4; j++) {
	//		const auto floorEntity = scene.createEntity(
	//			planoSincolor,
	//			planeMatHandle,
	//			Transform{
	//				glm::vec3(static_cast<float>(i) * -4, -1, static_cast<float>(j) * 4),
	//				glm::vec3(0.0f),
	//				glm::vec3(4)
	//			}
	//		);
	//	}
	//}


	//std::normal_distribution<float> roughnessRan(0.5, 0.5);
	//std::normal_distribution<float> metallicRan(0.5, 0.2);

	//std::normal_distribution<float> scaleRan(0.6,0.3);


	//std::uniform_real_distribution<float> positionXRan(-50, 50);
	//std::uniform_real_distribution<float> positionZRan(-50, 50);


	//for (int i = 0; i < NUM_OF_SPHERES; i++) {
	//	//for (int j = 0; j < 50; j++) {
	//		MaterialCreateInfo dynamicMat = mat3;
	//		dynamicMat.roughness = std::clamp(roughnessRan(_gen), 0.001f,1.f); //0.5f; // std::min( static_cast<float>( i ) * 0.1f, 1.0f );
	//		dynamicMat.metallic = std::clamp(metallicRan(_gen), 0.001f,1.f);//0.5f; //std::min( static_cast<float>( j ) * 0.1f, 1.0f );
	//		MaterialHandle dynamicMatHandle = resources.createMaterial("mat_grid_" + std::to_string(i) /*+ "_" + std::to_string(j)*/, dynamicMat);

	//		float aux = std::max(scaleRan(_gen),0.1f);

	//		const auto gridEntity = scene.createEntity(
	//			esfera,
	//			dynamicMatHandle,
	//			Transform{
	//				glm::vec3(positionXRan(_gen), aux, positionZRan(_gen)),//glm::vec3( -5.0f + static_cast<float>( i ) * -2.0f, k*2.f, static_cast<float>( j ) * 2.0f ),
	//				glm::vec3(0.0f),
	//				glm::vec3(aux)
	//			}
	//		);
	//	//}
	//}

	//generateFloor(scene,planoSincolor,planeMatHandle,15.f);

}

void App::addLighting()
{
	Scene& scene = _engine.getScene();

	LightEntityHandle mainLight = scene.createLight(LightType::Directional, glm::vec3(-0, -0.9, 0.0001), glm::vec3(1, 1, 1), 0.05);
	LightEntityHandle redLight = scene.createLight( LightType::Point, glm::vec3( 0, 0, -1 ), glm::vec3( 1, 0, 0 ), 0.2f, 10 );
	LightEntityHandle greenLight = scene.createLight( LightType::Point, glm::vec3( 1, 0, -1 ), glm::vec3( 0, 1, 0 ), 0.2f, 10 );
	LightEntityHandle blueLight = scene.createLight( LightType::Point, glm::vec3( -1, 0, -1 ), glm::vec3( 0, 0, 1 ), 0.2f, 10 );



	//std::uniform_real_distribution<float> positionXRan(-50, 50);
	//std::uniform_real_distribution<float> positionZRan(-50, 50);

	//std::normal_distribution<float> intensity(0.2,0.001);
	//std::normal_distribution<float> r(0.5,0.5);
	//std::normal_distribution<float> g(0.5,0.5);
	//std::normal_distribution<float> b(0.5,0.5);

	//float inten = intensity(_gen);

	//for (int k = 0; k < NUM_OF_LIGHTS; k++) {
	//	LightEntityHandle gridLight = scene.createLight(LightType::Point, glm::vec3(positionXRan(_gen), 1, positionZRan(_gen)), glm::vec3(r(_gen), g(_gen), b(_gen)), inten, 50);
	//}



	////3Dgrid of lights
	//for (int i = 0; i < 20; i++) {
	//    for (int j = 0; j < 20; j++) {
	//        for (int k = 0; k < 4; k++) {
	//            LightEntityHandle gridLight = scene.createLight( LightType::Point, glm::vec3( -5 + i * -2, k*4  + 2, j * 2 - 1 ), glm::vec3( 1 , 0.4 , 0.8 ), 0.1, 20 );
	//        }
	//    }
	//}

	//for (int i = 0; i < _candleEntity.size(); i++) {
	//	LightEntityHandle candleLight = scene.createLight(LightType::Point, _candleEntity[i].getPosition() + glm::vec3(0, 0.2, 0), glm::vec3(226, 135, 67) * 1.f / 255.f, 0.05f, 10);
	//}

	scene.setMainLight(mainLight);
}

void App::freeObjects()
{
	ResourceManager& resources = _engine.getResourceManager();
	_engine.getScene().clear();
	resources.clear();
}


App::App()
{
	_gen = std::mt19937(_rd()); // mersenne_twister_engine seeded with rd()
	/* _window.init("Proyecto Vulkan",WIDTH,HEIGHT,_instance);*/
}


#include "DefaultSceneLayer.h"

// GLM math library
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp> 
#define GLM_ENABLE_EXPERIMENTAL
#include <GLM/gtx/common.hpp> // for fmod (floating modulus)

#include <filesystem>
 
// Graphics 
#include "Graphics/Buffers/IndexBuffer.h"
#include "Graphics/Buffers/VertexBuffer.h"
#include "Graphics/VertexArrayObject.h"
#include "Graphics/ShaderProgram.h"
#include "Graphics/Textures/Texture2D.h" 
#include "Graphics/Textures/TextureCube.h"
#include "Graphics/VertexTypes.h"
#include "Graphics/Font.h"
#include "Graphics/GuiBatcher.h"
#include "Graphics/Framebuffer.h"

// Utilities
#include "Utils/MeshBuilder.h"
#include "Utils/MeshFactory.h"
#include "Utils/ObjLoader.h"
#include "Utils/ImGuiHelper.h"
#include "Utils/ResourceManager/ResourceManager.h"
#include "Utils/FileHelpers.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/StringUtils.h"
#include "Utils/GlmDefines.h"

// Gameplay
#include "Gameplay/Material.h"
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"

// Components
#include "Gameplay/Components/IComponent.h"
#include "Gameplay/Components/Camera.h"
#include "Gameplay/Components/RotatingBehaviour.h"
#include "Gameplay/Components/JumpBehaviour.h"
#include "Gameplay/Components/RenderComponent.h"
#include "Gameplay/Components/MaterialSwapBehaviour.h"
#include "Gameplay/Components/TriggerVolumeEnterBehaviour.h"
#include "Gameplay/Components/SimpleCameraControl.h"
#include "Gameplay/Components/TestComponent.h"

// Physics
#include "Gameplay/Physics/RigidBody.h"   
#include "Gameplay/Physics/Colliders/BoxCollider.h"
#include "Gameplay/Physics/Colliders/PlaneCollider.h"
#include "Gameplay/Physics/Colliders/SphereCollider.h"
#include "Gameplay/Physics/Colliders/ConvexMeshCollider.h"
#include "Gameplay/Physics/Colliders/CylinderCollider.h"
#include "Gameplay/Physics/TriggerVolume.h"
#include "Graphics/DebugDraw.h"

// GUI
#include "Gameplay/Components/GUI/RectTransform.h"
#include "Gameplay/Components/GUI/GuiPanel.h"
#include "Gameplay/Components/GUI/GuiText.h"
#include "Gameplay/InputEngine.h"

#include "Application/Application.h"
#include "Gameplay/Components/ParticleSystem.h"
#include "Graphics/Textures/Texture3D.h"
#include "Graphics/Textures/Texture1D.h"

DefaultSceneLayer::DefaultSceneLayer() :
	ApplicationLayer()
{
	Name = "Default Scene";
	Overrides = AppLayerFunctions::OnAppLoad;
}

DefaultSceneLayer::~DefaultSceneLayer() = default;

void DefaultSceneLayer::OnAppLoad(const nlohmann::json& config) {
	_CreateScene();
}

void DefaultSceneLayer::_CreateScene()
{
	using namespace Gameplay;
	using namespace Gameplay::Physics;

	Application& app = Application::Get(); 

	bool loadScene = false; 
	// For now we can use a toggle to generate our scene vs load from file
	if (loadScene && std::filesystem::exists("scene.json")) {
		app.LoadScene("scene.json");
	}
	else {
		// This time we'll have 2 different shaders, and share data between both of them using the UBO
		// This shader will handle reflective materials 
		ShaderProgram::Sptr reflectiveShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_environment_reflective.glsl" }
		});
		reflectiveShader->SetDebugName("Reflective");

		// This shader handles our basic materials without reflections (cause they expensive)
		ShaderProgram::Sptr basicShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_blinn_phong_textured.glsl" }
		});
		basicShader->SetDebugName("Blinn-phong");

		ShaderProgram::Sptr invaderShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/invader_vert.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/invader_frag.glsl" }
		});
		invaderShader->SetDebugName("Invader Shader");

		// This shader handles our basic materials without reflections (cause they expensive)
		ShaderProgram::Sptr specShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/textured_specular.glsl" }
		});
		specShader->SetDebugName("Textured-Specular");

		// This shader handles our foliage vertex shader example
		ShaderProgram::Sptr foliageShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/foliage.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/screendoor_transparency.glsl" }
		});
		foliageShader->SetDebugName("Foliage");

		// This shader handles our cel shading example
		ShaderProgram::Sptr toonShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/toon_shading.glsl" }
		});
		toonShader->SetDebugName("Toon Shader");

		// This shader handles our displacement mapping example
		ShaderProgram::Sptr displacementShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/displacement_mapping.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_tangentspace_normal_maps.glsl" }
		}); 
		displacementShader->SetDebugName("Displacement Mapping");  

		// This shader handles our tangent space normal mapping
		ShaderProgram::Sptr tangentSpaceMapping = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_tangentspace_normal_maps.glsl" }
		});
		tangentSpaceMapping->SetDebugName("Tangent Space Mapping");

		// This shader handles our multitexturing example
		ShaderProgram::Sptr multiTextureShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/vert_multitextured.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_multitextured.glsl" }
		});
		multiTextureShader->SetDebugName("Multitexturing");

		//Used for Assignment 1
		ShaderProgram::Sptr assignmentShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string> {
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/assignment_vert.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/assignment_frag.glsl" }
		});

		// Load in the meshes
		MeshResource::Sptr monkeyMesh = ResourceManager::CreateAsset<MeshResource>("Monkey.obj");
		MeshResource::Sptr invaderMesh = ResourceManager::CreateAsset<MeshResource>("SpaceInvaderModel.obj");

		// Load in some textures
		Texture2D::Sptr    boxTexture = ResourceManager::CreateAsset<Texture2D>("textures/box-diffuse.png");
		Texture2D::Sptr    boxSpec = ResourceManager::CreateAsset<Texture2D>("textures/box-specular.png");
		Texture2D::Sptr    monkeyTex = ResourceManager::CreateAsset<Texture2D>("textures/monkey-uvMap.png");
		Texture2D::Sptr    leafTex = ResourceManager::CreateAsset<Texture2D>("textures/leaves.png");
		Texture2D::Sptr    kingMonkTex = ResourceManager::CreateAsset<Texture2D>("textures/kingMonkeyTexture.png");
		Texture2D::Sptr    pillarTex = ResourceManager::CreateAsset<Texture2D>("textures/pillarTexture.png");
		Texture2D::Sptr    fireTex = ResourceManager::CreateAsset<Texture2D>("textures/fireTexture.png");
		
		leafTex->SetMinFilter(MinFilter::Nearest);
		leafTex->SetMagFilter(MagFilter::Nearest);


		// Loading in a 1D LUT
		Texture1D::Sptr toonLut = ResourceManager::CreateAsset<Texture1D>("luts/toon-1D.png");
		Texture1D::Sptr chillyLut = ResourceManager::CreateAsset<Texture1D>("luts/chilly-1D.png");
		toonLut->SetWrap(WrapMode::ClampToEdge);

		// Here we'll load in the cubemap, as well as a special shader to handle drawing the skybox
		TextureCube::Sptr testCubemap = ResourceManager::CreateAsset<TextureCube>("cubemaps/ocean/ocean.jpg");
		ShaderProgram::Sptr      skyboxShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/skybox_vert.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/skybox_frag.glsl" } 
		});

		// Create an empty scene
		Scene::Sptr scene = std::make_shared<Scene>();

		// Setting up our enviroment map
		//scene->SetSkyboxTexture(testCubemap);
		//scene->SetSkyboxShader(skyboxShader);
		// Since the skybox I used was for Y-up, we need to rotate it 90 deg around the X-axis to convert it to z-up 
		//scene->SetSkyboxRotation(glm::rotate(MAT4_IDENTITY, glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f)));
		

		// Loading in a color lookup table
		Texture3D::Sptr lut = ResourceManager::CreateAsset<Texture3D>("luts/cool.CUBE");

		// Configure the color correction LUT
		scene->SetColorLUT(lut);

		// Create our materials
		// This will be our box material, with no environment reflections
		Material::Sptr boxMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			boxMaterial->Name = "Box";
			boxMaterial->Set("u_Material.Diffuse", boxTexture);
			boxMaterial->Set("u_Material.Shininess", 0.1f);
		}

		// This will be the reflective material, we'll make the whole thing 90% reflective
		Material::Sptr monkeyMaterial = ResourceManager::CreateAsset<Material>(reflectiveShader);
		{
			monkeyMaterial->Name = "Monkey";
			monkeyMaterial->Set("u_Material.Diffuse", monkeyTex);
			monkeyMaterial->Set("u_Material.Shininess", 0.5f);
		}

		// This will be the reflective material, we'll make the whole thing 90% reflective
		Material::Sptr testMaterial = ResourceManager::CreateAsset<Material>(specShader);
		{
			testMaterial->Name = "Box-Specular";
			testMaterial->Set("u_Material.Diffuse", boxTexture);
			testMaterial->Set("u_Material.Specular", boxSpec);
		}

		// Our foliage vertex shader material
		Material::Sptr foliageMaterial = ResourceManager::CreateAsset<Material>(foliageShader);
		{
			foliageMaterial->Name = "Foliage Shader";
			foliageMaterial->Set("u_Material.Diffuse", leafTex);
			foliageMaterial->Set("u_Material.Shininess", 0.1f);
			foliageMaterial->Set("u_Material.Threshold", 0.1f);

			foliageMaterial->Set("u_WindDirection", glm::vec3(1.0f, 1.0f, 0.0f));
			foliageMaterial->Set("u_WindStrength", 0.5f);
			foliageMaterial->Set("u_VerticalScale", 1.0f);
			foliageMaterial->Set("u_WindSpeed", 1.0f);
		}

		// Our toon shader material
		Material::Sptr toonMaterial = ResourceManager::CreateAsset<Material>(toonShader);
		{
			toonMaterial->Name = "Toon";
			toonMaterial->Set("u_Material.Diffuse", boxTexture);
			toonMaterial->Set("s_ToonTerm", toonLut);
			toonMaterial->Set("u_Material.Shininess", 0.1f);
			toonMaterial->Set("u_Material.Steps", 8);
		}


		Material::Sptr displacementTest = ResourceManager::CreateAsset<Material>(displacementShader);
		{
			Texture2D::Sptr displacementMap = ResourceManager::CreateAsset<Texture2D>("textures/displacement_map.png");
			Texture2D::Sptr normalMap = ResourceManager::CreateAsset<Texture2D>("textures/normal_map.png");
			Texture2D::Sptr diffuseMap = ResourceManager::CreateAsset<Texture2D>("textures/bricks_diffuse.png");

			displacementTest->Name = "Displacement Map";
			displacementTest->Set("u_Material.Diffuse", diffuseMap);
			displacementTest->Set("s_Heightmap", displacementMap);
			displacementTest->Set("s_NormalMap", normalMap);
			displacementTest->Set("u_Material.Shininess", 0.5f);
			displacementTest->Set("u_Scale", 0.1f);
		}

		Material::Sptr normalmapMat = ResourceManager::CreateAsset<Material>(tangentSpaceMapping);
		{
			Texture2D::Sptr normalMap = ResourceManager::CreateAsset<Texture2D>("textures/normal_map.png");
			Texture2D::Sptr diffuseMap = ResourceManager::CreateAsset<Texture2D>("textures/bricks_diffuse.png");

			normalmapMat->Name = "Tangent Space Normal Map";
			normalmapMat->Set("u_Material.Diffuse", diffuseMap);
			normalmapMat->Set("s_NormalMap", normalMap);
			normalmapMat->Set("u_Material.Shininess", 0.5f);
			normalmapMat->Set("u_Scale", 0.1f);
		}

		Material::Sptr multiTextureMat = ResourceManager::CreateAsset<Material>(multiTextureShader);
		{
			Texture2D::Sptr sand = ResourceManager::CreateAsset<Texture2D>("textures/terrain/sand.png");
			Texture2D::Sptr grass = ResourceManager::CreateAsset<Texture2D>("textures/terrain/grass.png");

			multiTextureMat->Name = "Multitexturing";
			multiTextureMat->Set("u_Material.DiffuseA", sand);
			multiTextureMat->Set("u_Material.DiffuseB", grass);
			multiTextureMat->Set("u_Material.Shininess", 0.5f);
			multiTextureMat->Set("u_Scale", 0.1f);
		}

		Material::Sptr kingMonkeyMat = ResourceManager::CreateAsset<Material>(basicShader);
		{
			kingMonkeyMat->Name = "KingMonkeyMat";
			kingMonkeyMat->Set("u_Material.Diffuse", kingMonkTex);
			kingMonkeyMat->Set("u_Material.Shininess", 0.5f);
		}

		Material::Sptr pillarMat = ResourceManager::CreateAsset<Material>(basicShader);
		{
			pillarMat->Name = "PillarMat";
			pillarMat->Set("u_Material.Diffuse", pillarTex);
			pillarMat->Set("u_Material.Shininess", 0.5f);
		}

		Material::Sptr fireMat = ResourceManager::CreateAsset<Material>(basicShader);
		{
			fireMat->Name = "FireMat";
			fireMat->Set("u_Material.Diffuse", fireTex); 
			fireMat->Set("u_Material.Shininess", 0.5f);
		}

		Material::Sptr invaderMat = ResourceManager::CreateAsset<Material>(invaderShader);
		{
			Texture2D::Sptr    invaderBlue = ResourceManager::CreateAsset<Texture2D>("textures/InvaderBlueTexture.png");
			Texture2D::Sptr    invaderOrange = ResourceManager::CreateAsset<Texture2D>("textures/InvaderOrangeTexture.png");
			Texture2D::Sptr    invaderPink = ResourceManager::CreateAsset<Texture2D>("textures/InvaderPinkTexture.png");
			Texture2D::Sptr    invaderPurple = ResourceManager::CreateAsset<Texture2D>("textures/InvaderPurpleTexture.png");
			Texture2D::Sptr    invaderRed = ResourceManager::CreateAsset<Texture2D>("textures/InvaderRedTexture.png");
			Texture2D::Sptr    invaderYellow = ResourceManager::CreateAsset<Texture2D>("textures/InvaderYellowTexture.png");
			invaderMat->Name = "InvaderMat";
			invaderMat->Set("u_Material.Diffuse", invaderBlue);
			invaderMat->Set("u_Material.Shininess", 0.5f);

			invaderMat->Set("u_InvaderMaterial.blueDiffuse", invaderBlue);
			invaderMat->Set("u_InvaderMaterial.redDiffuse", invaderRed);
			invaderMat->Set("u_InvaderMaterial.pinkDiffuse", invaderPink);
			invaderMat->Set("u_InvaderMaterial.pinkDiffuse", invaderPink);
			invaderMat->Set("u_InvaderMaterial.purpleDiffuse", invaderPurple);
			invaderMat->Set("u_InvaderMaterial.yellowDiffuse", invaderYellow);
			invaderMat->Set("u_InvaderMaterial.orangeDiffuse", invaderOrange);
		}   
		 
		Texture2D::Sptr specularTexture = ResourceManager::CreateAsset<Texture2D>("textures/assignment_specular.png");
		Material::Sptr assignmentMat = ResourceManager::CreateAsset<Material>(assignmentShader);
		{   
			assignmentMat->Name = "AssignmentMat";
			assignmentMat->Set("u_Material.Diffuse", kingMonkTex); 
			assignmentMat->Set("u_Material.Specular", specularTexture);
			assignmentMat->Set("u_Material.Shininess", 0.5f);
			assignmentMat->Set("s_Chilly", chillyLut);
		} 

		Material::Sptr assignmentMatPillar = ResourceManager::CreateAsset<Material>(assignmentShader);
		{
			assignmentMatPillar->Name = "AssignmentMatPillar";
			assignmentMatPillar->Set("u_Material.Diffuse", pillarTex);
			assignmentMatPillar->Set("u_Material.Specular", specularTexture);
			assignmentMatPillar->Set("u_Material.Shininess", 0.5f);
			assignmentMatPillar->Set("s_Chilly", chillyLut);
		}

		Material::Sptr assignmentMatFire = ResourceManager::CreateAsset<Material>(assignmentShader);
		{
			assignmentMatFire->Name = "AssignmentMatFire";
			assignmentMatFire->Set("u_Material.Diffuse", fireTex);
			assignmentMatFire->Set("u_Material.Specular", specularTexture);
			assignmentMatFire->Set("u_Material.Shininess", 0.5f);
			assignmentMatFire->Set("s_Chilly", chillyLut);
		}

		// Create some lights for our scene
		scene->Lights.resize(1);
		scene->Lights[0].Position = glm::vec3(0.0f, 1.0f, 3.0f);
		scene->Lights[0].Color = glm::vec3(1.0f, 1.0f, 1.0f);
		scene->Lights[0].Range = 100.0f;

		/*scene->Lights[1].Position = glm::vec3(1.0f, 0.0f, 3.0f);
		scene->Lights[1].Color = glm::vec3(0.2f, 0.8f, 0.1f);

		scene->Lights[2].Position = glm::vec3(0.0f, 1.0f, 3.0f);
		scene->Lights[2].Color = glm::vec3(1.0f, 0.2f, 0.1f);*/

		// We'll create a mesh that is a simple plane that we can resize later
		MeshResource::Sptr planeMesh = ResourceManager::CreateAsset<MeshResource>();
		planeMesh->AddParam(MeshBuilderParam::CreatePlane(ZERO, UNIT_Z, UNIT_X, glm::vec2(1.0f)));
		planeMesh->GenerateMesh();

		MeshResource::Sptr sphere = ResourceManager::CreateAsset<MeshResource>();
		sphere->AddParam(MeshBuilderParam::CreateIcoSphere(ZERO, ONE, 5));
		sphere->GenerateMesh();

		// Set up the scene's camera
		GameObject::Sptr camera = scene->MainCamera->GetGameObject()->SelfRef();
		{
			camera->SetPostion({ -3.810f, 0.09f, 6.250f });
			camera->LookAt(glm::vec3(1.5f, 0.0f, 4.0f));

			camera->Add<SimpleCameraControl>();

			// This is now handled by scene itself!
			//Camera::Sptr cam = camera->Add<Camera>();
			// Make sure that the camera is set as the scene's main camera!
			//scene->MainCamera = cam;
		}

		// Set up all our sample objects
		GameObject::Sptr plane = scene->CreateGameObject("Plane");
		{
			// Make a big tiled mesh
			MeshResource::Sptr tiledMesh = ResourceManager::CreateAsset<MeshResource>();
			tiledMesh->AddParam(MeshBuilderParam::CreatePlane(ZERO, UNIT_Z, UNIT_X, glm::vec2(100.0f), glm::vec2(20.0f)));
			tiledMesh->GenerateMesh();

			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = plane->Add<RenderComponent>();
			renderer->SetMesh(tiledMesh);
			renderer->SetMaterial(boxMaterial);

			// Attach a plane collider that extends infinitely along the X/Y axis
			RigidBody::Sptr physics = plane->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(BoxCollider::Create(glm::vec3(50.0f, 50.0f, 1.0f)))->SetPosition({ 0,0,-1 });
		}			

		GameObject::Sptr demoBase = scene->CreateGameObject("Demo Parent");

		GameObject::Sptr showFloor = scene->CreateGameObject("Show Floor");

		GameObject::Sptr kingMonkey = scene->CreateGameObject("King Monkey");
		{
			// Set position in the scene 
			kingMonkey->SetPostion(glm::vec3(1.5f, 0.0f, 4.0f)); 

			// Add some behaviour that relies on the physics body
			kingMonkey->Add<JumpBehaviour>();  
			kingMonkey->Add<TestComponent>();
			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer = kingMonkey->Add<RenderComponent>();
			renderer->SetMesh(monkeyMesh);
			renderer->SetMaterial(assignmentMat); 
			showFloor->AddChild(kingMonkey);

			RotatingBehaviour::Sptr behaviour = kingMonkey->Add<RotatingBehaviour>();
			behaviour->RotationSpeed = glm::vec3(90.0f, 0.0f, 0.0f);
		}

		GameObject::Sptr leftPillar = scene->CreateGameObject("Left Pillar");
		{
			MeshResource::Sptr pillarMeshL = ResourceManager::CreateAsset<MeshResource>();
			pillarMeshL->AddParam(MeshBuilderParam::CreateCube(ZERO, ONE));
			pillarMeshL->GenerateMesh();

			// Set and rotation position in the scene
			leftPillar->SetPostion(glm::vec3(1.5f, 2.7f, 4.0f));

			leftPillar->Add<TestComponent>();

			// Add a render component
			RenderComponent::Sptr renderer = leftPillar->Add<RenderComponent>();
			renderer->SetMesh(pillarMeshL);
			renderer->SetMaterial(assignmentMatPillar);
			showFloor->AddChild(leftPillar);

			RotatingBehaviour::Sptr behaviour = leftPillar->Add<RotatingBehaviour>();
			behaviour->RotationSpeed = glm::vec3(0.0f, 0.0f, 90.0f);
		}

		GameObject::Sptr rightPillar = scene->CreateGameObject("Right Pillar");
		{
			MeshResource::Sptr pillarMeshR = ResourceManager::CreateAsset<MeshResource>();
			pillarMeshR->AddParam(MeshBuilderParam::CreateCube(ZERO, ONE));
			pillarMeshR->GenerateMesh();

			// Set and rotation position in the scene
			rightPillar->SetPostion(glm::vec3(1.5f, -2.7f, 4.0f));

			rightPillar->Add<TestComponent>();

			// Add a render component
			RenderComponent::Sptr renderer = rightPillar->Add<RenderComponent>();
			renderer->SetMesh(pillarMeshR);
			renderer->SetMaterial(assignmentMatPillar);
			showFloor->AddChild(rightPillar);

			RotatingBehaviour::Sptr behaviour = rightPillar->Add<RotatingBehaviour>();
			behaviour->RotationSpeed = glm::vec3(0.0f, 0.0f, 90.0f);
		}

		GameObject::Sptr leftFlame = scene->CreateGameObject("Left Flame");
		{
			// Set and rotation position in the scene
			leftFlame->SetPostion(glm::vec3(1.5f, 2.7f, 5.0f));
			leftFlame->SetScale(glm::vec3(0.4f));

			leftFlame->Add<TestComponent>();

			// Add a render component
			RenderComponent::Sptr renderer = leftFlame->Add<RenderComponent>();
			renderer->SetMesh(sphere);
			renderer->SetMaterial(assignmentMatFire);
			showFloor->AddChild(leftFlame);

			RotatingBehaviour::Sptr behaviour = leftFlame->Add<RotatingBehaviour>();
			behaviour->RotationSpeed = glm::vec3(0.0f, 0.0f, -90.0f);
		} 

		GameObject::Sptr rightFlame = scene->CreateGameObject("Right Flame");
		{ 
			// Set and rotation position in the scene
			rightFlame->SetPostion(glm::vec3(1.5f, -2.7f, 5.0f));
			rightFlame->SetScale(glm::vec3(0.4f));
			 
			rightFlame->Add<TestComponent>();

			// Add a render component
			RenderComponent::Sptr renderer = rightFlame->Add<RenderComponent>();
			renderer->SetMesh(sphere);
			renderer->SetMaterial(assignmentMatFire);
			showFloor->AddChild(rightFlame);

			RotatingBehaviour::Sptr behaviour = rightFlame->Add<RotatingBehaviour>();
			behaviour->RotationSpeed = glm::vec3(0.0f, 0.0f, -90.0f);
		}

		// Create a trigger volume for testing how we can detect collisions with objects!
		GameObject::Sptr trigger = scene->CreateGameObject("Trigger");
		{
			TriggerVolume::Sptr volume = trigger->Add<TriggerVolume>();
			CylinderCollider::Sptr collider = CylinderCollider::Create(glm::vec3(3.0f, 3.0f, 1.0f));
			collider->SetPosition(glm::vec3(0.0f, 0.0f, 0.5f));
			volume->AddCollider(collider);

			trigger->Add<TriggerVolumeEnterBehaviour>();
		}

		/////////////////////////// UI //////////////////////////////
		/*
		GameObject::Sptr canvas = scene->CreateGameObject("UI Canvas");
		{
			RectTransform::Sptr transform = canvas->Add<RectTransform>();
			transform->SetMin({ 16, 16 });
			transform->SetMax({ 256, 256 });

			GuiPanel::Sptr canPanel = canvas->Add<GuiPanel>();


			GameObject::Sptr subPanel = scene->CreateGameObject("Sub Item");
			{
				RectTransform::Sptr transform = subPanel->Add<RectTransform>();
				transform->SetMin({ 10, 10 });
				transform->SetMax({ 128, 128 });

				GuiPanel::Sptr panel = subPanel->Add<GuiPanel>();
				panel->SetColor(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

				panel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/upArrow.png"));

				Font::Sptr font = ResourceManager::CreateAsset<Font>("fonts/Roboto-Medium.ttf", 16.0f);
				font->Bake();

				GuiText::Sptr text = subPanel->Add<GuiText>();
				text->SetText("Hello world!");
				text->SetFont(font);

				monkey1->Get<JumpBehaviour>()->Panel = text;
			}

			canvas->AddChild(subPanel);
		}
		*/

		/*GameObject::Sptr particles = scene->CreateGameObject("Particles");
		{
			ParticleSystem::Sptr particleManager = particles->Add<ParticleSystem>();  
			particleManager->AddEmitter(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 10.0f), 10.0f, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f)); 
		}*/

		GuiBatcher::SetDefaultTexture(ResourceManager::CreateAsset<Texture2D>("textures/ui-sprite.png"));
		GuiBatcher::SetDefaultBorderRadius(8);

		// Save the asset manifest for all the resources we just loaded
		ResourceManager::SaveManifest("scene-manifest.json");
		// Save the scene to a JSON file
		scene->Save("scene.json");

		// Send the scene to the application
		app.LoadScene(scene);
	}
}

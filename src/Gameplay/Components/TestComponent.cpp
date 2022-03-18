#include "Gameplay/Components/TestComponent.h"
#include <GLFW/glfw3.h>
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"
#include "Utils/ImGuiHelper.h" 
#include "Gameplay/InputEngine.h"
#include "Gameplay/Light.h" 

void TestComponent::Awake() 
{
	_renderer = GetComponent<RenderComponent>();
	std::cout << "TEST COMPONENT WORKING\N";
}

void TestComponent::RenderImGui() {
	LABEL_LEFT(ImGui::DragFloat, "Impulse", &test, 13.6f);
}

nlohmann::json TestComponent::ToJson() const {
	return {
		{ "test", test }
	};
}

TestComponent::TestComponent() :
	IComponent(),
	_renderer(nullptr),
	test(10.0f)
{ }

TestComponent::~TestComponent() = default;

TestComponent::Sptr TestComponent::FromJson(const nlohmann::json& blob) {
	TestComponent::Sptr result = std::make_shared<TestComponent>();
	result->test = blob["test"];
	return result;
}

void TestComponent::Update(float deltaTime)
{
	
	if (InputEngine::GetKeyState(GLFW_KEY_1) == ButtonState::Pressed) //lighting on/off
	{
		_renderer->GetMaterial()->Set("u_Material.choice", 1);
		ambientOnly = false;
		if (lights == true)
		{
			for (int i = 0; i < GetGameObject()->GetScene()->Lights.size(); i++)
			{
				GetGameObject()->GetScene()->Lights[i].Range = 0.f;
			}
			GetGameObject()->GetScene()->SetAmbientLight(glm::vec3(0.f));
			lights = false;
		}
		else
		{
			for (int i = 0; i < GetGameObject()->GetScene()->Lights.size(); i++)
			{
				GetGameObject()->GetScene()->Lights[i].Range = 100.f;				
			}
			GetGameObject()->GetScene()->SetAmbientLight(glm::vec3(0.5f));
			lights = true;
		}
		
		GetGameObject()->GetScene()->SetupShaderAndLights();
	}

	if (InputEngine::GetKeyState(GLFW_KEY_2) == ButtonState::Pressed) //ambient lighting only
	{
		_renderer->GetMaterial()->Set("u_Material.choice", 2);
		if (ambientOnly == false)
		{
			for (int i = 0; i < GetGameObject()->GetScene()->Lights.size(); i++)
			{
				GetGameObject()->GetScene()->Lights[i].Range = 0.f;				
			}
			GetGameObject()->GetScene()->SetAmbientLight(glm::vec3(1.0f));
			ambientOnly = true;
		}
		else
		{
			for (int i = 0; i < GetGameObject()->GetScene()->Lights.size(); i++)
			{
				GetGameObject()->GetScene()->Lights[i].Range = 100.f;
			}
			GetGameObject()->GetScene()->SetAmbientLight(glm::vec3(0.5f));
			ambientOnly = false;
		} 
		
	}
	if (InputEngine::GetKeyState(GLFW_KEY_3) == ButtonState::Pressed) //specular lighting only
	{
		_renderer->GetMaterial()->Set("u_Material.choice", 3);
		for (int i = 0; i < GetGameObject()->GetScene()->Lights.size(); i++)
		{
			GetGameObject()->GetScene()->Lights[i].Range = 100.f;
		}
		GetGameObject()->GetScene()->SetAmbientLight(glm::vec3(0.0f));
		GetGameObject()->GetScene()->SetupShaderAndLights();
	}
	if (InputEngine::GetKeyState(GLFW_KEY_4) == ButtonState::Pressed) //ambient + specular
	{
		_renderer->GetMaterial()->Set("u_Material.choice", 4);
		GetGameObject()->GetScene()->SetAmbientLight(glm::vec3(0.5f));
	}
	if (InputEngine::GetKeyState(GLFW_KEY_5) == ButtonState::Pressed) //ambient + specular + custom shader effect
	{
		if (customShader == false)
		{
			_renderer->GetMaterial()->Set("u_Material.choice", 5);
			customShader = true;
		}
		else
		{
			_renderer->GetMaterial()->Set("u_Material.choice", 10);
			customShader = false;
		}
		
	}
	if (InputEngine::GetKeyState(GLFW_KEY_6) == ButtonState::Pressed) //diffuse warp/ramp
	{
		if (diffuseRamp == false)
		{
			_renderer->GetMaterial()->Set("u_Material.choice", 6);
			diffuseRamp = true;
		}
		else
		{
			_renderer->GetMaterial()->Set("u_Material.choice", 10);
			diffuseRamp = false;
		}
	}
	if (InputEngine::GetKeyState(GLFW_KEY_7) == ButtonState::Pressed)
	{
		if (specularRamp == false)
		{
			_renderer->GetMaterial()->Set("u_Material.choice", 7);
			specularRamp = true;
		}
		else
		{
			_renderer->GetMaterial()->Set("u_Material.choice", 10);
			specularRamp = false;
		}
	}
	if (InputEngine::GetKeyState(GLFW_KEY_8) == ButtonState::Pressed) //warm lut
	{
		GetGameObject()->GetScene()->SetColorLUT(warmLut);
		
	}
	if (InputEngine::GetKeyState(GLFW_KEY_9) == ButtonState::Pressed) //cool lut
	{
		GetGameObject()->GetScene()->SetColorLUT(coolLut);
	}
	if (InputEngine::GetKeyState(GLFW_KEY_0) == ButtonState::Pressed) //custom lut
	{
		GetGameObject()->GetScene()->SetColorLUT(trippyLut);
	}
}


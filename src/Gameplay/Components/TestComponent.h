#pragma once
#include "IComponent.h"
#include "Gameplay/Physics/RigidBody.h"
#include "Gameplay/Components/RenderComponent.h"
#include "Graphics/Textures/Texture3D.h"

class TestComponent : public Gameplay::IComponent {
public:
	typedef std::shared_ptr<TestComponent> Sptr;

	std::weak_ptr<Gameplay::IComponent> Panel;

	TestComponent();
	virtual ~TestComponent();

	virtual void Awake() override;
	virtual void Update(float deltaTime) override;

public:
	virtual void RenderImGui() override;
	MAKE_TYPENAME(TestComponent);
	virtual nlohmann::json ToJson() const override;
	static TestComponent::Sptr FromJson(const nlohmann::json& blob);
	
protected:
	float test;
	int lives = 6;
	bool lights = true;
	bool ambientOnly = false;
	bool diffuseRamp = false;
	bool specularRamp = false;
	bool customShader = false;
	RenderComponent::Sptr _renderer;
	Texture3D::Sptr coolLut = ResourceManager::CreateAsset<Texture3D>("luts/supercool.CUBE");
	Texture3D::Sptr warmLut = ResourceManager::CreateAsset<Texture3D>("luts/warm.CUBE");
	Texture3D::Sptr trippyLut = ResourceManager::CreateAsset<Texture3D>("luts/trippy.CUBE");
};

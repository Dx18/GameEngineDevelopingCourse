#include <Camera.h>
#include <DefaultGeometry.h>
#include <Game.h>
#include <GameObject.h>
#include <Input/InputHandler.h>

#include <random>

namespace GameEngine
{
	constexpr float GRAVITY = 9.8;

	constexpr float PENDULUM_OBJECT_VELOCITY = 2.0;

	constexpr float CONTROLLABLE_OBJECT_VELOCITY = 2.0;

	Game::Game(
		std::function<bool()> PlatformLoopFunc
	) :
		PlatformLoop(PlatformLoopFunc)
	{
		Core::g_MainCamera = new Core::Camera();
		Core::g_MainCamera->SetPosition(Math::Vector3f(0.0f, 6.0f, -6.0f));
		Core::g_MainCamera->SetViewDir(Math::Vector3f(0.0f, -6.0f, 6.0f).Normalized());

		m_renderThread = std::make_unique<Render::RenderThread>();

		std::mt19937 random;

		static constexpr std::size_t OBJECT_COUNT = 100;

		std::size_t remainingObjectCount = OBJECT_COUNT;

		std::size_t physicalObjectCount = std::uniform_int_distribution<std::size_t>(0, remainingObjectCount)(random);
		remainingObjectCount -= physicalObjectCount;

		std::size_t pendulumObjectCount = std::uniform_int_distribution<std::size_t>(0, remainingObjectCount)(random);
		remainingObjectCount -= pendulumObjectCount;

		std::size_t controllableObjectCount = remainingObjectCount;

		std::uniform_real_distribution<float> genHorizontalCoordinate(-10.0, 10.0);
		std::uniform_real_distribution<float> genVerticalCoordinate(1.0, 15.0);

		m_Objects.resize(OBJECT_COUNT);
		for (std::size_t i = 0; i < OBJECT_COUNT; ++i)
		{
			m_Objects[i] = new GameObject();

			m_Objects[i]->SetPosition(Math::Vector3f(
				genHorizontalCoordinate(random), genVerticalCoordinate(random), genHorizontalCoordinate(random)
			), 0);

			Render::RenderObject** renderObject = m_Objects[i]->GetRenderObjectRef();
			m_renderThread->EnqueueCommand(Render::ERC::CreateRenderObject, RenderCore::DefaultGeometry::Cube(), renderObject);
		}

		std::size_t offset = 0;

		m_physicalObjects.resize(
			physicalObjectCount, PhysicalObjectState
			{
				.verticalVelocity = 0.0,
			});
		offset += physicalObjectCount;

		m_pendulumObjects.resize(pendulumObjectCount);
		for (std::size_t i = 0; i < pendulumObjectCount; ++i)
		{
			m_pendulumObjects[i] =
			{
				.zMin = m_Objects[offset + i]->GetPosition().z - 2.0f,
				.zMax = m_Objects[offset + i]->GetPosition().z + 2.0f,
				.isMovingTowardsMax = true,
			};
		}

		m_controllableObjectsState =
		{
			.moveDirection = 0,
		};

		Core::g_InputHandler->RegisterCallback("GoForward", [&]() { Core::g_MainCamera->Move(Core::g_MainCamera->GetViewDir()); });
		Core::g_InputHandler->RegisterCallback("GoBack", [&]() { Core::g_MainCamera->Move(-Core::g_MainCamera->GetViewDir()); });
		Core::g_InputHandler->RegisterCallback("GoRight", [&]() { Core::g_MainCamera->Move(Core::g_MainCamera->GetRightDir()); });
		Core::g_InputHandler->RegisterCallback("GoLeft", [&]() { Core::g_MainCamera->Move(-Core::g_MainCamera->GetRightDir()); });
		Core::g_InputHandler->RegisterCallback("ObjectGoRight", [&]() { m_controllableObjectsState.moveDirection = 1; });
		Core::g_InputHandler->RegisterCallback("ObjectGoLeft", [&]() { m_controllableObjectsState.moveDirection = -1; });
	}

	void Game::Run()
	{
		assert(PlatformLoop != nullptr);

		m_GameTimer.Reset();

		bool quit = false;
		while (!quit)
		{
			m_GameTimer.Tick();
			float dt = m_GameTimer.GetDeltaTime();

			Core::g_MainWindowsApplication->Update();
			Core::g_InputHandler->Update();
			Core::g_MainCamera->Update(dt);

			Update(dt);

			m_renderThread->OnEndFrame();

			// The most common idea for such a loop is that it returns false when quit is required, or true otherwise
			quit = !PlatformLoop();
		}
	}

	void Game::Update(float dt)
	{
		UpdatePhysicalObjects(dt);
		UpdatePendulumObjects(dt);
		UpdateControllableObjects(dt);
	}

	std::span<GameObject*> Game::GetPhysicalObjectsSpan()
	{
		std::vector<GameObject*>::iterator begin = m_Objects.begin();
		return std::span(begin, begin + m_physicalObjects.size());
	}

	std::span<GameObject*> Game::GetPendulumObjectsSpan()
	{
		std::vector<GameObject*>::iterator begin = m_Objects.begin() + m_physicalObjects.size();
		return std::span(begin, begin + m_pendulumObjects.size());
	}

	std::span<GameObject*> Game::GetControllableObjectsSpan()
	{
		std::vector<GameObject*>::iterator begin = m_Objects.begin() + m_physicalObjects.size() + m_pendulumObjects.size();
		return std::span(begin, m_Objects.end());
	}

	void Game::UpdatePhysicalObjects(float dt)
	{
		std::span<GameObject*> objects = GetPhysicalObjectsSpan();

		for (std::size_t i = 0; i < m_physicalObjects.size(); ++i)
		{
			PhysicalObjectState& state = m_physicalObjects[i];

			state.verticalVelocity += GRAVITY * dt;

			Math::Vector3f position = objects[i]->GetPosition();
			position.y = std::max(0.0f, position.y - state.verticalVelocity * dt);

			if (position.y == 0.0)
			{
				state.verticalVelocity *= -1.0;
			}

			objects[i]->SetPosition(position, m_renderThread->GetMainFrame());
		}
	}

	void Game::UpdatePendulumObjects(float dt)
	{
		std::span<GameObject*> objects = GetPendulumObjectsSpan();

		for (std::size_t i = 0; i < m_pendulumObjects.size(); ++i)
		{
			PendulumObjectState& state = m_pendulumObjects[i];

			Math::Vector3f position = objects[i]->GetPosition();

			position.z += (state.isMovingTowardsMax ? 1.0 : -1.0) * PENDULUM_OBJECT_VELOCITY * dt;
			if (state.isMovingTowardsMax && position.z >= state.zMax)
			{
				position.z = state.zMax;
				state.isMovingTowardsMax = false;
			}
			else if (!state.isMovingTowardsMax && position.z <= state.zMin)
			{
				position.z = state.zMin;
				state.isMovingTowardsMax = true;
			}

			objects[i]->SetPosition(position, m_renderThread->GetMainFrame());
		}
	}

	void Game::UpdateControllableObjects(float dt)
	{
		ControllableObjectsState& state = m_controllableObjectsState;

		std::span<GameObject*> objects = GetControllableObjectsSpan();

		for (std::size_t i = 0; i < m_pendulumObjects.size(); ++i)
		{
			Math::Vector3f position = objects[i]->GetPosition();

			position.x += state.moveDirection * CONTROLLABLE_OBJECT_VELOCITY * dt;

			objects[i]->SetPosition(position, m_renderThread->GetMainFrame());
		}

		state.moveDirection = 0;
	}
}
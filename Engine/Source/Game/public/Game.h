#pragma once

#include <RenderEngine.h>
#include <RenderThread.h>
#include <Timer.h>
#include <Window/IWindow.h>

#include <span>

namespace GameEngine
{
	class GameObject;

	class Game final
	{
	public:
		Game() = delete;
		Game(
			std::function<bool()> PlatformLoopFunc
		);

	public:
		void Run();
		void Update(float dt);

	private:
		// The main idea behind having this functor is to abstract the common code from the platfrom-specific code
		std::function<bool()> PlatformLoop = nullptr;

	private:
		struct PhysicalObjectState
		{
			float verticalVelocity;
		};

		struct PendulumObjectState
		{
			float zMin;
			float zMax;
			bool isMovingTowardsMax;
		};

		struct ControllableObjectsState
		{
			int moveDirection;
		};

		Core::Timer m_GameTimer;
		std::unique_ptr<Render::RenderThread> m_renderThread;
		std::vector<GameObject*> m_Objects;

		std::vector<PhysicalObjectState> m_physicalObjects;
		std::vector<PendulumObjectState> m_pendulumObjects;

		ControllableObjectsState m_controllableObjectsState;

		std::span<GameObject*> GetPhysicalObjectsSpan();
		std::span<GameObject*> GetPendulumObjectsSpan();
		std::span<GameObject*> GetControllableObjectsSpan();

		void UpdatePhysicalObjects(float dt);
		void UpdatePendulumObjects(float dt);
		void UpdateControllableObjects(float dt);
	};
}
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include <Window/IWindow.h>
#include <Game.h>
#include <array.h>
#include <iostream>
#include <filesystem>

#include <ini.h>

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

bool WindowsMessageLoop()
{
	MSG msg = { 0 };

	while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);

		if (msg.message == (WM_QUIT | WM_CLOSE))
		{
			return false;
		}
	}

	return msg.message != (WM_QUIT | WM_CLOSE);
}

static int GameConfigParseHandler(void* user, const char* section, const char* name, const char* value)
{
	GameEngine::GameConfig* config = reinterpret_cast<GameEngine::GameConfig*>(user);

	if (std::strcmp(section, "control") == 0)
	{
		if (std::strcmp(name, "camera_speed") == 0)
		{
			config->cameraSpeed = std::stof(value);
		}
	}

	return 1;
}

std::optional<GameEngine::GameConfig> ReadGameConfig(const std::filesystem::path &path)
{
	GameEngine::GameConfig result;

	if (ini_parse(path.string().c_str(), GameConfigParseHandler, &result) < 0) {
		return std::nullopt;
	}

	return result;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// Enable run-time memory check for debug builds.
#if defined(DEBUG) || defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	static const std::filesystem::path DEFAULT_CONFIG_PATH = "../../../../../Assets/config.ini";

	// It's useless, since (perhaps) shaders are loaded from relative path anyway, but I'll leave it here
	std::filesystem::path configPath = DEFAULT_CONFIG_PATH;
	const char* configPathEnv = std::getenv("GAME_CONFIG_PATH");
	if (configPathEnv != nullptr && std::strlen(configPathEnv) > 0)
	{
		configPath = configPathEnv;
	}

	std::optional<GameEngine::GameConfig> config = ReadGameConfig(configPath);

	if (!config.has_value())
	{
		std::cerr << "Could not open config file at \"" << configPath.string() << "\"" << std::endl;
		std::cerr << "Current working directory: \"" << std::filesystem::current_path().string() << "\"" << std::endl;
		return 1;
	}

	GameEngine::Core::g_MainWindowsApplication = new GameEngine::Core::Window();
	GameEngine::Core::g_MainWindowsApplication->Init(hInstance);

	std::unique_ptr<GameEngine::Game> game = std::make_unique<GameEngine::Game>(&WindowsMessageLoop, std::move(*config));

	game->Run();

	return 0;
}
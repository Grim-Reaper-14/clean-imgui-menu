#include "Core/Application.hpp"

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    Core::Application application;
    return application.Run();
}

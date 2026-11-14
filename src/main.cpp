#include <windows.h>
#include <exception>
#include "core/Application.h"

int WINAPI WinMain(HINSTANCE inst, HINSTANCE, LPSTR, int) {
    try {
        hat::Application app;
        return app.Run(inst);
    } catch (const std::exception& e) {
        MessageBoxA(nullptr, e.what(), "Hatenium - fatal error", MB_OK | MB_ICONERROR);
    } catch (...) {
        MessageBoxA(nullptr, "Unknown error", "Hatenium - fatal error", MB_OK | MB_ICONERROR);
    }
    return 1;
}

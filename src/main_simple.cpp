#include <windows.h>

// Export mod entry points
extern "C" __declspec(dllexport) void modLoaded() {
    OutputDebugStringA("Collaborative Editor Mod loaded");
}

extern "C" __declspec(dllexport) void modUnloaded() {
    OutputDebugStringA("Collaborative Editor Mod unloaded");
}

#include <windows.h>
#include "CollabManager.hpp"

// Export mod entry points
extern "C" __declspec(dllexport) void modLoaded() {
    OutputDebugStringA("Collaborative Editor Mod loaded");
    
    // Initialize collab manager
    CollabManager::get();
}

extern "C" __declspec(dllexport) void modUnloaded() {
    OutputDebugStringA("Collaborative Editor Mod unloaded");
    
    // Cleanup collab manager
    // Note: In real implementation, would call CollabManager cleanup here
}

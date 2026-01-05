// src/main.cpp
#define REAPERAPI_IMPLEMENT
#include <Salix/ReaperPlugin.h>
#include <Salix/SalixActions.h>


int HookCommand(int command, int flag) {
    if (Salix::ReaperPlugin::Get().OnCommand(command)) return 1;
    return 0;
}

int OnCheckAction(int command) {
    return Salix::ReaperPlugin::Get().OnToggleCheck(command);
}

void HookCustomMenu(const char* menustr, HMENU hMenu, int flag) {
    Salix::ReaperPlugin::Get().OnCustomMenu(menustr, hMenu, flag);
}

void OnTimer() {
    Salix::ReaperPlugin::Get().OnTimer();
}

// --------------------------------------------------------
// ENTRY POINT
// --------------------------------------------------------
extern "C" REAPER_PLUGIN_DLL_EXPORT int REAPER_PLUGIN_ENTRYPOINT(
    HINSTANCE hInstance, reaper_plugin_info_t* rec) 
{
    // --- 1a. HANDLE SHUTDOWN ---
    // If 'rec' is null, Reaper is telling us to clean up and leave.
    if (!rec) {
        // Optional: Call your cleanup logic here
        Salix::ReaperPlugin::Get().Shutdown(); 
        return 0; // Return 0 to confirm exit
    }

    // 1c. Load the tools FIRST
    REAPERAPI_LoadAPI(rec->GetFunc);

    // 1c. Initialize Plugin Instance
    auto& plugin = Salix::ReaperPlugin::Get();

    // 2. Register Internal Actions
    // This now handles all the std::make_unique logic internally
    plugin.RegisterAllActions();

    // 3. Register with Reaper
    if (!plugin.Register(rec)) return 0;

    // 4. Register Hooks
    rec->Register("hookcustommenu", (void*)HookCustomMenu);
    rec->Register("hookcommand", (void*)HookCommand);
    rec->Register("checkaction", (void*)OnCheckAction);
    rec->Register("timer", (void*)OnTimer);

    // Note: We removed AddExtensionsMainMenu() as discussed to simplify logic
    // The context menu hook will work automatically.

    return 1;
}
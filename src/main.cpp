// main.cpp
#define REAPERAPI_IMPLEMENT
#include <Salix/SalixRPT.h>
#include <Salix/version.h>
#include <string>
#include <map>

bool g_startup_reported = false;
int g_startup_ticks = 0;
// preparing startup/registration
int g_salix_rpt_id = 0 ;
int g_salix_rpt_overwrite_id  = 0;
int g_salix_rpt_copy_id = 0;
int g_salix_rpt_append_id = 0;
int g_salix_rpt_insert_id = 0;
int g_salix_rpt_clear_id = 0;

/* Reaper Context names
Menubar:
    'Main file'
    'Main edit'
    'Main view'
    'Main insert'
    'Main item'
    'Main track'
    'Main options'
    'Main actions'
    'Main extensions'
    -----------------------
The rest of the window:
    'Ruler/arrange context'
    'Media item context'
    'Empty TCP context'
    'Track control panel'
    'Transport context'
    'Mixer context'
    -----------------------
*/

const char* main_file_str                   = "Main file";
const char* main_edit_str                   = "Main edit";
const char* main_view_str                   = "Main view";
const char* main_insert_str                 = "Main insert";
const char* main_item_str                   = "Main item";
const char* main_track_str                  = "Main track";
const char* main_options_str                = "Main options";
const char* main_actions_str                = "Main actions";
const char* main_extensions_str             = "Main extensions";
const char* ruler_arrange_context_str       = "Ruler/arrange context";
const char* media_item_context_str          = "Media item context";
const char* empty_tcp_context_str           = "Empty TCP context";
const char* empty_tcp_area_toolbar_str      = "Empty TCP area toolbar";
const char* track_control_panel_str         = "Track control panel";
const char* transport_context_str           = "Transport context";
const char* mixer_context_str               = "Mixer context";

std::map<const std::string, const char*> menu_str =  {
    {"file", main_file_str},
    {"edit", main_edit_str},
    {"view", main_view_str},
    {"insert", main_insert_str},
    {"item", main_item_str},
    {"track", main_track_str},
    {"options ", main_options_str },
    {"actions", main_actions_str},
    {"extensions", main_extensions_str},
    {"ruler", ruler_arrange_context_str},
    {"arrange", ruler_arrange_context_str},
    {"media item", media_item_context_str},
    {"empty tcp", empty_tcp_context_str},
    {"tcp toolbar", empty_tcp_area_toolbar_str},
    {"track control panel", track_control_panel_str},
    {"tcp", track_control_panel_str},
    {"transport", transport_context_str},
    {"mixer", mixer_context_str}
};

void HookCustomMenu(const char* menustr, HMENU hMenu, int flag) {
    /* TEMPORARY: See what REAPER calls the area you right-clicked
    if (menustr) {
        char log[256];
        sprintf(log, "SalixRPT: Right-clicked context: '%s'\n", menustr);
        ShowConsoleMsg(log);
    }
    */
    // 1. Extensions Menu
    if (menustr && !strcmp(menustr, "Main extensions")) {
        int count = GetMenuItemCount(hMenu);
        for (int i = 0; i < count; i++) {
            char buf[128];
            if (GetMenuStringA(hMenu, i, buf, sizeof(buf), MF_BYPOSITION)) {
                if (!strcmp(buf, "Salix")) return; 
            }
        }
        HMENU salixPopup = CreatePopupMenu();
        AppendMenuA(salixPopup, MF_STRING, g_salix_rpt_id, "RPT");
        AppendMenuA(hMenu, MF_POPUP, (UINT_PTR)salixPopup, "Salix");
    }

    if (menustr && (
        !strcmp(menustr, menu_str["ruler"]) || 
        !strcmp(menustr, menu_str["empty tcp"]) || 
        !strcmp(menustr, menu_str["tcp toolbar"]) ||
        !strcmp(menustr, menu_str["track control panel"]) || 
        !strcmp(menustr, menu_str["transport"]) ||
        !strcmp(menustr, menu_str["mixer"]) 
    )) {
        // Deduplication
        // --- STALE MENU PREVENTION ---
        int count = GetMenuItemCount(hMenu);
        for (int i = 0; i < count; i++) {
            char buffer[128];
            if (GetMenuStringA(hMenu, i, buffer, sizeof(buffer), MF_BYPOSITION)) {
                if (!strcmp(buffer, "SalixRPT")) {
                    // FOUND IT! But it might be old/stale.
                    // Delete it so we can rebuild it with the fresh 'hasData' state.
                    DeleteMenu(hMenu, i, MF_BYPOSITION); 
                    break; 
                }
            }
        }

        double start, end;
        GetSet_LoopTimeRange2(nullptr, false, false, &start, &end, false);
        
        bool hasSelection = (start != end);

        // This will now definitely be TRUE if your toolbar button is lit
        bool hasData = Salix::TransferEngine::IsWaiting();

        HMENU salixSubMenu = CreatePopupMenu();

        AppendMenuA(salixSubMenu, hasSelection ? MF_ENABLED : MF_GRAYED, g_salix_rpt_copy_id, "Copy Selection to Buffer");
        AppendMenuA(salixSubMenu, MF_SEPARATOR, 0, NULL);

        
        // Sub-menu for Pasting
        HMENU pasteSubMenu = CreatePopupMenu();
        UINT pasteFlags = hasData ? (MF_STRING | MF_ENABLED) : (MF_STRING | MF_GRAYED);
        AppendMenuA(pasteSubMenu, pasteFlags, g_salix_rpt_overwrite_id, "Overwrite at Cursor");
        AppendMenuA(pasteSubMenu, pasteFlags, g_salix_rpt_append_id, "Append to End");
        AppendMenuA(pasteSubMenu, pasteFlags, g_salix_rpt_insert_id, "Insert & Shift");

        // NEW: Clear Buffer Option
        AppendMenuA(salixSubMenu, MF_SEPARATOR, 0, NULL);
        AppendMenuA(salixSubMenu, hasData ? MF_ENABLED : MF_GRAYED, g_salix_rpt_clear_id, "Clear Buffer");

        AppendMenuA(salixSubMenu, MF_POPUP | pasteFlags, (UINT_PTR)pasteSubMenu, "Paste from Buffer...");
        AppendMenuA(hMenu, MF_POPUP, (UINT_PTR)salixSubMenu, "SalixRPT");
    }
}

int HookCommand(int command, int flag) {
    ReaProject* active = EnumProjects(-1, nullptr, 0);

    if (command == g_salix_rpt_id) { 
        if (Salix::TransferEngine::ConfirmTransfer()) {
            Salix::TransferEngine::CollectData(active);
            Main_OnCommand(40859, 0); // Create New Tab
            // We set waiting to TRUE and specify Mode 0 (Overwrite/Default)
            Salix::TransferEngine::SetWaiting(true, 0);
            Salix::TransferEngine::SetWaitingForTab(true); // NEW: Only for tab transfers
        }
        return 1;
    }


    // COPY LOGIC
    if (command == g_salix_rpt_copy_id) {
        double start, end;
        GetSet_LoopTimeRange2(active, false, false, &start, &end, false);
        if (start != end) {
            // Clear the old flag first to ensure the UI 'flashes' correctly
            Salix::TransferEngine::SetWaiting(false);

            Salix::TransferEngine::CollectDataInRange(active, start, end);
            Salix::TransferEngine::SetWaiting(true);
            Salix::TransferEngine::SetWaitingForTab(true); // NEW: Only for tab transfers

            // Refresh the toolbars so the buttons light up immediately
            RefreshToolbar2(0, g_salix_rpt_copy_id);
            return 1;
        }
    }

    // CLEAR BUFFER LOGIC
    if (command == g_salix_rpt_clear_id) {
        // 1. Reset the logic state
        Salix::TransferEngine::SetWaiting(false);
        Salix::TransferEngine::SetWaitingForTab(false);

        // 2. Force the Toolbar Buttons to turn off (refreshing the UI)
        // We refresh all 3 paste IDs because any of them could be on the toolbar
        RefreshToolbar2(0, g_salix_rpt_overwrite_id);
        RefreshToolbar2(0, g_salix_rpt_append_id);   
        RefreshToolbar2(0, g_salix_rpt_insert_id);   

        ShowConsoleMsg("SalixRPT: Buffer Cleared.\n");
        return 1;
    }

    // PASTE LOGIC
    int mode = -1;
    if (command == g_salix_rpt_overwrite_id) mode = 0;   // Overwrite
    else if (command == g_salix_rpt_append_id) mode = 1; // Append
    else if (command == g_salix_rpt_insert_id) mode = 2; // Insert

    if (mode != -1 && Salix::TransferEngine::IsWaiting() || mode != -1 && Salix::TransferEngine::IsWaitingForTab()) {
        Salix::TransferEngine::InjectData(active, mode);
        return 1;
    }
    return 0;
}


void OnTimer() {
    // --- VERSION CHECK ---
    // 1. Report versioning info once the menu is confirmed live
    if (!g_startup_reported) {
        g_startup_ticks++;
        // Wait a few ticks to ensure the UI hook has had a chance to run
        if (g_startup_ticks > 30) {
            std::string msg = "SalixRPT " + std::string(SALIX_VERSION_STRING);
            msg += " (Build: " + std::string(SALIX_BUILD_DATE) + ") is Live.\n";
            ShowConsoleMsg(msg.c_str());
            g_startup_reported = true;
        }
    }

    
    // 2. Handle the Project Transfer
    if (!Salix::TransferEngine::IsWaiting()) return;

    // Handle the Project Transfer ONLY if we are specifically waiting for a tab
    if (!Salix::TransferEngine::IsWaitingForTab()) return;

    ReaProject* current_last = nullptr;
    int idx = 0;
    while (ReaProject* p = EnumProjects(idx++, nullptr, 0)) current_last = p;

    if (current_last && current_last != Salix::TransferEngine::GetSource()) {
        // 1. Retrieve the stored mode before resetting
        int mode_to_use = Salix::TransferEngine::GetPendingMode();
        
        // 2. Clear the waiting flag
        Salix::TransferEngine::SetWaiting(false);
        Salix::TransferEngine::SetWaitingForTab(false); // Stop the timer
        
        // 3. Inject using the correct logic path (Overwrite, Append, or Insert)
        Salix::TransferEngine::InjectData(current_last, mode_to_use);
        
        ShowConsoleMsg("SalixRPT: Transfer Complete.\n");
    }
}



int OnCheckAction(int command) {
    // If the engine is 'waiting' (buffer is full), light up the Paste buttons
    if (command == g_salix_rpt_overwrite_id || 
        command == g_salix_rpt_append_id || 
        command == g_salix_rpt_insert_id) 
    {
        return Salix::TransferEngine::IsWaiting() ? 1 : 0;
    }
    return -1; // -1 means 'not a toggle action'
}



extern "C" REAPER_PLUGIN_DLL_EXPORT int REAPER_PLUGIN_ENTRYPOINT(HINSTANCE hInstance, reaper_plugin_info_t* rec) {
    if (!rec || rec->caller_version != REAPER_PLUGIN_VERSION) return 0;
    if (REAPERAPI_LoadAPI(rec->GetFunc) != 0) return 0;

    // --- STEP 1: Register IDs FIRST ---
    // This ensures these IDs exist before ANY menus are drawn
    g_salix_rpt_id = rec->Register("command_id", (void*)"SALIXRPT_TRANSFER_FULL");
    g_salix_rpt_copy_id = rec->Register("command_id", (void*)"SALIXRPT_COPY_BUFFER");
    g_salix_rpt_overwrite_id = rec->Register("command_id", (void*)"SALIXRPT_PASTE_OVERWRITE");
    g_salix_rpt_append_id = rec->Register("command_id", (void*)"SALIXRPT_PASTE_APPEND");
    g_salix_rpt_insert_id = rec->Register("command_id", (void*)"SALIXRPT_PASTE_INSERT");
    g_salix_rpt_clear_id = rec->Register("command_id", (void*)"SALIXRPT_CLEAR_BUFFER");
    
    // --- STEP 2: Give them Human-Readable Names ---
    // This is what users will see in the Action List search
    rec->Register("action", (void*)new custom_action_register_t{ g_salix_rpt_copy_id, "SalixRPT: Copy Selection to Buffer" });
    rec->Register("action", (void*)new custom_action_register_t{ g_salix_rpt_overwrite_id, "SalixRPT: Paste Overwrite at Cursor" });
    rec->Register("action", (void*)new custom_action_register_t{ g_salix_rpt_append_id, "SalixRPT: Paste Append to End" });
    rec->Register("action", (void*)new custom_action_register_t{ g_salix_rpt_insert_id, "SalixRPT: Paste Insert and Shift" });
    rec->Register("action", (void*)new custom_action_register_t{ g_salix_rpt_clear_id, "SalixRPT: Clear Copy Buffer" });

    // --- STEP 3: Register Hooks ---
    rec->Register("hookcustommenu", (void*)HookCustomMenu);
    rec->Register("hookcommand", (void*)HookCommand);
    rec->Register("timer", (void*)OnTimer);
    rec->Register("checkaction", (void*)OnCheckAction);

    // --- STEP 4: Build UI Last ---
    AddExtensionsMainMenu();

    return 1;
}
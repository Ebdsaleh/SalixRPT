// src/Salix/ReaperPlugin.cpp
#include <Salix/SalixRPT.h> 
#include <Salix/ReaperPlugin.h>
#include <map>
#include <iostream>
#include <Salix/SalixActions.h>
#include <Salix/version.h>
namespace Salix {

    // ---------------------------------------------------------
    // The Hidden Implementation Struct
    // ---------------------------------------------------------
    struct ReaperPlugin::Pimpl {
        // This effectively replaces your global variable: g_salix_rpt_copy_id
        int copyCommandID = 0; 
        
        // Defines the list of actions we own
        std::vector<std::unique_ptr<Action>> actions;
        
        // Defines a fast lookup map: CommandID -> Action*
        std::map<int, Action*> commandMap;
    };

    // ---------------------------------------------------------
    // The Public Class Implementation
    // ---------------------------------------------------------

    // Singleton Instance
    ReaperPlugin& ReaperPlugin::Get() {
        static ReaperPlugin instance;
        return instance;
    }

    ReaperPlugin::ReaperPlugin() : pimpl(std::make_unique<Pimpl>()) {
        // Use ShowConsoleMsg instead of std::cout so you see it in REAPER
        ShowConsoleMsg("SalixRPT: Plugin Instance Created Successfully.\n");
    }

    ReaperPlugin::~ReaperPlugin() {
        // unique_ptr handles the cleanup of pimpl automatically
        
    }

    bool ReaperPlugin::Register(reaper_plugin_info_t* rec) {
        // 1. Validate Reaper API
        if (!rec) return false;
        
        // AUTO-REGISTRATION LOOP
        for (auto& action : pimpl->actions) {
            // 1. Register the Command ID
            int id = rec->Register("command_id", (void*)action->GetCommandName());
            action->SetCommandID(id);

            // 2. Register the Human-Readable Description
            custom_action_register_t desc = { id, action->GetDescription(), NULL };
            rec->Register("action", (void*)&desc);

            // 3. Map it for fast lookup later
            pimpl->commandMap[id] = action.get();
        }

        return true;
    }


    void ReaperPlugin::RegisterAction(std::unique_ptr<Action> action) {
        pimpl->actions.push_back(std::move(action));
    }

    void ReaperPlugin::RegisterAllActions() {
        RegisterAction(std::make_unique<Salix::TransferFullAction>());
        RegisterAction(std::make_unique<Salix::CopyFullAction>());
        RegisterAction(std::make_unique<Salix::CopySelectionAction>());
        RegisterAction(std::make_unique<Salix::ClearBufferAction>());
        
        RegisterAction(std::make_unique<Salix::SaveTemplateAction>());
        RegisterAction(std::make_unique<Salix::LoadTemplateAction>());
        
        RegisterAction(std::make_unique<Salix::PasteAction>(0, "SALIXRPT_PASTE_OVERWRITE", "SalixRPT: Paste Overwrite"));
        RegisterAction(std::make_unique<Salix::PasteAction>(1, "SALIXRPT_PASTE_APPEND",    "SalixRPT: Paste Append"));
        RegisterAction(std::make_unique<Salix::PasteAction>(2, "SALIXRPT_PASTE_INSERT",    "SalixRPT: Paste Insert"));
    }

    void ReaperPlugin::Shutdown() {
        // Unregister hooks or timers here
    }

    int ReaperPlugin::GetCopyCommandID() const {
        return pimpl->copyCommandID;
    }

    


    void ReaperPlugin::RefreshUI() {
        // Refreshes the toolbar button for every registered action
        for (auto& action : pimpl->actions) {
            RefreshToolbar2(0, action->GetCommandID());
        }
    }


    // The Timer Logic (Migrated from main.cpp)
    void ReaperPlugin::OnTimer() {
        // --- Version Check Logic ---
        static bool startup_reported = false;
        static int startup_ticks = 0;

        if (!startup_reported) {
            startup_ticks++;
            if (startup_ticks > 30) {
                std::string msg = "SalixRPT: " + std::string(SALIX_VERSION_STRING);
                msg += " (Build: " + std::string(SALIX_BUILD_DATE) + " is Live\n";
                ShowConsoleMsg(msg.c_str());
                startup_reported = true;
            }
        }

        // --- Transfer Engine Logic ---
        if (!TransferEngine::IsWaiting()) return;
        if (!TransferEngine::IsWaitingForTab()) return;

        ReaProject* current_last = nullptr;
        int idx = 0;
        while (ReaProject* p = EnumProjects(idx++, nullptr, 0)) current_last = p;

        if (current_last && current_last != TransferEngine::GetSource()) {
            int mode_to_use = TransferEngine::GetPendingMode();

            TransferEngine::SetWaiting(false);
            TransferEngine::SetWaitingForTab(false);

            TransferEngine::InjectData(current_last, mode_to_use);
            ShowConsoleMsg("SalixRPT: Transfer Complete.\n");
        }
    }

    // The Menu Logic (Refactored and Cleaned) 
    bool ReaperPlugin::OnCustomMenu(const char* menustr, void* hMenuPtr, int flag) {
        HMENU hMenu = (HMENU)hMenuPtr;  // Cast generic pointer to HMENU, should we cast dynamically for safety?

        // Define the context strings we care about
        const char* target_contexts[] = {
            "Main extensions",
            "Ruler/arrange context",
            "Empty TCP context",
            "Empty TCP area toolbar",
            "Track control panel",
            "Transport context",
            "Mixer context",
            nullptr
        };

        bool is_valid_context = false;
        bool is_main_menu = false;
        const char* extName = GetContextName("extensions");

        if (menustr) {
            for (int i = 0; target_contexts[i]; i++) {
                if (strcmp(menustr, target_contexts[i]) == 0) {
                    is_valid_context = true;
                    if (strcmp(menustr, extName) == 0) {
                        is_main_menu = true;
                    }
                    break;
                }
            }
        }

        if (!is_valid_context) return false;

        // --- Remove Stale "SalixRPT" Entries ---
        int count = GetMenuItemCount(hMenu);
        for (int i = 0; i < count; i++) {
            char buffer[128];
            if (GetMenuStringA(hMenu, i, buffer, sizeof(buffer), MF_BYPOSITION)) {
                if (strcmp(buffer, "SalixRPT") == 0) {
                    DeleteMenu(hMenu, i, MF_BYPOSITION);
                    break;
                }
            }
        }

        // --- Check State ---
        double start, end;
        GetSet_LoopTimeRange2(nullptr, false, false, &start, &end, false);
        bool hasSelection = (start != end);
        bool hasData = TransferEngine::IsWaiting();

        // --- Retrieve Dynamic IDs ---
        // This is why we need GetActionID! We don't have global variables anymore.
        int id_transfer_all = GetActionID("SALIXRPT_TRANSFER_ALL");
        int id_copy_full = GetActionID("SALIXRPT_COPY_FULL");
        int id_copy      = GetActionID("SALIXRPT_COPY_BUFFER");
        int id_overwrite = GetActionID("SALIXRPT_PASTE_OVERWRITE");
        int id_append    = GetActionID("SALIXRPT_PASTE_APPEND");
        int id_insert    = GetActionID("SALIXRPT_PASTE_INSERT");
        int id_clear     = GetActionID("SALIXRPT_CLEAR_BUFFER");
        int id_save_tmpl = GetActionID("SALIXRPT_SAVE_TEMPLATE");
        int id_load_tmpl = GetActionID("SALIXRPT_LOAD_TEMPLATE");

        // --- Build Menu ---
        HMENU salixSubMenu = CreatePopupMenu();

        // --- GROUP 1: FULL PROJECT TOOLS ---
        // (Available everywhere, not just main menu, because it's a utility now)
        AppendMenuA(salixSubMenu, MF_STRING, id_copy_full, "Copy All Metadata to Buffer");

        // --- TEMPLATES SUBMENU (New!) ---
        HMENU templateSubMenu = CreatePopupMenu();
        AppendMenuA(templateSubMenu, MF_STRING, id_save_tmpl, "Save Buffer as Template...");
        AppendMenuA(templateSubMenu, MF_STRING, id_load_tmpl, "Load Template to Buffer...");

        // Add the submenu to the main list (maybe at the top or bottom)
        AppendMenuA(salixSubMenu, MF_POPUP, (UINT_PTR)templateSubMenu, "Groove Templates");
        AppendMenuA(salixSubMenu, MF_SEPARATOR, 0, NULL);

        // --- SECTION A: MAIN MENU ONLY 'Extensions' ---
        // We only show "Transfer Full Project" if we are in the Extensions menu.
        if (is_main_menu) {
            AppendMenuA(salixSubMenu, MF_STRING, id_transfer_all, "Transfer All Metadata to New Project Tab");
            AppendMenuA(salixSubMenu, MF_SEPARATOR, 0 , NULL);

        }

        // --- SECTION B: STANDARD TOOLS 'Right-Click' Context menu ---

        AppendMenuA(salixSubMenu, hasSelection ? MF_ENABLED : MF_GRAYED, id_copy, "Copy Selection to Buffer");
        AppendMenuA(salixSubMenu, MF_SEPARATOR, 0, NULL);

        // Paste Submenu
        HMENU pasteSubMenu = CreatePopupMenu();
        UINT pasteFlags = hasData ? (MF_STRING | MF_ENABLED) : (MF_STRING | MF_GRAYED);
        AppendMenuA(pasteSubMenu, pasteFlags, id_overwrite, "Overwrite at Cursor");
        AppendMenuA(pasteSubMenu, pasteFlags, id_append,    "Append to End");
        AppendMenuA(pasteSubMenu, pasteFlags, id_insert,    "Insert & Shift");

        AppendMenuA(salixSubMenu, MF_POPUP | pasteFlags, (UINT_PTR)pasteSubMenu, "Paste from Buffer...");

        // Cleanup
        AppendMenuA(salixSubMenu, MF_SEPARATOR, 0, NULL);
        AppendMenuA(salixSubMenu, hasData ? MF_ENABLED : MF_GRAYED, id_clear, "Clear Metadata Buffer");

        // Attach to Reaper
        AppendMenuA(hMenu, MF_POPUP, (UINT_PTR)salixSubMenu, "SalixRPT");

        return true;
    }


    // Helper to find IDs (needed for the Menu)
    int ReaperPlugin::GetActionID(const char* commandName) const {
        // Iterate actions to find the matching name
        for (const auto& action : pimpl->actions) {
            if (strcmp(action->GetCommandName(), commandName) == 0) {
                return action->GetCommandID();
            }
        }
        
        return 0;  // Not found
    }

    
    // Handles HookCommand: Looks up the ID and runs Execute()
    bool ReaperPlugin::OnCommand(int commandID) {
        auto it = pimpl->commandMap.find(commandID);
        if (it != pimpl->commandMap.end()) {
            it->second->Execute();
            return true;
        }
        return false;
    }


    // Handles OnCheckAction: Looks up the ID and checks the Toggle State
    int ReaperPlugin::OnToggleCheck(int commandID) {
        auto it = pimpl->commandMap.find(commandID);
        if (it != pimpl->commandMap.end()) {
            return it->second->GetToggleState();            
        }
        return -1;
    }

    /* Reaper Context names  - For reference
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
    const char* ReaperPlugin::GetContextName(const std::string& shortName) {
        static const std::map<std::string, const char*> context_map = {
            // STATIC LOCAL VARIABLE:
            // This map is built ONLY the first time this function runs.
            // It is thread-safe and crash-proof.
            {"file",                "Main file"},
            {"edit",                "Main edit"},
            {"view",                "Main view"},
            {"insert",              "Main insert"},
            {"item",                "Main item"},
            {"track",               "Main track"},
            {"options",             "Main options"},
            {"actions",             "Main actions"},
            {"extensions",          "Main extensions"},
            
            {"ruler",               "Ruler/arrange context"},
            {"arrange",             "Ruler/arrange context"},
            
            {"media item",          "Media item context"},
            
            {"empty tcp",           "Empty TCP context"},
            {"tcp toolbar",         "Empty TCP area toolbar"},
            
            {"track control panel", "Track control panel"},
            {"tcp",                 "Track control panel"},
            
            {"transport",           "Transport context"},
            {"mixer",               "Mixer context"}
        };
        auto it = context_map.find(shortName);
        if (it != context_map.end()) {
            return it->second;
        }
        return nullptr;
    }

}  // namespace Salix
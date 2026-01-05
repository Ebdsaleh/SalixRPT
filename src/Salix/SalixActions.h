// src/Salix/SalixActions.h
#pragma once
#include <Salix/Action.h>
#include <Salix/SalixRPT.h>
#include <Salix/ReaperPlugin.h>
#include "reaper_plugin_functions.h" // For Reaper API calls

namespace Salix {
    // =========================================================
    // TRANSFER TO NEW TAB ACTION
    // =========================================================
    class TransferFullAction : public Action {
        public:
            const char* GetCommandName() const override { return "SALIXRPT_TRANSFER_ALL"; }
            const char* GetDescription() const override { return "SalixRPT: Transfer All Metadata to New Project Tab"; }

            void Execute() override {
                if (TransferEngine::ConfirmTransfer()) {
                    ReaProject* active = EnumProjects(-1, nullptr, 0);
                    TransferEngine::CollectData(active);
                    // Command 40859 is Reaper's native ID for "New project tab"
                    Main_OnCommand(40859, 0);

                    // Set waiting to TRUE and specify Mode 0 (Overwrite/Default)
                    TransferEngine::SetWaiting(true, 0);
                    TransferEngine::SetWaitingForTab(true);
                }
            }
    };



    // =========================================================
    // COPY SELECTION ACTION
    // =========================================================
    class CopySelectionAction : public Action {
        public:
            const char* GetCommandName() const override { return"SALIXRPT_COPY_BUFFER"; }
            const char* GetDescription() const override { return "SalixRPT: Copy Selection to Buffer"; }

            void Execute() override {
                // Retrieve Active Project
                ReaProject* active = EnumProjects(-1, nullptr, 0);
                double start, end;
                GetSet_LoopTimeRange2(active, false, false, &start, &end, false);

                if (start != end) {
                    // 1. Reset everything first for safety
                    TransferEngine::SetWaiting(false);
                    
                    // 2. Collect the new data
                    TransferEngine::CollectDataInRange(active, start, end);
                    
                    // 3. Arm the "Paste" buttons
                    // We set mode 0 (Overwrite) as a default, though the user will likely choose their own mode via menu.
                    TransferEngine::SetWaiting(true, 0); 

                    // 4. CRITICAL FIX: TURN OFF AUTO-INJECT
                    // The original code had this set to 'true', which caused the leak.
                    TransferEngine::SetWaitingForTab(false); // <--- CHANGE THIS TO FALSE
                    
                    // Refresh UI to light up paste buttons
                    ReaperPlugin::Get().RefreshUI();
                    
                    // Optional: Feedback
                    ShowConsoleMsg("SalixRPT: Selection Copied to Buffer.\n");
                }
            }
        };

    // =========================================================
    // PASTE ACTIONS (Overwrite, Append, Insert)
    // =========================================================
    class PasteAction : public Action {
        public:
            // We use one class for all 3 paste modes to reduce code duplication
            PasteAction(int new_mode, const char* new_name, const char* new_desc) 
                : mode(new_mode), name(new_name), desc(new_desc) {}
            
            const char* GetCommandName() const override { return name; }
            const char* GetDescription() const override { return desc; }

            void Execute() override {
                ReaProject* active = EnumProjects(-1, nullptr, 0);

                // Only execute if we have data or are waiting
                if (TransferEngine::IsWaiting() || TransferEngine::IsWaitingForTab()) {
                    TransferEngine::InjectData(active, mode);
                }
            }

            // Logic moved from OnCheckAction
            int GetToggleState() override {
                // Lights up if data is in the buffer
                return TransferEngine::IsWaiting() ? 1 : 0;
            }

        private:
            int mode;
            const char* name;
            const char* desc;

    };
    // =========================================================
    // CLEAR BUFFER ACTION
    // =========================================================
    class ClearBufferAction : public Action {
        public:
            const char* GetCommandName() const override { return "SALIXRPT_CLEAR_BUFFER"; }
            const char* GetDescription() const override { return "SalixRPT: Clear Copy Buffer"; }

            void Execute() override {
                // NUKE IT. Even if a flag accidentally turns on later, there is zero data to paste.
                TransferEngine::ClearQueue();
                TransferEngine::SetWaiting(false);
                TransferEngine::SetWaitingForTab(false);
                
                ShowConsoleMsg("SalixRPT: Buffer Cleared.\n");
                
                // Force buttons to turn off
                ReaperPlugin::Get().RefreshUI();
            }
        };  
    
        // =====================================================================
        // COPY FULL PROJECT (SWS STYLE) inspired by 'Copy Marker Set' action
        // =====================================================================
        class CopyFullAction : public Action {
            public:
                const char* GetCommandName() const override { return "SALIXRPT_COPY_FULL"; }
                const char* GetDescription() const override { return "SalixRPT: Copy Full Project Metadata to Buffer"; }

                void Execute() override {
                    // 1. Get Active Project
                    ReaProject* active = EnumProjects(-1, nullptr, 0);

                    // 2. Clear any old junk first
                    TransferEngine::ClearQueue();

                    // 3. Capture EVERYTHING (Markers, Regions, Tempo)
                    // We use CollectData() instead of CollectDataInRange()
                    TransferEngine::CollectData(active);

                    // 4. Arm the Buffer
                    // IMPORTANT: Set WaitingForTab to FALSE so it doesn't leak/auto-paste!
                    TransferEngine::SetWaiting(true);
                    TransferEngine::SetWaitingForTab(false);

                    // 5. Feeback
                    ReaperPlugin::Get().RefreshUI();
                    ShowConsoleMsg("SalixRPT: Entire Project Metadata Copied to Buffer.\n");
                }
            
        };

}  // namespace Salix
// src/Salix/SalixRPT.h
#pragma once
#include "../WDL/wdltypes.h"
#define Reaproject ReaProject
#include "reaper_plugin.h"
#include "reaper_plugin_functions.h"
#include <vector>
#include <string>
#include <windows.h>

namespace Salix {
    struct Marker { bool isrgn; double pos; double rgnend; std::string name; int color; };
    struct TempoEvent { double time; double bpm; int num; int den; bool linear; };
    struct MusicalContext {double bpm; int numerator; int denominator; };
    struct SalixMarker { 
        std::string name;
        int source_index; // Store the original ID (e.g., '1', '2', '3')
        double relative_seconds; // Distance from selection start
        MusicalContext context; // The signature/tempo active AT this marker
        bool is_region;
        double duration;        // Only for Regions
        int color;        
    };

    // The "Data Packet" for the transfer
    struct ExportQueue {
        std::vector<SalixMarker> items;
        double total_duration_seconds;
    };

    class TransferEngine {
    public:
        static void Init(void* rec_ptr);
        static void CollectData(ReaProject* source_project);
        static void CollectDataInRange(ReaProject* source_project, double start, double end);
        
        // REFINED: Now supports your 3 modes (0=Overwrite, 1=Append, 2=Insert)
        static void InjectData(ReaProject* target, int mode);
        static bool IsWaiting() { return is_waiting; }
        static void SetWaiting(bool w, int mode = 0) {
            is_waiting = w;
            pending_mode = mode;
        }
        static bool IsWaitingForTab() { return waiting_for_tab; } 
        static void SetWaitingForTab(bool w) { waiting_for_tab = w; }
        static ReaProject* GetSource() { return src_project; }
        static bool ConfirmTransfer(); // NEW: UI Prompt
        static std::string pending_notes; // NEW: Added for Notes
        static double pending_offset;    // NEW: Added for SMPTE
        static int GetPendingMode() { return pending_mode; }
        static ExportQueue& GetQueue() { return queue; }
        // Deletes the data from memory
        static void ClearQueue() {
            GetQueue().items.clear();
            GetQueue().total_duration_seconds = 0.0;
            SetWaiting(false, 0);
            waiting_for_tab = false;
        }

        
    private:
        static ReaProject* src_project;
        static bool is_waiting;
        static bool waiting_for_tab;
        // We replace the old vectors with our new ExportQueue
        static ExportQueue queue;
        static double CalculateSelectionLength();
        // Helper for the 'Insert & Shift' logic
        static void ShiftExistingData(ReaProject* target_project, double start_time, double shift_amount);
        // Injection modes
        static void InjectOverwrite(ReaProject* target_project, double anchor_time);
        static void InjectAppend(ReaProject* target_project, double anchor_time);
        static void InjectInsertAndShift(ReaProject* target_project, double anchor_time);
        static int pending_mode; // NEW: Stores 0, 1, or 2
    };
} // namespace Salix
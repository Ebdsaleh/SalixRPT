// Salix/TransferEngine.cpp
#include "SalixRPT.h"
#include <string>
using namespace Salix;

// Human-readable constants for REAPER API defaults
const int CREATE_NEW_MARKER = -1;
const int IGNORE_MEASURE_BEAT = -1;
const int AUTO_ID = -1;
const int UNDO_ALL_CHANGES = -1;

ExportQueue TransferEngine::queue;
ReaProject* TransferEngine::src_project = nullptr;
int Salix::TransferEngine::pending_mode = 0;
bool TransferEngine::is_waiting = false;
bool TransferEngine::waiting_for_tab = false; 
double TransferEngine::pending_offset = 0.0;

void TransferEngine::CollectData(ReaProject* source_project) {
    src_project = source_project;
    if (!src_project) return;

    // Reset the smart queue for a new full-project capture session
    queue.items.clear();
    double min_pos = 0.0; 
    double max_pos = 0.0;

    // 1. Process Markers and Regions
    int total = CountProjectMarkers(source_project, nullptr, nullptr);

    for (int i = 0; i < total; i++) {
        bool is_region;
        double position, region_end;
        const char* name;
        int index, color;

        if (EnumProjectMarkers3(
            source_project,
            i,
            &is_region,
            &position,
            &region_end,
            &name,
            &index,
            &color)) {

            SalixMarker sm;
            sm.name = name;
            sm.source_index = index;
            // In a full transfer, the 'anchor' is the start of the project (0.0)
            sm.relative_seconds = position; 
            sm.is_region = is_region;
            sm.duration = is_region ? (region_end - position) : 0.0;
            sm.color = color;

            // 2. BAKE CONTEXT: Capture the tempo/time signature at this marker's specific location
            TimeMap_GetTimeSigAtTime(
                source_project, 
                position, 
                &sm.context.numerator, 
                &sm.context.denominator, 
                &sm.context.bpm
            );

            queue.items.push_back(sm);

            // 3. Track the footprint for the 'total_duration_seconds'
            // This is vital for the 'Insert & Shift' logic if this queue is used later
            double actual_end = is_region ? region_end : position;
            if (actual_end > max_pos) max_pos = actual_end;
        }
    }

    // Calculate the total length of the 'Metadata Packet'
    queue.total_duration_seconds = max_pos;
    
    // Optional: Log the capture result for visual confirmation
    std::string report = "SalixRPT: Captured " + std::to_string(queue.items.size()) + " smart objects.\n";
    ShowConsoleMsg(report.c_str());
}



// REFINED: Now supports your 3 modes (0=Overwrite, 1=Append, 2=Insert)
void TransferEngine::InjectData(ReaProject* target_project, int mode) {
    auto* target = target_project;
    if (!target) return;

    // 1. Calculate the 'Anchor'—where the very first marker will land
    double anchor_time  = 0.0;

    switch (mode) {
        case 0: // OVERWRITE
        anchor_time = GetCursorPositionEx(target);
        InjectOverwrite(target, anchor_time);
        break;

        case 1: // APPEND
        // Find the end of the project and add a 2-second gap
        anchor_time = GetProjectLength(target) + 2.0;
        InjectAppend(target, anchor_time);
        break;

        case 2: // INSERT & SHIFT
        anchor_time = GetCursorPositionEx(target);
        InjectInsertAndShift(target, anchor_time);
        break;

        default: // INVALID MODE
        ShowConsoleMsg("SalixRPT: Unknown Injection Mode!\n");
        break;
    }
}



void TransferEngine::InjectOverwrite(ReaProject* target_project, double anchor_time) {
    auto* target = target_project;
    if (!target) {
        ShowConsoleMsg("SalixRPT: target project invalid!\n");
        return;
    }

    Undo_BeginBlock2(target);

    for (const auto& marker : queue.items) {
        double final_position = anchor_time + marker.relative_seconds;
        
        // --- PRECISION MIRROR LOGIC ---
        // 1. Get the CURRENT state of the target project at this exact position
        int target_num, target_den;
        double target_bpm;

        TimeMap_GetTimeSigAtTime(target, final_position, &target_num, &target_den, &target_bpm);

        // 2. Only inject if the marker's baked context is DIFFERENT from the target's current state
        bool needs_tempo_change =   (target_bpm != marker.context.bpm) ||
                                    (target_num != marker.context.numerator) ||
                                    (target_den != marker.context.denominator);
        
        if (needs_tempo_change) {
            SetTempoTimeSigMarker(
            target,
            CREATE_NEW_MARKER,  // Index: -1 (New Marker)
            final_position,
            IGNORE_MEASURE_BEAT,  // Measure: -1 (Use seconds)
            IGNORE_MEASURE_BEAT,  // Beat: -1 (Use seconds)
            marker.context.bpm,
            marker.context.numerator,
            marker.context.denominator,
            false
            );
        }
        
        // LOGIC: If the marker has no name, use its source ID as the name
        std::string final_name = marker.name;
        if (final_name.empty()) {
            final_name = std::to_string(marker.source_index);
        } 
        // 3. Always place the marker itself
        AddProjectMarker2(
            target,
            marker.is_region,
            final_position,
            final_position + marker.duration,
            final_name.c_str(),
            // Attempt to preserve the exact ID number,
            // was using this previously -1 (Auto-increment ID).
            marker.source_index,  
            marker.color);  
    }

    Undo_EndBlock2(target, "SalixRpt: Overwrite Metadata", UNDO_ALL_CHANGES); 
    UpdateTimeline();
}



void TransferEngine::InjectAppend(ReaProject* target_project, double anchor_time) {
    auto* target = target_project;
    if (!target) {
        ShowConsoleMsg("SalixRPT: target project invalid!\n");
        return;
    }

    // Logic is identical to Overwrite, but anchor was set to GetProjectLength()
    anchor_time = GetProjectLength(target);
    InjectOverwrite(target, anchor_time);
}



void TransferEngine::InjectInsertAndShift(ReaProject* target_project, double anchor_time) {
    auto* target = target_project;
    if (!target) {
        ShowConsoleMsg("SalixRPT: target project invalid!\n");
        return;
    }

    Undo_BeginBlock2(target);
    // We use the total length of the queue to know how far to push things
    // The 'shift_amount' is the total length of the music we are bringing in
    double shift_amount = queue.total_duration_seconds;

    // PHASE 1. First move everything in the target project that's TO-THE-RIGHT-OF-THE-CURSOR
    ShiftExistingData(target, anchor_time, shift_amount);

    // PHASE 2. Now perform the standard injection into the empty space we just made
    // We reuse InjectOverwrite because the 'anchor_time' is already at the start of the hole
    InjectOverwrite(target, anchor_time);

    Undo_EndBlock2(target, "SalixRpt: Insert & Shift Metadata", UNDO_ALL_CHANGES);
    UpdateTimeline();
}



void TransferEngine::ShiftExistingData(ReaProject* target_project, double start_time, double shift_amount) {
    auto* target = target_project;
    if (!target) {
        ShowConsoleMsg("SalixRPT: target project invalid!\n");
        return;
    }

    // 1. Shift Markers and Regions
    // We get the total count first
    int marker_count, region_count;
    int total_count = CountProjectMarkers(target, &marker_count, &region_count);

    // Iterate backwards to safely update positions without index conflicts
    for (int i = total_count - 1; i >= 0; i--) {
        bool is_region;
        double position, region_end;
        const char* name;
        int index, color;
    
        if (EnumProjectMarkers3(
            target, i, &is_region, &position,
            &region_end, &name,
            &index, &color)) {
            // Only shift items that are at or after the insertion point
            if (position >= start_time) {
                // SetProjectMarker3 updates the existing marker's position
                SetProjectMarker3(
                    target,
                    index,
                    is_region,
                    position + shift_amount,
                    region_end + shift_amount,
                    name,
                    color
                );
            }
        }
    }
    
    // 2. Shift Tempo and Time Signature Markers
    int tempo_total = CountTempoTimeSigMarkers(target);

    for (int i = tempo_total - 1; i >= 0; i--) {
        double time_pos, bpm;
        int numerator, denominator;
        bool is_linear;

        if (GetTempoTimeSigMarker(
            target, i, &time_pos,
            nullptr, nullptr, &bpm,
            &numerator, &denominator, &is_linear )) {
            if (time_pos >= start_time) {
                // Update the existing tempo marker with the new shifted time
                SetTempoTimeSigMarker(
                    target,
                    i,
                    time_pos + shift_amount,
                    IGNORE_MEASURE_BEAT,
                    IGNORE_MEASURE_BEAT,
                    bpm,
                    numerator,
                    denominator,
                    is_linear
                );
            }
        }
    }
}



bool TransferEngine::ConfirmTransfer() {
    // Create a standard Windows Message Box parented to the REAPER main window
    int result = MessageBoxA(
        GetMainHwnd(),
        "Would you like to copy Metadata (Markers & Tempo) to the new project tab?", 
        "SalixRPT: Copy Metadata?",
        MB_YESNOCANCEL | MB_ICONQUESTION | MB_TOPMOST
    );
    return (result == IDYES);
}

void TransferEngine::CollectDataInRange(ReaProject* source_project, double start, double end) {
    src_project = source_project;
    if (!src_project) {
        ShowConsoleMsg("SalixRPT: source project invalid!\n");
        return;
    }
    queue.items.clear(); // Clear our new smart queue
    double min_found = end;
    double max_found = start;
    int marker_count, region_count;
    int total = CountProjectMarkers(source_project, &marker_count, &region_count);

    for (int i = 0; i < total; i++) {
        bool is_region;
        double position, region_end;
        const char* name;
        int index, color;
        
        if (EnumProjectMarkers3(
            source_project, i, &is_region, &position,
            &region_end, &name, &index, &color)
        ) {
            // Check if this item is within our selection
            bool in_range = is_region ? (
                position < end && region_end >start
            ) : (
                position >= start && position <= end
            );

            if (in_range) {
                // 1. Capture the Musical Context at this exact spot
                MusicalContext context;
                TimeMap_GetTimeSigAtTime(
                    source_project,
                    position,
                    &context.numerator,
                    &context.denominator,
                    &context.bpm
                );

                // 2. Create our Smart Marker
                SalixMarker sm;
                sm.name = name;
                sm.source_index = index;
                sm.relative_seconds = position - start;  // Distance from the start of the white bar
                sm.is_region = is_region;
                sm.duration = is_region ? (region_end - position) : 0.0;
                sm.color = color;
                sm.context = context;
                
                queue.items.push_back(sm);

                // 3. Track the footprint for the shift amount
                if (position < min_found) min_found = position;
                double actual_end =  is_region ? region_end : position;
                if (actual_end > max_found) max_found = actual_end;
            }
        }
    }

    // Update the total footprint for "Insert & Shift" logic
    queue.total_duration_seconds = CalculateSelectionLength();
}



double TransferEngine::CalculateSelectionLength() {
    if (queue.items.empty()) return 0.0;
    double min_pos = 999999.0;
    double max_pos = -999999.0;
    
    for (const auto& item: queue.items) {
        // Track the earliest point
        if (item.relative_seconds < min_pos) min_pos = item.relative_seconds;

        // Track the furthest point (accounting for regions)
        double item_end = item.relative_seconds + item.duration;
        if (item_end > max_pos) max_pos = item_end;
    }
    return max_pos - min_pos;
}

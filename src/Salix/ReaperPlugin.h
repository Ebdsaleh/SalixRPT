// src/Salix/ReaperPlugin.h
#pragma once
#include <memory>
#include <vector>
#include <Salix/Action.h>

// Forward declaration of the struct containing the internal data
// (This is the magic of Pimpl: The compiler doesn't need to know what's in it yet)

struct reaper_plugin_info_t; // Forward declare Reaper's struct

namespace Salix
{
    class ReaperPlugin
    {
    public:
        // Singleton access (Standard for Reaper extensions)
        static ReaperPlugin& Get();

        // Constructor / Destructor
        ReaperPlugin();
        ~ReaperPlugin();

        // The Main Setup
        // Returns true if initialization succeeded
        bool Register(reaper_plugin_info_t* rec);

        void RegisterAction(std::unique_ptr<Action> action);

        void RegisterAllActions();

        bool OnCommand(int commandID);
        int OnToggleCheck(int commandID);

        void RefreshUI();

        // Cleanup actions
        void Shutdown();

        // Getters for your specific state (previously global variables)
        int GetCopyCommandID() const;

        // Handle the Timer (Version check + Tab Transfer)
        void OnTimer();

        // Handle the Menu Hook
        // Returns true if we handled the menu, false if we didn't
        bool OnCustomMenu(const char* menustr, void* hMenu, int flag);

        // Helper to find an ID by its name (Crucial for building menus!)
        int GetActionID(const char* commandName) const;

        // The "Rosetta Stone" Lookup
        // Usage: ReaperPlugin::GetContextName("tcp") -> returns "Track control panel"
        static const char* GetContextName(const std::string& shortName);

    private:
        // The opaque pointer that holds all the "Messy" Reaper internals
        struct Pimpl;
        std::unique_ptr<Pimpl> pimpl;
    };
}
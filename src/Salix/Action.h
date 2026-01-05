// src/Salix/Action.h
#pragma once
#include <string>

namespace Salix {

    class Action {
        public:
            virtual ~Action() = default;

            // 1. Identity
            // The internal ID string Reaper uses (e.g., "SALIXRPT_COPY")
            virtual const char* GetCommandName() const = 0;
            
            // The human-readable name for the Action List
            virtual const char* GetDescription() const = 0;

            // 2. Behavior
            virtual void Execute() = 0;

            // 3. UI State
            // Returns 1 (On), 0 (Off), or -1 (Not a toggle/No state)
            virtual int GetToggleState() { return -1; }

            // 4. Runtime ID (Assigned by Reaper)
            void SetCommandID(int id) { commandID = id; }
            int GetCommandID() const { return commandID; }

        private:
            int commandID = 0;
    };
}  // namespace Salix
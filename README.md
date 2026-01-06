# SalixRPT (Reaper Project Transfer)

**SalixRPT** is a C++ extension for the [REAPER](https://www.reaper.fm/) Digital Audio Workstation. It provides advanced tools for copying, moving, and templating project metadata—specifically **Markers**, **Regions**, **Tempo Maps**, and **Time Signatures**—independent of audio items.

This tool solves the common problem: *"How do I copy the song structure from Project A to Project B without messing up my timeline?"*

![SalixRPT Menu](https://github.com/Ebdsaleh/SalixRPT/raw/main/assets/menu_screenshot.png) ## 🚀 Features

### 1. The Transfer Engine
SalixRPT uses an internal "Smart Buffer" that captures metadata relative to the project start or a specific selection.
* **Copy Selection:** Grabs markers/tempo only within your time selection.
* **Copy Full Project:** Grabs the entire structure of the active project.
* **Hard Clear:** Physically wipes the buffer from memory to prevent accidental pastes.

### 2. Intelligent Injection Modes
When pasting metadata into a target project, you can choose how it interacts with existing data:
* **Overwrite:** Pastes at the cursor. If a tempo marker exists at that exact spot, it updates it.
* **Append:** Automatically finds the end of your project, adds a 2-second safety gap, and pastes the structure there.
* **Insert & Shift:** The "Ripple Edit" mode. It splits your project at the cursor, pushes everything to the right, and inserts the new structure in the gap.

### 3. Groove Templates (JSON)
Save your favorite song structures to disk!
* **Save Template:** Exports the current buffer to a `.json` file.
* **Load Template:** Imports a saved structure back into the buffer, ready to paste into any project.
* **Portable:** Share `.json` files between computers or users.

### 4. One-Click Transfer
* **Transfer to New Tab:** A macro that copies the current project's metadata, opens a fresh project tab, and pastes it immediately. Perfect for starting a new mix or arrangement based on an existing template.

---

## 📦 Installation

1.  Download the latest release from the [Releases Page](../../releases).
2.  Locate your REAPER User Plugins directory:
    * **Windows:** `C:\Users\<YOU>\AppData\Roaming\REAPER\UserPlugins`
    * *(Or Options -> Show REAPER resource path in explorer/finder)*
3.  Drop `reaper_salix_rpt-x64.dll` into that folder.
4.  Restart REAPER.

## 🛠️ Build from Source

**Requirements:**
* CMake 3.15+
* Visual Studio 2022 (MSVC)
* [nlohmann/json](https://github.com/nlohmann/json) (Header-only)

**Steps:**
```bash
# 1. Clone the repo (recursive for dependencies if needed)
git clone https://github.com/Ebdsaleh/SalixRPT.git
cd SalixRPT

# 2. Build Release (Auto-deploys to APPDATA if REAPER is closed)
# Use the included batch script for convenience:
./build_release.bat
```

## 🎮 Usage Guide

Access the tools via the **Extensions** menu -> **SalixRPT**.

| Action | Description |
| :--- | :--- |
| **Copy Full Project** | Snapshots the entire project structure to the buffer. |
| **Copy Selection** | Snapshots only the structure within the Loop Points. |
| **Groove Templates** | Save/Load the buffer to `.json` files on disk. |
| **Paste: Overwrite** | Pastes data starting at the Edit Cursor. |
| **Paste: Insert** | Shifts existing project content to make room for the new data. |

📜 Credits

Author: Ebdsaleh

Dependencies:

WDL / SWELL (Cockos) - The backbone of REAPER extensions.

nlohmann/json - JSON for Modern C++.

Built with ❤️ for the REAPER Community.

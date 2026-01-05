cmake_minimum_required(VERSION 3.15)
project(SalixRPT)

# --- VERSIONING LOGIC ---
set(SALIX_VERSION_MAJOR 1)
set(SALIX_VERSION_MINOR 0)
set(SALIX_VERSION_PATCH 0)

# Get current date for the build string
string(TIMESTAMP SALIX_BUILD_DATE "%Y-%m-%d")

# Generate the header file in the build directory
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Salix/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/Salix/version.h"
)

# 1. Standard C++ Settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

# 2. Path Definitions
# This allows you to #include "SalixRPT.h" instead of "../Salix/SalixRPT.h"
set(SALIX_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(REAPER_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendors/reaper/sdk")
# Add the generated folder to your include paths
include_directories("${CMAKE_CURRENT_BINARY_DIR}/generated")

include_directories(
    ${SALIX_SRC_DIR}
    ${SALIX_SRC_DIR}/Salix
    ${REAPER_SDK_DIR}
    ${REAPER_SDK_DIR}/WDL
)

# 3. Collect Source Files
# Explicitly grouping these helps as the project grows
set(SALIX_CORE_SOURCES
    "src/Salix/SalixRPT.h"
    "src/Salix/TransferEngine.cpp"
    "src/Salix/ReaperPlugin.cpp"
)

set(REAPER_WRAPPER_SOURCES
    "src/main.cpp"
)

# 4. Create the Extension Library
add_library(${PROJECT_NAME} SHARED 
    ${SALIX_CORE_SOURCES} 
    ${REAPER_WRAPPER_SOURCES}
)

# 5. Windows Linker Settings
if(WIN32)
    # Required for the Win32 Menu and UI calls
    target_link_libraries(${PROJECT_NAME} PRIVATE User32 Gdi32)
    
    # REAPER expects extension names to be exactly .dll on Windows
    set_target_properties(${PROJECT_NAME} PROPERTIES 
        PREFIX "" 
        OUTPUT_NAME "reaper_salix_rpt-x64"
        SUFFIX ".dll"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif()

# copy dll to Reaper Plugin directory
if(WIN32)
   # DYNAMIC PATH: Works for any user on any Windows machine
   # $ENV{APPDATA} resolves to "C:\Users\YourName\AppData\Roaming"
   set(REAPER_PLUGIN_DIR "$ENV{APPDATA}/REAPER/UserPlugins")

   # This command runs AFTER the build is finished
   add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
       COMMAND ${CMAKE_COMMAND} -E copy_if_different
       "$<TARGET_FILE:${PROJECT_NAME}>"
       "${REAPER_PLUGIN_DIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
       COMMENT "Deploying ${PROJECT_NAME} to REAPER UserPlugins..."
   )
endif()

# --- STATIC LINKING (Run on any machine) ---
if(MSVC)
    # Replace /MD (Dynamic) with /MT (Static)
    add_compile_options(
        $<$<CONFIG:Release>:/MT>
        $<$<CONFIG:Debug>:/MTd>
    )
endif()
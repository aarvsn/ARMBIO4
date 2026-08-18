cmake_minimum_required(VERSION 3.22.1)

project(bio4bridge C CXX)

# --- Build the bridge shared library ---
# This library is loaded by Bio4NativeBridge.java via System.loadLibrary("bio4bridge")
# It provides the JNI bridge between Java and the game's native code.

add_library(bio4bridge SHARED
    bio4_bridge.c
    patch.c
    video_player.c
)

# Include directories
target_include_directories(bio4bridge PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link against Android libraries
find_library(log-lib log)
find_library(dl-lib dl)
find_library(android-lib android)

target_link_libraries(bio4bridge
    ${log-lib}
    ${dl-lib}
    ${android-lib}
)

# Compiler flags
target_compile_options(bio4bridge PRIVATE
    -Wall
    -Wextra
    -Wno-deprecated
    -fvisibility=hidden
)

# Set C standard
set_target_properties(bio4bridge PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# --- Debug options ---
if(DEBUG_LOG)
    target_compile_definitions(bio4bridge PRIVATE DEBUG_LOG=1)
endif()

# --- Export JNI symbols ---
# Ensure JNI_OnLoad is visible (not hidden)
set_target_properties(bio4bridge PROPERTIES
    LINK_FLAGS "-Wl,--export-dynamic"
)

cmake_minimum_required(VERSION 3.20)
project(ImGuiLoaderBase LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows-spezifische Einstellungen
if(MSVC)
    # MultiByte Character Set
    add_compile_definitions(_MBCS)
    
    # SDL Checks und Conformance Mode
    add_compile_options(/W3 /sdl /permissive-)
    
    # Release Optimierungen
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Oi /GL /Gy")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG /OPT:REF /OPT:ICF")
endif()

# Quelldateien
set(SOURCES
    # Source
    src/Main.cpp
    src/ui/custom_ui.cpp
    src/ui/ui.cc
    
    # Anti-RE
    antiRe/anti_debug.cpp
    
    # ImGui
    import/lib/imgui/imgui.cpp
    import/lib/imgui/imgui_demo.cpp
    import/lib/imgui/imgui_draw.cpp
    import/lib/imgui/imgui_impl_dx9.cpp
    import/lib/imgui/imgui_impl_win32.cpp
    import/lib/imgui/imgui_tables.cpp
    import/lib/imgui/imgui_widgets.cpp
)

set(HEADERS
    # Import
    import/Main.h
    import/defines.hpp
    import/globals.hh
    
    # UI
    src/ui/custom_ui.hpp
    src/ui/ui.hh
    
    # Anti-RE
    antiRe/anti_debug.h
    antiRe/lazy.h
    antiRe/protection.h
    
    # KeyAuth
    import/lib/keyauth/auth.hpp
    import/lib/keyauth/json.hpp
    import/lib/keyauth/skStr.h
    import/lib/keyauth/utils.hpp
    
    # ImGui
    import/lib/imgui/imconfig.h
    import/lib/imgui/imgui.h
    import/lib/imgui/imgui_impl_dx9.h
    import/lib/imgui/imgui_impl_win32.h
    import/lib/imgui/imgui_internal.h
    import/lib/imgui/imstb_rectpack.h
    import/lib/imgui/imstb_textedit.h
    import/lib/imgui/imstb_truetype.h
)

# Executable erstellen (WIN32 für Windows Subsystem)
add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS})

# Include-Verzeichnisse
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/crypto
    ${CMAKE_CURRENT_SOURCE_DIR}/import
    ${CMAKE_CURRENT_SOURCE_DIR}/import/lib/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/import/lib/keyauth
    ${CMAKE_CURRENT_SOURCE_DIR}/antiRe
)

# DirectX 9 und Windows Libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    d3d9
    wininet
    ws2_32
    crypt32
    # MSVC Runtime Libraries für KeyAuth Kompatibilität
    ucrt
    vcruntime
    msvcrt
)

# ATL Library und MSVC Runtime für KeyAuth (MSVC-spezifisch)
if(MSVC)
    # Finde den neuesten MSVC Tools-Pfad mit ATL
    file(GLOB MSVC_VERSIONS "C:/Program Files/Microsoft Visual Studio/2022/*/VC/Tools/MSVC/*")
    foreach(MSVC_PATH ${MSVC_VERSIONS})
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(POTENTIAL_ATL_DIR "${MSVC_PATH}/atlmfc/lib/x64")
            set(POTENTIAL_LIB_DIR "${MSVC_PATH}/lib/x64")
        else()
            set(POTENTIAL_ATL_DIR "${MSVC_PATH}/atlmfc/lib/x86")
            set(POTENTIAL_LIB_DIR "${MSVC_PATH}/lib/x86")
        endif()
        
        if(EXISTS "${POTENTIAL_ATL_DIR}/atls.lib")
            set(ATL_LIB_DIR "${POTENTIAL_ATL_DIR}")
            set(MSVC_LIB_DIR "${POTENTIAL_LIB_DIR}")
        endif()
    endforeach()
    
    if(ATL_LIB_DIR)
        message(STATUS "Found ATL at: ${ATL_LIB_DIR}")
        message(STATUS "Using MSVC libs from: ${MSVC_LIB_DIR}")
        target_link_directories(${PROJECT_NAME} PRIVATE ${ATL_LIB_DIR} ${MSVC_LIB_DIR})
    else()
        message(WARNING "ATL not found - KeyAuth may not link correctly")
    endif()
endif()

# KeyAuth Library (x64) - nur Release
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/import/lib/keyauth/library_x64.lib
    )
    
    # KeyAuth wurde mit Release-Runtime kompiliert
    # Erzwinge Release Runtime für Debug, um Linker-Fehler zu vermeiden
    set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
endif()

# Request Administrator bei Release x64
if(CMAKE_SIZEOF_VOID_P EQUAL 8 AND CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\""
    )
endif()

# Output-Verzeichnis
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Fonts-Verzeichnis kopieren
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/import/lib/fonts
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/fonts
)

# Encryptor Tool
add_executable(Encryptor tools/Encryptor.cpp)
set_target_properties(Encryptor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

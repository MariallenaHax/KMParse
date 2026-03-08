cmake_minimum_required(VERSION 3.20)
project(KMParse LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

file(GLOB SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gctoolsplusplus/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/glad/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nw4r/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/egg/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kmp/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/discord/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/KMParseCode/*.cpp
)

list(FILTER SRC_FILES EXCLUDE REGEX "/\\._[^/]*$")

# Windows
if (WIN32)
    set(RESOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/KMParse.rc)
    add_executable(KMParse WIN32 ${SRC_FILES} ${RESOURCE_FILES})
        find_file(DISCORD_GAMESDK_DLL
        NAMES discord_game_sdk.dll
        PATHS ${CMAKE_CURRENT_SOURCE_DIR}/
        NO_DEFAULT_PATH
    )

    find_file(DISCORD_GAMESDK_LIB
        NAMES discord_game_sdk.dll.lib
        PATHS ${CMAKE_CURRENT_SOURCE_DIR}/lib/
        NO_DEFAULT_PATH
    )

    add_custom_command(TARGET KMParse POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            ${DISCORD_GAMESDK_DLL}
            $<TARGET_FILE_DIR:KMParse>
    )
    target_link_libraries(KMParse PRIVATE OpenGL::GL glfw ${DISCORD_GAMESDK_LIB})
    
    if (MSVC)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /GL /Gy /GS- /DNDEBUG /MT")
        set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "None" CACHE STRING "" FORCE)
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/INCREMENTAL:NO ${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
        string(REPLACE "/Zi" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    endif()

# macOS
elseif(APPLE)
    set(MAC_RES 
        "${CMAKE_CURRENT_SOURCE_DIR}/funkey_kong.icns"
        "${CMAKE_CURRENT_SOURCE_DIR}/font.brfnt"
    )
    set(MAC_RES_STRINGS
        "${CMAKE_CURRENT_SOURCE_DIR}/en.lproj/InfoPlist.strings"
    )

    add_executable(KMParse MACOSX_BUNDLE ${SRC_FILES} ${MAC_RES} ${MAC_RES_STRINGS})

    find_file(DISCORD_GAMESDK_BUNDLE
        NAMES discord_game_sdk.dylib
        PATHS ${CMAKE_CURRENT_SOURCE_DIR}/lib/
        NO_DEFAULT_PATH
    )

    target_link_libraries(KMParse PRIVATE 
        OpenGL::GL 
        glfw 
        "-framework AppKit" 
        "-framework Foundation" 
        ${DISCORD_GAMESDK_BUNDLE}
    )

    set_source_files_properties(KMParseCode/Engine.cpp KMParseCode/RenderContext.cpp PROPERTIES
        COMPILE_FLAGS "-ObjC++"
    )

    set_source_files_properties(${MAC_RES} PROPERTIES 
        MACOSX_PACKAGE_LOCATION "Resources"
    )
    set_source_files_properties(${MAC_RES_STRINGS} PROPERTIES 
        MACOSX_PACKAGE_LOCATION "Resources/en.lproj"
    )

    add_custom_command(TARGET KMParse POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_BUNDLE_CONTENT_DIR:KMParse>/Frameworks
        COMMAND ${CMAKE_COMMAND} -E copy
            ${DISCORD_GAMESDK_BUNDLE}
            $<TARGET_BUNDLE_CONTENT_DIR:KMParse>/Frameworks/
    )

    set_target_properties(KMParse PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
        MACOSX_BUNDLE_ICON_FILE "funkey_kong.icns"
        INSTALL_RPATH "@executable_path/../Frameworks"
        BUILD_WITH_INSTALL_RPATH TRUE
    )
# Linux
else()
    add_executable(KMParse ${SRC_FILES})
    target_link_libraries(KMParse PRIVATE OpenGL::GL glfw)
endif()
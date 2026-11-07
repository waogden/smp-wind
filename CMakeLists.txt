cmake_minimum_required(VERSION 3.22)

project(
	smp-wind
	VERSION 2.4.0
	LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)

if("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")
	message(FATAL_ERROR "in-source builds are not allowed")
endif()

# Must match how FSMP is built: we touch its Bullet objects through inline Bullet code.
add_compile_definitions(BT_USE_SSE_IN_API NOMINMAX _SILENCE_AMP_DEPRECATION_WARNINGS)

# CommonLibSSE-NG (alandtse/CommonLibVR, branch ng) -- same library and commit FSMP builds against.
# SKSE exports are declared by hand in main.cpp (see the note there about the Address Library v5 flag).
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(extern/CommonLibSSE CommonLibSSE EXCLUDE_FROM_ALL)

find_package(Bullet CONFIG REQUIRED)

set(SOURCES
	smp-wind/Config.cpp
	smp-wind/Config.h
	smp-wind/main.cpp
	smp-wind/papyrus.cpp
	smp-wind/PluginAPI.h
	smp-wind/PluginHelper.h
	smp-wind/Timer.h
	smp-wind/Wind.cpp
	smp-wind/Wind.h)

add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE CommonLibSSE::CommonLibSSE)

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "SMP Wind")

target_precompile_headers(${PROJECT_NAME} PRIVATE smp-wind/pch.h)
target_include_directories(${PROJECT_NAME} PRIVATE smp-wind ${BULLET_INCLUDE_DIRS})

if(MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE /utf-8 /permissive- /Zc:preprocessor /W3 /Zi)
	target_link_options(${PROJECT_NAME} PRIVATE
		"$<$<CONFIG:Release>:/INCREMENTAL:NO;/OPT:REF;/OPT:ICF;/DEBUG:FULL>")
endif()

# Optional: copy the DLL straight into a mod folder, e.g. -DOUTPUT_DIR="C:/MO2/mods/SMP Wind"
set(OUTPUT_DIR "" CACHE PATH "If set, copy the built DLL to <OUTPUT_DIR>/SKSE/Plugins")
if(OUTPUT_DIR)
	add_custom_command(
		TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory "${OUTPUT_DIR}/SKSE/Plugins"
		COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${PROJECT_NAME}>" "${OUTPUT_DIR}/SKSE/Plugins/")
endif()

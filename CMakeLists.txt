cmake_minimum_required(VERSION 2.6)
project(Collaborative-Robby)

set(Robby_VERSION_MAJOR 0)
set(Robby_VERSION_MINOR 1)

set(CORE_SOURCE_PROJECT_DIR "${PROJECT_SOURCE_DIR}/src")
set(PLUGINS_SOURCE_DIR "${CORE_SOURCE_PROJECT_DIR}/plugins")

if (DEFINED ROBBY_DEBUG AND ROBBY_DEBUG)
	MESSAGE(STATUS "Compiling with debug flag.")
	ADD_DEFINITIONS(-D DEBUG)
else()
	MESSAGE(STATUS "Compiling without debug flag.")
	REMOVE_DEFINITIONS(-D DEBUG)
endif()

include_directories("${CORE_SOURCE_PROJECT_DIR}/include")

file(GLOB Plugins
     "${PLUGINS_SOURCE_DIR}/*"
)

foreach(PLUG ${Plugins})
	string(REGEX REPLACE "${PLUGINS_SOURCE_DIR}/" "" OUTNAME ${PLUG})
	string(REGEX REPLACE ".c$" "" OUTNAME ${OUTNAME})
	file(GLOB CurrentPlugSources
		"${PLUG}"
		"${CORE_SOURCE_PROJECT_DIR}/modules.c"
	)
	add_library(${OUTNAME} SHARED ${CurrentPlugSources})
endforeach(PLUG)

file(GLOB EngineSources
     "${CORE_SOURCE_PROJECT_DIR}/robby.c"
)

add_executable(robby-engine ${EngineSources})
target_link_libraries(robby-engine dl)

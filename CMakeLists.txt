cmake_minimum_required(VERSION 2.6)
project(Collaborative-Robby)

set(Robby_VERSION_MAJOR 0)
set(Robby_VERSION_MINOR 1)

set(CORE_SOURCE_PROJECT_DIR "${PROJECT_SOURCE_DIR}/src")
set(PLUGINS_SOURCE_DIR "${CORE_SOURCE_PROJECT_DIR}/plugins")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wconversion -Wall -Wextra -Wno-unused-parameter")

if (DEFINED DEBUG_GENOMES AND DEBUG_GENOMES)
    MESSAGE(STATUS "Compiling with debug genomes")
    ADD_DEFINITIONS(-DDEBUG_GENOMES)
else()
    MESSAGE(STATUS "Compiling without debug genomes")
    REMOVE_DEFINITIONS(-DDEBUG_GENOMES)
endif()

if (DEFINED DEBUG_MSG AND DEBUG_MSG)
    MESSAGE(STATUS "Compiling with debug messages")
    ADD_DEFINITIONS(-DDEBUG_MSG)
else()
    MESSAGE(STATUS "Compiling without debug messages")
    REMOVE_DEFINITIONS(-DDEBUG_MSG)
endif()

if (DEFINED DEBUG_SPECIES AND DEBUG_SPECIES)
    MESSAGE(STATUS "Compiling with debug species")
    ADD_DEFINITIONS(-DDEBUG_SPECIES)
else()
    MESSAGE(STATUS "Compiling without debug species")
    REMOVE_DEFINITIONS(-DDEBUG_SPECIES)
endif()

if (DEFINED DEBUG_MOVE AND DEBUG_MOVE)
    MESSAGE(STATUS "Compiling with debug move")
    ADD_DEFINITIONS(-DDEBUG_MOVE)
else()
    MESSAGE(STATUS "Compiling without debug move")
    REMOVE_DEFINITIONS(-DDEBUG_MOVE)
endif()



if (DEFINED KNOWN_MAP AND KNOWN_MAP)
	MESSAGE(STATUS "Compiling with known map.")
	ADD_DEFINITIONS(-DKNOWN_MAP)
	if (DEFINED DEBUG_KNOWN_MAP AND DEBUG_KNOWN_MAP)
		MESSAGE(STATUS "Compiling with debug known map.")
		ADD_DEFINITIONS(-DDEBUG_KNOWN_MAP)
	else()
		MESSAGE(STATUS "Compiling without debug known map.")
		REMOVE_DEFINITIONS(-DDEBUG_KNOWN_MAP)
	endif()
else()
	MESSAGE(STATUS "Compiling without known map.")
	REMOVE_DEFINITIONS(-DKNOWN_MAP)
endif()

include_directories("${CORE_SOURCE_PROJECT_DIR}/include")

file(GLOB Plugins
     "${PLUGINS_SOURCE_DIR}/*"
)

foreach(PLUG_DIR ${Plugins})
	string(REGEX REPLACE "${PLUGINS_SOURCE_DIR}/" "" OUTNAME "${PLUG_DIR}")
	file(GLOB CurrentPlugSources
		"${PLUG_DIR}/*"
		"${CORE_SOURCE_PROJECT_DIR}/modules.cpp"
	)
	add_library(${OUTNAME} SHARED ${CurrentPlugSources})
	target_link_libraries(${OUTNAME} m)
endforeach(PLUG_DIR)

file(GLOB EngineSources
     "${CORE_SOURCE_PROJECT_DIR}/robby.cpp"
)

add_executable(robby-engine ${EngineSources})
target_link_libraries(robby-engine dl)

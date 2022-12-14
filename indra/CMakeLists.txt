# -*- cmake -*-

# cmake_minimum_required should appear before any
# other commands to guarantee full compatibility
# with the version specified
## 3.8 added VS_DEBUGGER_WORKING_DIRECTORY support
cmake_minimum_required(VERSION 3.8.0 FATAL_ERROR)

# As of XCode 12 this no longer defaults sensibly - adopt the fix implemented in CMake 3.18.1
set(CMAKE_OSX_ARCHITECTURES $(NATIVE_ARCH_ACTUAL) CACHE STRING "Set arch to actual" FORCE)

set(ROOT_PROJECT_NAME "Kokua" CACHE STRING
    "The root project/makefile/solution name. Defaults to Kokua.")
project(${ROOT_PROJECT_NAME})

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# this could change the VIEWER_CHANNEL definition - make sure it stays as the first thing executed!
if (DEFINED RLV_ALWAYS_ON)
	if (RLV_ALWAYS_ON)
		set(RLVALWAYSON 1)
		string(REPLACE RLV FTRLV NEW_CHANNEL ${VIEWER_CHANNEL})
		set(VIEWER_CHANNEL ${NEW_CHANNEL} CACHE STRING "Viewer channel" FORCE)
		message("-- RLV will be always on")
	else (RLV_ALWAYS_ON)
	  set(RLVALWAYSON 0)
	  message("-- RLV will be user-switchable")
	endif (RLV_ALWAYS_ON)
else (DEFINED RLV_ALWAYS_ON)
  set(RLVALWAYSON 0)
  message("-- RLV option -DRLV_ALWAYS_ON not used, defaulting to user-switchable")
endif (DEFINED RLV_ALWAYS_ON)

# fmodex and fmodstudio are mutually exclusive, make sure we only have one
# give fmodex priority if both are defined
if (FMODEX)
set (FMODSTUDIO OFF CACHE STRING "fmodstudio" FORCE)
message("-- Sound system: FMODex")
endif (FMODEX)
if (FMODSTUDIO)
set (FMODEX OFF CACHE STRING "fmodex" FORCE)
message("-- Sound system: FMODstudio")
endif (FMODSTUDIO)

include(Variables)
include(BuildVersion)

set(LEGACY_STDIO_LIBS)
if (WINDOWS)
      set(LEGACY_STDIO_LIBS legacy_stdio_definitions)
endif (WINDOWS)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Build type.  One of: Debug Release RelWithDebInfo" FORCE)
endif (NOT CMAKE_BUILD_TYPE)
if (HAVOK_TPV)
  add_definitions(-DHAVOK_TPV=1)
  message("-- Compiling with Havok libraries")
endif (HAVOK_TPV)
#</FS:AW optional opensim support>
#<FS:Ansariel> Support for test builds
option(TESTBUILD "Generating test build" OFF)
if(TESTBUILD AND TESTBUILDPERIOD)
  add_definitions(-DTESTBUILD=1 -DTESTBUILDPERIOD=${TESTBUILDPERIOD})
  message("creating test build version; test period: ${TESTBUILDPERIOD} days")
endif(TESTBUILD AND TESTBUILDPERIOD)
#</FS:Ansariel>


add_subdirectory(cmake)

add_subdirectory(${LIBS_OPEN_PREFIX}llaudio)
add_subdirectory(${LIBS_OPEN_PREFIX}llappearance)
add_subdirectory(${LIBS_OPEN_PREFIX}llcharacter)
add_subdirectory(${LIBS_OPEN_PREFIX}llcommon)
add_subdirectory(${LIBS_OPEN_PREFIX}llcorehttp)
add_subdirectory(${LIBS_OPEN_PREFIX}llimage)
add_subdirectory(${LIBS_OPEN_PREFIX}llkdu)
add_subdirectory(${LIBS_OPEN_PREFIX}llimagej2coj)
add_subdirectory(${LIBS_OPEN_PREFIX}llinventory)
add_subdirectory(${LIBS_OPEN_PREFIX}llmath)
add_subdirectory(${LIBS_OPEN_PREFIX}llmeshoptimizer)
add_subdirectory(${LIBS_OPEN_PREFIX}llmessage)
add_subdirectory(${LIBS_OPEN_PREFIX}llprimitive)
add_subdirectory(${LIBS_OPEN_PREFIX}llrender)
add_subdirectory(${LIBS_OPEN_PREFIX}llfilesystem)
add_subdirectory(${LIBS_OPEN_PREFIX}llwindow)
add_subdirectory(${LIBS_OPEN_PREFIX}llxml)

if (WINDOWS AND EXISTS ${LIBS_CLOSED_DIR}copy_win_scripts)
  add_subdirectory(${LIBS_CLOSED_PREFIX}copy_win_scripts)
endif (WINDOWS AND EXISTS ${LIBS_CLOSED_DIR}copy_win_scripts)

add_custom_target(viewer)

if (NOT USE_BUGSPLAT)
add_subdirectory(${LIBS_OPEN_PREFIX}llcrashlogger)
endif (NOT USE_BUGSPLAT)

add_subdirectory(${LIBS_OPEN_PREFIX}llplugin)
add_subdirectory(${LIBS_OPEN_PREFIX}llui)
add_subdirectory(${LIBS_OPEN_PREFIX}viewer_components)

# Legacy C++ tests. Build always, run if LL_TESTS is true.
add_subdirectory(${VIEWER_PREFIX}test)

if (ENABLE_MEDIA_PLUGINS)
# viewer media plugins
add_subdirectory(${LIBS_OPEN_PREFIX}media_plugins)
endif (ENABLE_MEDIA_PLUGINS)

if (LINUX)
  if (INSTALL_PROPRIETARY)
      include(LLAppearanceUtility)
      add_subdirectory(${LLAPPEARANCEUTILITY_SRC_DIR} ${LLAPPEARANCEUTILITY_BIN_DIR})
  endif (INSTALL_PROPRIETARY)
  add_dependencies(viewer linux-crash-logger-strip-target)
elseif (WINDOWS)
  # cmake EXISTS requires an absolute path, see indra/cmake/Variables.cmake
  if (EXISTS ${VIEWER_DIR}win_setup)
    add_subdirectory(${VIEWER_DIR}win_setup)
  endif (EXISTS ${VIEWER_DIR}win_setup)
endif (LINUX)

if (WINDOWS)
    # cmake EXISTS requires an absolute path, see indra/cmake/Variables.cmake
    if (EXISTS ${VIEWER_DIR}win_setup)
      add_subdirectory(${VIEWER_DIR}win_setup)
    endif (EXISTS ${VIEWER_DIR}win_setup)
endif (WINDOWS)

# sets the 'startup project' for debugging from visual studio.
set_property(
    DIRECTORY ${VIEWER_PREFIX}
    PROPERTY VS_STARTUP_PROJECT kokua-bin
    )

if (USE_BUGSPLAT)
    if (BUGSPLAT_DB)
        message(STATUS "Building with BugSplat; database '${BUGSPLAT_DB}'")
    else (BUGSPLAT_DB)
        message(WARNING "Building with BugSplat, but no database name set (BUGSPLAT_DB)")
    endif (BUGSPLAT_DB)
else (USE_BUGSPLAT)
    message(STATUS "Not building with BugSplat")
endif (USE_BUGSPLAT)

add_subdirectory(${VIEWER_PREFIX}newview)
  add_dependencies(viewer kokua-bin)

add_subdirectory(${VIEWER_PREFIX}doxygen EXCLUDE_FROM_ALL)

if (LL_TESTS)
  # Define after the custom targets are created so
  # individual apps can add themselves as dependencies
  add_subdirectory(${INTEGRATION_TESTS_PREFIX}integration_tests)
endif (LL_TESTS)


cmake_minimum_required (VERSION 3.16.0 FATAL_ERROR)

#
# Set for each plugin
#
file (READ "${CMAKE_CURRENT_LIST_DIR}/VERSION" PLUGIN_VERSION)
set (PLUGIN_NAME Wavetable)
set (BUNDLE_ID com.socalabs.wavetable)
set (AU_ID WavetableAU)
set (LV2_URI "https://socalabs.com/wavetable/")
set (PLUGIN_CODE Wave)

set (CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set (CMAKE_SUPPRESS_REGENERATION true)
set (CMAKE_SKIP_INSTALL_RULES YES)
set (CMAKE_POSITION_INDEPENDENT_CODE ON)
set_property (GLOBAL PROPERTY DEBUG_CONFIGURATIONS "Debug")

set (CMAKE_C_FLAGS_DEVELOPMENT ${CMAKE_C_FLAGS_RELEASE})
set (CMAKE_CXX_FLAGS_DEVELOPMENT ${CMAKE_CXX_FLAGS_RELEASE})

project (${PLUGIN_NAME} VERSION ${PLUGIN_VERSION} LANGUAGES CXX C HOMEPAGE_URL "https://socalabs.com/")

include (CMakeDependentOption)

set_property (DIRECTORY APPEND PROPERTY LABELS ${PLUGIN_NAME})

set_property (DIRECTORY APPEND PROPERTY LABELS SocaLabs)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PLUGIN_NAME}_Standalone)

set (CMAKE_OSX_DEPLOYMENT_TARGET 10.11)

set (CMAKE_EXPORT_COMPILE_COMMANDS ON)
set (CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION ON)
set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_EXTENSIONS OFF)
set (CMAKE_OBJCXX_STANDARD 20)
set (CMAKE_OBJCXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_VISIBILITY_PRESET hidden)
set (CMAKE_VISIBILITY_INLINES_HIDDEN ON)
set (CMAKE_MINSIZEREL_POSTFIX -rm)
set (CMAKE_RELWITHDEBINFO_POSTFIX -rd)
set (CMAKE_OPTIMIZE_DEPENDENCIES OFF)

set (BUILD_SHARED_LIBS OFF)

if(APPLE)
	set (CMAKE_OSX_ARCHITECTURES arm64 x86_64)
endif()

if (WIN32)
	set (FORMATS Standalone VST VST3 LV2)
else()
	set (FORMATS Standalone VST VST3 AU LV2)
endif()

set_property (GLOBAL PROPERTY USE_FOLDERS YES)
set_property (GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER utility)
set_property (GLOBAL PROPERTY REPORT_UNDEFINED_PROPERTIES "${CMAKE_BINARY_DIR}/undefined_properties.log")
set_property (GLOBAL PROPERTY JUCE_COPY_PLUGIN_AFTER_BUILD YES)

set_property (DIRECTORY APPEND PROPERTY LABELS External)

# JUCE

set (JUCE_MODULES_ONLY OFF)
set (JUCE_ENABLE_MODULE_SOURCE_GROUPS ON)
set (JUCE_BUILD_EXTRAS OFF)
set (JUCE_BUILD_EXAMPLES OFF)

add_subdirectory (modules/juce)

set_property (DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/modules/juce" APPEND PROPERTY LABELS JUCE)

#

# Gin modules

foreach(module_name IN ITEMS gin gin_dsp gin_graphics gin_gui gin_metadata gin_network gin_plugin gin_simd)
	juce_add_module (
		"${CMAKE_CURRENT_LIST_DIR}/modules/gin/modules/${module_name}"
		)

	set_property (TARGET "${module_name}" APPEND PROPERTY LABELS Gin)
endforeach()

# melatonin
juce_add_module ("${CMAKE_CURRENT_LIST_DIR}/modules/melatonin_inspector")
set_property (TARGET melatonin_inspector APPEND PROPERTY LABELS melatonin)

# Binary Data

set_property (DIRECTORY APPEND PROPERTY LABELS Assets)

file (GLOB_RECURSE binary_files CONFIGURE_DEPENDS
	"./plugin/Resources/layout.json"
	"./plugin/Resources/WavetablesFLAC/*.wt2048"
	"./plugin/Resources/Presets/*.xml")

juce_add_binary_data (${PLUGIN_NAME}_Assets SOURCES ${binary_files})

set_target_properties(${PLUGIN_NAME}_Assets PROPERTIES UNITY_BUILD ON UNITY_BUILD_MODE BATCH UNITY_BUILD_BATCH_SIZE 10)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

juce_set_vst2_sdk_path (${CMAKE_SOURCE_DIR}/modules/plugin_sdk/vstsdk2.4)

juce_add_plugin (${PLUGIN_NAME}
				 PRODUCT_NAME ${PLUGIN_NAME}
				 VERSION ${PLUGIN_VERSION}
				 COMPANY_NAME SocaLabs
				 COMPANY_WEBSITE "https://socalabs.com/"
				 BUNDLE_ID ${BUNDLE_ID}
				 FORMATS ${FORMATS}
				 PLUGIN_MANUFACTURER_CODE Soca
				 PLUGIN_CODE ${PLUGIN_CODE}
				 IS_SYNTH ON
				 NEEDS_MIDI_INPUT ON
				 EDITOR_WANTS_KEYBOARD_FOCUS ON
				 VST2_CATEGORY kPlugCategSynth
				 VST3_CATEGORIES Instrument
				 AU_MAIN_TYPE kAudioUnitType_MusicDevice
				 AU_EXPORT_PREFIX ${AU_ID}
				 AU_SANDBOX_SAFE FALSE
				 LV2URI ${LV2_URI})

file (GLOB_RECURSE source_files CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/plugin/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugin/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/plugin/*.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/plugin/*.h)

target_sources (${PLUGIN_NAME} PRIVATE ${source_files})
source_group (TREE ${CMAKE_CURRENT_SOURCE_DIR}/plugin PREFIX Source FILES ${source_files})

file (GLOB_RECURSE asset_files CONFIGURE_DEPENDS
	${CMAKE_CURRENT_SOURCE_DIR}/Assets/*)

target_sources (${PLUGIN_NAME} PRIVATE ${asset_files})
source_group (TREE ${CMAKE_CURRENT_SOURCE_DIR}/Assets PREFIX Assets FILES ${asset_files})

target_link_libraries (${PLUGIN_NAME} PRIVATE
						${PLUGIN_NAME}_Assets

						gin
						gin_dsp
						gin_graphics
						gin_gui
						gin_plugin
						gin_simd

						melatonin_inspector

						juce::juce_audio_basics
						juce::juce_audio_devices
						juce::juce_audio_formats
						juce::juce_audio_plugin_client
						juce::juce_audio_processors
						juce::juce_audio_utils
						juce::juce_core
						juce::juce_cryptography
						juce::juce_data_structures
						juce::juce_events
						juce::juce_graphics
						juce::juce_gui_basics
						juce::juce_gui_extra

    					juce::juce_recommended_config_flags
					)

target_include_directories (${PLUGIN_NAME} PRIVATE modules/fmt/include)
target_include_directories (${PLUGIN_NAME} PRIVATE modules/ASIO/common)
target_include_directories (${PLUGIN_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source/Definitions)

juce_generate_juce_header (${PLUGIN_NAME})

target_compile_definitions (${PLUGIN_NAME} PRIVATE
								JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1
								JUCE_COREGRAPHICS_DRAW_ASYNC=1
								JUCE_MODAL_LOOPS_PERMITTED=0
								JUCE_WEB_BROWSER=0
								JUCE_USE_FLAC=1
								JUCE_USE_CURL=1
								JUCE_USE_MP3AUDIOFORMAT=0
								JUCE_USE_LAME_AUDIO_FORMAT=0
								JUCE_USE_WINDOWS_MEDIA_FORMAT=0
								JUCE_SILENCE_XCODE_15_LINKER_WARNING=1
								_CRT_SECURE_NO_WARNINGS
							)

if (APPLE)
	set_target_properties("juce_vst3_helper" PROPERTIES XCODE_ATTRIBUTE_CLANG_LINK_OBJC_RUNTIME NO)

    foreach(t ${FORMATS} "Assets" "All" "")
        set(tgt ${CMAKE_PROJECT_NAME})
        if (NOT t STREQUAL "")
            set(tgt ${tgt}_${t})
        endif()
        if (TARGET ${tgt})
			set_target_properties(${tgt} PROPERTIES
				XCODE_ATTRIBUTE_CLANG_LINK_OBJC_RUNTIME NO
				#XCODE_ATTRIBUTE_DEPLOYMENT_POSTPROCESSING[variant=Release] YES
				XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH[variant=Debug] "YES"
				)
			if (NOT t STREQUAL "All")
				target_compile_options(${tgt} PRIVATE
					-Wall -Wstrict-aliasing -Wunused-parameter -Wconditional-uninitialized -Woverloaded-virtual -Wreorder -Wconstant-conversion -Wbool-conversion -Wextra-semi
					-Wunreachable-code -Winconsistent-missing-destructor-override -Wshift-sign-overflow -Wnullable-to-nonnull-conversion -Wuninitialized -Wno-missing-field-initializers
					-Wno-ignored-qualifiers -Wno-missing-braces -Wno-char-subscripts -Wno-unused-private-field -fno-aligned-allocation -Wunused-private-field -Wunreachable-code
					-Wenum-compare -Wshadow -Wfloat-conversion -Wshadow-uncaptured-local -Wshadow-field -Wsign-compare -Wdeprecated-this-capture -Wimplicit-float-conversion
					-ffast-math -fno-finite-math-only)
			endif()
       	endif()
    endforeach()
endif()

if (WIN32)
    foreach(t ${FORMATS} "Assets" "All" "")
        set(tgt ${CMAKE_PROJECT_NAME})
        if (NOT t STREQUAL "")
            set(tgt ${tgt}_${t})
        endif()
        if (TARGET ${tgt})
			set_property(TARGET ${tgt} APPEND_STRING PROPERTY LINK_FLAGS_DEBUG " /INCREMENTAL:NO")
			set_target_properties(${tgt} PROPERTIES LINK_FLAGS "/ignore:4099")
		endif()
	endforeach()
endif()

if(UNIX AND NOT APPLE)
	target_link_libraries (${PLUGIN_NAME} PRIVATE curl)
endif()


if(WIN32)
	set (dest "Program Files")
else()
	set (dest "Applications")
endif()

install (TARGETS ${PLUGIN_NAME} DESTINATION "${dest}")

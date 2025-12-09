cmake_minimum_required(VERSION 3.22)

# Project name and version
project(NovaTune VERSION 1.0.0)

# C++ Standard - we use C++17 for modern features
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optimization flags for Release builds
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -ffast-math")

# JUCE - The audio plugin framework
# You'll need to have JUCE installed or include it as a submodule
# Option 1: If JUCE is installed system-wide or via package manager
# find_package(JUCE CONFIG REQUIRED)

# Option 2: If JUCE is included as a git submodule (recommended)
add_subdirectory(JUCE)

# Determine which plugin formats to build based on platform
if(APPLE)
    set(PLUGIN_FORMATS AU VST3 Standalone)
elseif(WIN32)
    set(PLUGIN_FORMATS VST3 Standalone)
else()
    # Linux
    set(PLUGIN_FORMATS VST3 Standalone)
endif()

# Define the plugin target
juce_add_plugin(NovaTune
    # Plugin metadata
    COMPANY_NAME "NovaTune Audio"
    PLUGIN_MANUFACTURER_CODE Nova
    PLUGIN_CODE Nvtn
    BUNDLE_ID "com.novatune.NovaTune"
    
    # Plugin formats to build (platform-specific)
    FORMATS ${PLUGIN_FORMATS}
    
    # Plugin characteristics
    PRODUCT_NAME "NovaTune"
    
    # Plugin type - this is an effect that processes audio
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    
    # Copy plugin to system folders after build (handy for testing)
    COPY_PLUGIN_AFTER_BUILD TRUE
)

# Generate JUCE header
juce_generate_juce_header(NovaTune)

# Source files - all the C++ code we'll write
target_sources(NovaTune
    PRIVATE
        # Main plugin files
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/Utilities.cpp
        
        # DSP (Digital Signal Processing) modules
        Source/dsp/TunerEngine.cpp
        Source/dsp/PitchDetector.cpp
        Source/dsp/PitchMapper.cpp
        Source/dsp/LeadCorrection.cpp
        Source/dsp/HarmonyVoice.cpp
        Source/dsp/FormantProcessor.cpp
        Source/dsp/PitchShifter.cpp
)

# Preprocessor definitions
target_compile_definitions(NovaTune
    PUBLIC
        # JUCE plugin settings
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        
        # Enable SIMD (Single Instruction Multiple Data) for performance
        JUCE_DSP_USE_INTEL_MKL=0
        JUCE_DSP_USE_SHARED_FFTW=0
)

# Link against JUCE modules we need
target_link_libraries(NovaTune
    PRIVATE
        juce::juce_audio_utils      # Audio utilities
        juce::juce_audio_processors # Plugin framework
        juce::juce_dsp             # DSP utilities (FFT, filters, etc.)
        juce::juce_gui_extra       # Extended GUI components
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags      # Link-time optimization
        juce::juce_recommended_warning_flags
)

# Include directories
target_include_directories(NovaTune
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

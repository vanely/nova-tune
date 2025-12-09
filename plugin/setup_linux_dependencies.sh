#!/bin/bash
# Setup script for NovaTune Linux build dependencies
# Usage:
#   sudo bash setup_linux_dependencies.sh --all      # Install all dependencies
#   sudo bash setup_linux_dependencies.sh --xcursor   # Install only libxcursor-dev

echo "NovaTune - Linux Build Dependencies Setup"
echo "=========================================="
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo "Please run this script with sudo:"
    echo "  sudo bash setup_linux_dependencies.sh [--all|--xcursor]"
    exit 1
fi

# Parse command line arguments
INSTALL_ALL=false
INSTALL_XCURSOR=false

if [ "$1" == "--all" ]; then
    INSTALL_ALL=true
    echo "Installing all dependencies..."
elif [ "$1" == "--xcursor" ]; then
    INSTALL_XCURSOR=true
    echo "Installing libxcursor-dev only..."
else
    echo "Usage: sudo bash setup_linux_dependencies.sh [--all|--xcursor]"
    echo ""
    echo "Options:"
    echo "  --all      Install all required dependencies"
    echo "  --xcursor  Install only libxcursor-dev"
    exit 1
fi

echo "Updating package list..."
apt update

if [ "$INSTALL_XCURSOR" == true ]; then
    echo ""
    echo "Installing libxcursor-dev..."
    apt install -y libxcursor-dev
    echo ""
    echo "libxcursor-dev installed successfully!"
elif [ "$INSTALL_ALL" == true ]; then
    echo ""
    echo "Installing all JUCE dependencies for Linux..."
    apt install -y \
        libasound2-dev \
        libfreetype-dev \
        libfontconfig1-dev \
        libx11-dev \
        libxcomposite-dev \
        libxcursor-dev \
        libxext-dev \
        libxinerama-dev \
        libxrandr-dev \
        libxrender-dev \
        libglu1-mesa-dev \
        mesa-common-dev
    
    echo ""
    echo "All dependencies installed successfully!"
fi

echo ""
echo "You can now build NovaTune with:"
echo "  cd plugin/build"
echo "  cmake .. -DCMAKE_BUILD_TYPE=Release"
echo "  cmake --build . --config Release"


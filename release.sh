#!/bin/bash

# Exit on error
set -e

# Read version from file
VERSION=$(cat VERSION)
TAG="v$VERSION"

echo "Creating release for version $VERSION..."

# Function to detect OS
get_os() {
    case "$(uname -s)" in
        Linux*)     echo "linux";;
        MINGW*|MSYS*|CYGWIN*)    echo "windows";;
        *)          echo "unknown";;
    esac
}

OS=$(get_os)

# Build for Linux
build_linux() {
    echo "Building Linux version..."
    qmake
    make -j$(nproc)
    
    # Create release directory and copy binary
    mkdir -p release/linux
    cp db_manager release/linux/db_manager
}

# Build for Windows
build_windows() {
    echo "Building Windows version..."
    
    # Setup cross-compilation environment
    if [ "$OS" = "linux" ]; then
        export MXE_PATH=/usr/lib/mxe/usr/bin
        export PATH=$MXE_PATH:$PATH
        
        # Check if MXE is installed
        if ! command -v $MXE_PATH/x86_64-w64-mingw32.static-qmake-qt5 &> /dev/null; then
            echo "MXE (M cross environment) not found. Please install it first."
            echo "See: https://mxe.cc/"
            exit 1
        fi
        
        $MXE_PATH/x86_64-w64-mingw32.static-qmake-qt5
        $MXE_PATH/x86_64-w64-mingw32.static-make -j$(nproc)
        
        mkdir -p release/windows
        mv release/db_manager.exe release/windows/
        
        # Deploy Qt dependencies
        $MXE_PATH/x86_64-w64-mingw32.static-windeployqt.exe release/windows/db_manager.exe
    else
        qmake
        nmake
        
        mkdir -p release/windows
        mv release/db_manager.exe release/windows/
        windeployqt.exe release/windows/db_manager.exe
    fi
}

# Clean build artifacts
cleanup() {
    echo "Cleaning up..."
    make clean
    rm -rf release
    rm -f *.o moc_* ui_*
}

# Main build process
build_all() {
    cleanup
    
    # Build Linux version
    build_linux
    
    # Build Windows version
    # build_windows
}

# Create release notes
NOTES="Release $VERSION
"

# Build all versions
build_all

# Create a new release using gh
gh release create "$TAG" \
    "release/linux/db_manager" \
    --title "Release $VERSION" \
    --notes "$NOTES" \
    --prerelease=false

echo "Release $VERSION created successfully!"
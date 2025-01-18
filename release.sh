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

# Check and install appimagetool if needed
check_appimagetool() {
    if ! command -v appimagetool &> /dev/null; then
        echo "Installing appimagetool..."
        wget https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage -O appimagetool
        chmod +x appimagetool
        sudo mv appimagetool /usr/local/bin/
    fi
}

# Create AppDir structure with required files
create_appdir() {
    # Create basic structure
    mkdir -p AppDir/usr/bin
    mkdir -p AppDir/usr/share/applications
    mkdir -p AppDir/usr/share/icons/hicolor/256x256/apps

    # Copy binary
    cp db_manager AppDir/usr/bin/

    # Create .desktop file
    cat > AppDir/usr/share/applications/db_manager.desktop << EOF
[Desktop Entry]
Name=DB Manager
Exec=db_manager
Icon=db_manager
Type=Application
Categories=Development;
Comment=Database Management Tool
EOF

    # Create a simple icon if it doesn't exist
    if [ ! -f "icons/db_manager.png" ]; then
        # Create a temporary icon using convert (from ImageMagick)
        convert -size 256x256 xc:transparent -font DejaVu-Sans -pointsize 40 -gravity center -draw "text 0,0 'DB\nManager'" AppDir/usr/share/icons/hicolor/256x256/apps/db_manager.png
    else
        cp icons/db_manager.png AppDir/usr/share/icons/hicolor/256x256/apps/db_manager.png
    fi

    # Create symlinks
    ln -sf usr/share/applications/db_manager.desktop AppDir/db_manager.desktop
    ln -sf usr/share/icons/hicolor/256x256/apps/db_manager.png AppDir/db_manager.png
}

# Build for current platform
build_app() {
    echo "Building application..."
    qmake
    if [ "$OS" = "windows" ]; then
        nmake
        # Deploy Qt dependencies
        windeployqt.exe release/db_manager.exe
        # Create ZIP archive
        cd release
        zip -r ../db_manager-windows.zip *
        cd ..
    else
        make -j$(nproc)
        # Check for appimagetool
        check_appimagetool
        # Create AppDir with all required files
        create_appdir
        # Create AppImage
        appimagetool AppDir db_manager-linux.AppImage
    fi
}

# Build the application
build_app

# Create release notes
NOTES="Release $VERSION

## Changes
- Please edit this release note

## Downloads
- Windows: db_manager-windows.zip
- Linux: db_manager-linux.AppImage"

# Create a new release using gh
if [ "$OS" = "windows" ]; then
    gh release create "$TAG" db_manager-windows.zip --title "Release $VERSION" --notes "$NOTES"
else
    gh release create "$TAG" db_manager-linux.AppImage --title "Release $VERSION" --notes "$NOTES"
fi

echo "Release $VERSION created successfully!" 
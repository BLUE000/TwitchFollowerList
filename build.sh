#!/bin/bash

# Qt/Compiler paths
QT_PATH="C:/Qt/6.10.1/mingw_64"
MINGW_PATH="C:/Qt/Tools/mingw1310_64"
NINJA_PATH="C:/tmp/mingw64/bin"
CMAKE_PATH="C:/Qt/Tools/CMake_64/bin"

# Force MinGW compilers
export CC="$MINGW_PATH/bin/gcc.exe"
export CXX="$MINGW_PATH/bin/g++.exe"
export PATH="$QT_PATH/bin:$MINGW_PATH/bin:$NINJA_PATH:$CMAKE_PATH:$PATH"

DEBUG_EXE="bin/Debug/FollowerList.exe"
RELEASE_EXE="bin/Release/FollowerList.exe"

function show_menu() {
    DEBUG_EXISTS=false
    RELEASE_EXISTS=false
    [ -f "$DEBUG_EXE" ] && DEBUG_EXISTS=true
    [ -f "$RELEASE_EXE" ] && RELEASE_EXISTS=true

    echo "---------------------------"
    echo " FollowerList Build Menu"
    echo "---------------------------"
    echo "1) Debug Build   (d)"
    echo "2) Release Build (r)"
    
    if [ "$DEBUG_EXISTS" = true ]; then
        echo "3) Debug Run     (dr)"
    fi
    if [ "$RELEASE_EXISTS" = true ]; then
        echo "4) Release Run   (rr)"
    fi
    
    echo "q) Quit"
    echo "---------------------------"
    read -p "Choice: " choice
    case $choice in
        1|d|deb) ACTION="debug" ;;
        2|r|rel) ACTION="release" ;;
        3|dr) 
            if [ "$DEBUG_EXISTS" = true ]; then ACTION="run_debug"; else echo "Error: Debug EXE missing."; show_menu; fi
            ;;
        4|rr) 
            if [ "$RELEASE_EXISTS" = true ]; then ACTION="run_release"; else echo "Error: Release EXE missing."; show_menu; fi
            ;;
        q) exit 0 ;;
        *) echo "Invalid choice"; show_menu ;;
    esac
}

# Parse arguments
ACTION=""
if [ $# -eq 0 ]; then
    show_menu
else
    case $1 in
        d|deb) ACTION="debug" ;;
        r|rel) ACTION="release" ;;
        dr)    ACTION="run_debug" ;;
        rr)    ACTION="run_release" ;;
        run)
            D_OK=false; R_OK=false
            [ -f "$DEBUG_EXE" ] && D_OK=true
            [ -f "$RELEASE_EXE" ] && R_OK=true
            
            if [ "$D_OK" = true ] && [ "$R_OK" = true ]; then
                echo "Both versions found. Select to run:"
                echo "1) Debug"
                echo "2) Release"
                read -p "Choice [1-2]: " rchoice
                if [ "$rchoice" == "2" ]; then ACTION="run_release"; else ACTION="run_debug"; fi
            elif [ "$D_OK" = true ]; then
                ACTION="run_debug"
            elif [ "$R_OK" = true ]; then
                ACTION="run_release"
            else
                echo "Error: No executable found. Please build first (d/r)."
                exit 1
            fi
            ;;
        *) echo "Usage: ./build.sh [d|r|dr|rr|run]"; exit 1 ;;
    esac
fi

# Execution mode
if [[ "$ACTION" == run_* ]]; then
    TARGET_EXE=""
    if [ "$ACTION" == "run_debug" ]; then
        TARGET_EXE="$DEBUG_EXE"
    else
        TARGET_EXE="$RELEASE_EXE"
    fi
    
    if [ -f "$TARGET_EXE" ]; then
        echo "--- Starting App: $TARGET_EXE ---"
        "$TARGET_EXE" &
    else
        echo "Error: $TARGET_EXE not found."
        exit 1
    fi
    exit 0
fi

# Build mode
BUILD_TYPE="Debug"
[ "$ACTION" == "release" ] && BUILD_TYPE="Release"

echo "--- Building Target: $BUILD_TYPE ---"
BUILD_DIR="build_bash_${BUILD_TYPE}"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake -G "Ninja" \
    -DCMAKE_PREFIX_PATH="$QT_PATH" \
    -DCMAKE_MAKE_PROGRAM="$NINJA_PATH/ninja.exe" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_C_COMPILER="$CC" \
    -DCMAKE_CXX_COMPILER="$CXX" \
    ..

if [ $? -ne 0 ]; then
    echo "Error: CMake failed."
    exit 1
fi

ninja
if [ $? -ne 0 ]; then
    echo "Error: Build failed."
    exit 1
fi

EXE_PATH="../bin/${BUILD_TYPE}/FollowerList.exe"
echo "Deploying DLLs..."
"$QT_PATH/bin/windeployqt.exe" "$EXE_PATH"

echo "------------------------------------------------"
echo " Build Successful: $EXE_PATH"
echo "------------------------------------------------"

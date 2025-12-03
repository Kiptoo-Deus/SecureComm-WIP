#!/bin/bash

# ============================================================================
# CarrierBridge Android Build & Deploy Script
# ============================================================================
# This script builds the Android APK and deploys it to an emulator or device
# Usage: ./build_and_deploy.sh [--emulator|--device] [--test]
# ============================================================================

set -e

PROJECT_ROOT="/Users/joel/Documents/GitHub/CarrierBridge"
ANDROID_DIR="$PROJECT_ROOT/android"
GRADLEW="$ANDROID_DIR/gradlew"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Functions
print_header() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}========================================${NC}"
}

print_success() {
    echo -e "${GREEN}✅ $1${NC}"
}

print_error() {
    echo -e "${RED}❌ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠️  $1${NC}"
}

# Main script
print_header "CarrierBridge Android Build System"

# Parse arguments
BUILD_TYPE="debug"
DEPLOY_TARGET="none"
RUN_TEST="false"

for arg in "$@"; do
    case $arg in
        --emulator)
            DEPLOY_TARGET="emulator"
            ;;
        --device)
            DEPLOY_TARGET="device"
            ;;
        --test)
            RUN_TEST="true"
            ;;
        --release)
            BUILD_TYPE="release"
            ;;
    esac
done

echo ""
print_header "Step 1: Environment Check"
echo ""

# Check required tools
if ! command -v gradle &> /dev/null && [ ! -f "$GRADLEW" ]; then
    print_error "Gradle not found"
    exit 1
fi
print_success "Gradle found"

if [ -z "$ANDROID_HOME" ]; then
    export ANDROID_HOME="$HOME/Library/Android/sdk"
    print_warning "ANDROID_HOME not set, using: $ANDROID_HOME"
fi

if [ ! -d "$ANDROID_HOME" ]; then
    print_error "ANDROID_HOME directory not found: $ANDROID_HOME"
    exit 1
fi
print_success "Android SDK found at $ANDROID_HOME"

# Check if NDK is available
if [ ! -d "$ANDROID_HOME/ndk" ]; then
    print_error "Android NDK not found"
    echo "Install via Android Studio: Settings → SDK Manager → SDK Tools → NDK"
    exit 1
fi
print_success "Android NDK found"

echo ""
print_header "Step 2: Clean Build"
echo ""

cd "$ANDROID_DIR"
$GRADLEW clean

print_success "Clean completed"

echo ""
print_header "Step 3: Build APK ($BUILD_TYPE)"
echo ""

if [ "$BUILD_TYPE" = "debug" ]; then
    BUILD_TASK="assembleDebug"
    APK_PATH="app/build/outputs/apk/debug/app-debug.apk"
else
    BUILD_TASK="assembleRelease"
    APK_PATH="app/build/outputs/apk/release/app-release.apk"
fi

echo "Building with task: $BUILD_TASK"
$GRADLEW $BUILD_TASK

if [ -f "$ANDROID_DIR/$APK_PATH" ]; then
    APK_SIZE=$(ls -lh "$ANDROID_DIR/$APK_PATH" | awk '{print $5}')
    print_success "APK built successfully! Size: $APK_SIZE"
    echo "Location: $ANDROID_DIR/$APK_PATH"
else
    print_error "APK not found at expected location"
    exit 1
fi

echo ""
print_header "Step 4: Deployment"
echo ""

if [ "$DEPLOY_TARGET" != "none" ]; then
    
    # Check for connected devices/emulators
    if ! command -v adb &> /dev/null; then
        print_error "adb not found in PATH"
        echo "Try: export PATH=\$PATH:\$ANDROID_HOME/platform-tools"
        exit 1
    fi
    
    # List available devices
    DEVICES=$(adb devices -l | tail -n +2)
    if [ -z "$DEVICES" ] || [ "$DEVICES" = "" ]; then
        print_error "No devices or emulators found"
        echo "Start an emulator or connect a device"
        exit 1
    fi
    
    print_success "Connected devices:"
    adb devices -l | tail -n +2
    
    echo ""
    echo "Installing APK..."
    adb install -r "$ANDROID_DIR/$APK_PATH"
    
    if [ $? -eq 0 ]; then
        print_success "APK installed successfully"
    else
        print_error "Failed to install APK"
        exit 1
    fi
    
    echo ""
    echo "Launching app..."
    adb shell am start -n com.example.carrierbridge/.MainActivity
    
    sleep 2
    
    if [ "$RUN_TEST" = "true" ]; then
        echo ""
        print_header "Step 5: Testing"
        echo ""
        echo "Showing logcat (press Ctrl+C to stop)..."
        adb logcat -s "CarrierBridge*" -v threadtime
    fi
else
    echo ""
    print_warning "Deployment skipped (use --emulator or --device to deploy)"
fi

echo ""
print_header "Build Complete!"
echo ""
echo "APK Location: $ANDROID_DIR/$APK_PATH"
echo ""
echo "Next steps:"
echo "  1. Install on device: adb install -r $APK_PATH"
echo "  2. Launch app: adb shell am start -n com.example.carrierbridge/.MainActivity"
echo "  3. View logs: adb logcat -s CarrierBridge"
echo ""

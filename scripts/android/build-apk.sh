#!/usr/bin/env bash
# Packages the native Android build into a debug-signed APK without Gradle.
#
# Usage: build-apk.sh <build dir> <output apk>
#   <build dir>  CMake binary dir of the android-generalsmd-sdl3-bgfx preset
#
# Needs ANDROID_HOME (SDK with build-tools and platform 34) and a JDK on PATH.
set -euo pipefail

BUILD_DIR=$(cd "$1" && pwd)
OUT_APK=$(realpath -m "$2")
SDK=${ANDROID_HOME:-$LOCALAPPDATA/Android/Sdk}
BT=$(ls -d "$SDK"/build-tools/3[4-9].* | sort -V | tail -1)
PLATFORM_JAR=$SDK/platforms/android-34/android.jar
SDL_SRC=$BUILD_DIR/_deps/sdl3-src
KEYSTORE=${ANDROID_KEYSTORE:-$HOME/.android/debug.keystore}
WORK=$BUILD_DIR/apk-work
EXE=$(case "$(uname -s)" in MINGW*|MSYS*|CYGWIN*) echo .exe;; *) echo;; esac)
BAT=$(case "$(uname -s)" in MINGW*|MSYS*|CYGWIN*) echo .bat;; *) echo;; esac)

rm -rf "$WORK"
mkdir -p "$WORK/java" "$WORK/classes" "$WORK/dex" "$WORK/lib/arm64-v8a"

cat > "$WORK/AndroidManifest.xml" <<'XML'
<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
    package="com.generals.launcher"
    android:versionCode="1"
    android:versionName="1.0"
    android:installLocation="auto">
    <uses-feature android:glEsVersion="0x00020000" />
    <uses-feature android:name="android.hardware.touchscreen" android:required="false" />
    <uses-feature android:name="android.hardware.gamepad" android:required="false" />
    <uses-feature android:name="android.hardware.type.pc" android:required="false" />
    <uses-permission android:name="android.permission.VIBRATE" />
    <uses-permission android:name="android.permission.MANAGE_EXTERNAL_STORAGE" />
    <application android:label="Generals Zero Hour"
        android:allowBackup="false"
        android:enableOnBackInvokedCallback="false"
        android:hardwareAccelerated="true"
        android:extractNativeLibs="true">
        <activity android:name="com.generals.launcher.GameActivity"
            android:exported="true"
            android:screenOrientation="sensorLandscape"
            android:launchMode="singleTask"
            android:configChanges="layoutDirection|locale|orientation|uiMode|screenLayout|screenSize|smallestScreenSize|keyboard|keyboardHidden|navigation"
            android:preferMinimalPostProcessing="true">
            <intent-filter>
                <action android:name="android.intent.action.MAIN" />
                <category android:name="android.intent.category.LAUNCHER" />
            </intent-filter>
        </activity>
    </application>
</manifest>
XML

mkdir -p "$WORK/java/com/generals/launcher"
cat > "$WORK/java/com/generals/launcher/GameActivity.java" <<'JAVA'
package com.generals.launcher;

import org.libsdl.app.SDLActivity;

public class GameActivity extends SDLActivity {
    @Override
    protected String[] getLibraries() {
        return new String[] { "openal", "SDL3", "z_generals" };
    }
}
JAVA

cp -r "$SDL_SRC/android-project/app/src/main/java/org" "$WORK/java/"

"$BT/aapt2$EXE" link --manifest "$WORK/AndroidManifest.xml" -I "$PLATFORM_JAR" \
    --min-sdk-version 26 --target-sdk-version 34 -o "$WORK/base.apk"
javac -nowarn -g --release 11 -classpath "$PLATFORM_JAR" -d "$WORK/classes" \
    $(find "$WORK/java" -name '*.java') 2>&1 | grep -v '^Note\|warning' || true
"$BT/d8$BAT" --lib "$PLATFORM_JAR" --min-api 26 --output "$WORK/dex" $(find "$WORK/classes" -name '*.class')

cp "$BUILD_DIR/GeneralsMD/Code/Main/Release/libz_generals.so" "$WORK/lib/arm64-v8a/"
cp "$BUILD_DIR/_deps/sdl3-build/Release/libSDL3.so" "$WORK/lib/arm64-v8a/"
cp "$BUILD_DIR/_deps/openal_soft-build/Release/libopenal.so" "$WORK/lib/arm64-v8a/"

python - "$WORK" <<'PY'
import sys, zipfile, os
work = sys.argv[1]
with zipfile.ZipFile(os.path.join(work, 'base.apk'), 'a') as z:
    z.write(os.path.join(work, 'dex', 'classes.dex'), 'classes.dex', zipfile.ZIP_DEFLATED)
    for name in sorted(os.listdir(os.path.join(work, 'lib', 'arm64-v8a'))):
        z.write(os.path.join(work, 'lib', 'arm64-v8a', name), 'lib/arm64-v8a/' + name, zipfile.ZIP_DEFLATED)
PY

"$BT/zipalign$EXE" -p -f 4 "$WORK/base.apk" "$WORK/aligned.apk"
"$BT/apksigner$BAT" sign --ks "$KEYSTORE" --ks-pass pass:android --key-pass pass:android \
    --out "$OUT_APK" "$WORK/aligned.apk"
echo "Wrote $OUT_APK"

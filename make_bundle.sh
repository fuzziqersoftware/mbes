#!/bin/bash

set -e

EXECUTABLE_NAME=$1
BUNDLE_NAME=$2
ICON_FILE=$3

if [ -z "$EXECUTABLE_NAME" ] || [ -z "$BUNDLE_NAME" ]
then
	echo "usage: $0 executable_name bundle_identifier [icon_file_basename]"
	exit 1
fi

rm -rf $EXECUTABLE_NAME.app
mkdir -p $EXECUTABLE_NAME.app/Contents/MacOS $EXECUTABLE_NAME.app/Contents/Resources
if [ ! -z "$ICON_FILE" ]
then
	cp $ICON_FILE.icns $EXECUTABLE_NAME.app/Contents/Resources/AppIcon.icns
fi
cp $EXECUTABLE_NAME $EXECUTABLE_NAME.app/Contents/MacOS
echo "APPL????" > $EXECUTABLE_NAME.app/Contents/PkgInfo

cat <<EOF > $EXECUTABLE_NAME.app/Contents/Info.plist
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleDevelopmentRegion</key>
	<string>English</string>
	<key>CFBundleExecutable</key>
	<string>$EXECUTABLE_NAME</string>
	<key>CFBundleIconFile</key>
	<string>AppIcon</string>
	<key>CFBundleIdentifier</key>
	<string>$BUNDLE_NAME</string>
	<key>CFBundleInfoDictionaryVersion</key>
	<string>6.0</string>
	<key>CFBundleName</key>
	<string>$EXECUTABLE_NAME</string>
	<key>CFBundlePackageType</key>
	<string>APPL</string>
	<key>CFBundleSignature</key>
	<string>????</string>
</dict>
</plist>
EOF

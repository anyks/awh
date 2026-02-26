#!/bin/bash
set -e

# Аргументы
FRAMEWORK_NAME="$1"
BUILD_DIR="$2"
INSTALL_DIR="$3"
SOURCE_INCLUDE_DIR="$4"
LIB_FILES_INPUT="$5"  # Список путей к .a файлам через пробел

FRAMEWORK_PATH="${INSTALL_DIR}/${FRAMEWORK_NAME}.framework"
VERSIONS_A_PATH="${FRAMEWORK_PATH}/Versions/A"

echo "🔨 Building Framework: ${FRAMEWORK_NAME}"

# 1. Очистка
rm -rf "${FRAMEWORK_PATH}"
mkdir -p "${VERSIONS_A_PATH}/Headers"
mkdir -p "${VERSIONS_A_PATH}/Modules"
mkdir -p "${VERSIONS_A_PATH}/Resources"

# 2. Объединение библиотек
COMBINED_LIB="${VERSIONS_A_PATH}/${FRAMEWORK_NAME}"

if [ -z "$LIB_FILES_INPUT" ]; then
    echo "❌ Error: No library files provided"
    exit 1
fi

# Преобразуем строку в массив
read -ra LIB_ARRAY <<< "$LIB_FILES_INPUT"

echo "📦 Merging ${#LIB_ARRAY[@]} libraries..."
for lib in "${LIB_ARRAY[@]}"; do
    if [ ! -f "$lib" ]; then
        echo "❌ Error: Library not found: $lib"
        exit 1
    fi
    echo "   - $lib"
done

# Используем libtool для объединения (стандарт для macOS)
libtool -static -o "${COMBINED_LIB}" ${LIB_ARRAY[@]}

# 3. Заголовки
if [ -d "${SOURCE_INCLUDE_DIR}" ]; then
    cp -R "${SOURCE_INCLUDE_DIR}/"* "${VERSIONS_A_PATH}/Headers/"
fi

# 4. Info.plist
cat > "${VERSIONS_A_PATH}/Resources/Info.plist" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleIdentifier</key>
    <string>com.anyks.${FRAMEWORK_NAME}</string>
    <key>CFBundleName</key>
    <string>${FRAMEWORK_NAME}</string>
    <key>CFBundleVersion</key>
    <string>1.0.0</string>
    <key>CFBundlePackageType</key>
    <string>FMWK</string>
</dict>
</plist>
EOF

# 5. module.modulemap
cat > "${VERSIONS_A_PATH}/Modules/module.modulemap" <<EOF
framework module ${FRAMEWORK_NAME} {
    umbrella header "${FRAMEWORK_NAME}.hpp"
    export *
    module * { export * }
}
EOF

# 6. Символические ссылки
cd "${FRAMEWORK_PATH}/Versions"
ln -sf A Current
cd - > /dev/null

cd "${FRAMEWORK_PATH}"
ln -sf Versions/Current/Headers Headers
ln -sf Versions/Current/Modules Modules
ln -sf Versions/Current/Resources Resources
ln -sf Versions/Current/${FRAMEWORK_NAME} ${FRAMEWORK_NAME}
cd - > /dev/null

# 7. Подпись
codesign --force --deep --sign - "${FRAMEWORK_PATH}"

echo "✅ Framework built: ${FRAMEWORK_PATH}"

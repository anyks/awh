#!/bin/bash
set -e

# Аргументы
FRAMEWORK_NAME="$1"      # awh
BUILD_DIR="$2"           # build dir
INSTALL_DIR="$3"         # build/dist
SOURCE_INCLUDE_DIR="$4"  # include dir

# Пути
FRAMEWORK_PATH="${INSTALL_DIR}/${FRAMEWORK_NAME}.framework"
VERSIONS_A_PATH="${FRAMEWORK_PATH}/Versions/A"

echo "🔨 Building Framework: ${FRAMEWORK_NAME}"

# 1. Полная очистка
rm -rf "${FRAMEWORK_PATH}"
mkdir -p "${VERSIONS_A_PATH}/Headers"
mkdir -p "${VERSIONS_A_PATH}/Modules"
mkdir -p "${VERSIONS_A_PATH}/Resources"

# 2. Копирование библиотеки
# Ищем .a файл (статическая библиотека)
LIB_FILE=$(find "${BUILD_DIR}" -name "lib${FRAMEWORK_NAME}.a" -type f | head -n 1)
if [ -z "$LIB_FILE" ]; then
    echo "❌ Error: Library lib${FRAMEWORK_NAME}.a not found in ${BUILD_DIR}"
    exit 1
fi
cp "${LIB_FILE}" "${VERSIONS_A_PATH}/${FRAMEWORK_NAME}"

# 3. Копирование заголовков
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

# 6. Создание символических ссылок (Критично для macOS)
# Ссылка Current -> A внутри папки Versions
cd "${FRAMEWORK_PATH}/Versions"
ln -sf A Current
cd - > /dev/null

# Ссылки в корне фреймворка -> Versions/Current/...
cd "${FRAMEWORK_PATH}"
ln -sf Versions/Current/Headers Headers
ln -sf Versions/Current/Modules Modules
ln -sf Versions/Current/Resources Resources
ln -sf Versions/Current/${FRAMEWORK_NAME} ${FRAMEWORK_NAME}
cd - > /dev/null

# 7. Подпись кода (Ad-hoc)
# --deep подписывает вложенные структуры, --force перезаписывает
codesign --force --deep --sign - "${FRAMEWORK_PATH}"

echo "✅ Framework built: ${FRAMEWORK_PATH}"

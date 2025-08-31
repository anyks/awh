#!/usr/bin/env bash

# Generate application uninstallers for macOS.

# Parameters
DATE=`date +%Y-%m-%d`
TIME=`date +%H:%M:%S`
LOG_PREFIX="[$DATE $TIME]"

# Functions
log_info() {
    echo "${LOG_PREFIX}[INFO]" $1
}

log_warn() {
    echo "${LOG_PREFIX}[WARN]" $1
}

log_error() {
    echo "${LOG_PREFIX}[ERROR]" $1
}

# Check running user
if (( $EUID != 0 )); then
    echo "Please run as root."
    exit
fi

echo "Welcome to Application Uninstaller"
echo "The following packages will be REMOVED:"
echo "  __PRODUCT__-__VERSION__"
while true; do
    read -p "Do you wish to continue [Y/n]?" answer
    [[ $answer == "y" || $answer == "Y" || $answer == "" ]] && break
    [[ $answer == "n" || $answer == "N" ]] && exit 0
    echo "Please answer with 'y' or 'n'"
done


# Need to replace these with install preparation script
VERSION=__VERSION__
PRODUCT=__PRODUCT__

echo "Application uninstalling process started"

# Cmake Path
readonly CMAKE=$(cmake --system-information | grep CMAKE_ROOT | cut -d= -f2 | awk '{print $2}' | sed "s/^\([\"']\)\(.*\)\1\$/\2/g")

# Remove link CMake file
find $CMAKE/Modules -name "FindAWH.cmake" | xargs rm
if [ $? -eq 0 ]
then
  echo "[1/3] [DONE] Successfully deleted link CMake file FindAWH.cmake"
else
  echo "[1/3] [ERROR] Could not delete link CMake file FindAWH.cmake" >&2
fi

# Remove CMake file
find "/usr/local/share/__PRODUCT__/cmake" -name "FindAWH.cmake" | xargs rm
if [ $? -eq 0 ]
then
  echo "[1/3] [DONE] Successfully deleted CMake file FindAWH.cmake"
else
  echo "[1/3] [ERROR] Could not delete CMake file FindAWH.cmake" >&2
fi

# Remove static library
find "/usr/local/lib/" -name "lib__PRODUCT__.a" | xargs rm
if [ $? -eq 0 ]
then
  echo "[1/3] [DONE] Successfully deleted static library lib__PRODUCT__.a"
else
  echo "[1/3] [ERROR] Could not delete static library lib__PRODUCT__.a" >&2
fi

# Remove dynamic library
find "/usr/local/lib/" -name "lib__PRODUCT__.dylib" | xargs rm
if [ $? -eq 0 ]
then
  echo "[1/3] [DONE] Successfully deleted dynamic library lib__PRODUCT__.dylib"
else
  echo "[1/3] [ERROR] Could not delete dynamic library lib__PRODUCT__.dylib" >&2
fi

# Remove include files
[ -e "/usr/local/include/lib__PRODUCT__" ] && rm -rf "/usr/local/include/lib__PRODUCT__"
if [ $? -eq 0 ]
then
  echo "[3/3] [DONE] Successfully deleted include files"
else
  echo "[3/3] [ERROR] Could not delete include files" >&2
fi

# Forget from pkgutil
pkgutil --forget "org.$PRODUCT.$VERSION" > /dev/null 2>&1
if [ $? -eq 0 ]
then
  echo "[2/3] [DONE] Successfully deleted application informations"
else
  echo "[2/3] [ERROR] Could not delete application informations" >&2
fi

echo "Application uninstall process finished"
exit 0

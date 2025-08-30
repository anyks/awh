#!/usr/bin/env bash

#Configuration Variables and Parameters

#Parameters
SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
TARGET_DIRECTORY="$SCRIPTPATH/target"
PRODUCT=awh
VERSION=$1
AARCH=$(uname -m)
DATE=`date +%Y-%m-%d`
TIME=`date +%H:%M:%S`
LOG_PREFIX="[$DATE $TIME]"

function printSignature() {
	cat "$SCRIPTPATH/utils/ascii_art.txt"
	echo
}

#Start the generator
printSignature

#Functions
go_to_dir() {
	pushd $1 >/dev/null 2>&1
}

log_info() {
	echo "${LOG_PREFIX}[INFO]" $1
}

log_warn() {
	echo "${LOG_PREFIX}[WARN]" $1
}

log_error() {
	echo "${LOG_PREFIX}[ERROR]" $1
}

deleteInstallationDirectory() {
	log_info "Cleaning $TARGET_DIRECTORY directory."
	rm -rf "$TARGET_DIRECTORY"

	if [[ $? != 0 ]]; then
		log_error "Failed to clean $TARGET_DIRECTORY directory" $?
		exit 1
	fi
}

createInstallationDirectory() {
	if [ -d "${TARGET_DIRECTORY}" ]; then
		deleteInstallationDirectory
	fi
	mkdir -pv "$TARGET_DIRECTORY"

	if [[ $? != 0 ]]; then
		log_error "Failed to create $TARGET_DIRECTORY directory" $?
		exit 1
	fi
}

copyDarwinDirectory(){
	createInstallationDirectory
	cp -r "$SCRIPTPATH/darwin" "${TARGET_DIRECTORY}/"
	chmod -R 755 "${TARGET_DIRECTORY}/darwin/scripts"
	chmod -R 755 "${TARGET_DIRECTORY}/darwin/Resources"
	chmod 755 "${TARGET_DIRECTORY}/darwin/Distribution"
}

copyBuildDirectory() {
	sed -i '' -e 's/__VERSION__/'${VERSION}'/g' "${TARGET_DIRECTORY}/darwin/scripts/postinstall"
	sed -i '' -e 's/__PRODUCT__/'${PRODUCT}'/g' "${TARGET_DIRECTORY}/darwin/scripts/postinstall"
	chmod -R 755 "${TARGET_DIRECTORY}/darwin/scripts/postinstall"

	sed -i '' -e 's/__VERSION__/'${VERSION}'/g' "${TARGET_DIRECTORY}/darwin/Distribution"
	sed -i '' -e 's/__PRODUCT__/'${PRODUCT}'/g' "${TARGET_DIRECTORY}/darwin/Distribution"
	chmod -R 755 "${TARGET_DIRECTORY}/darwin/Distribution"

	sed -i '' -e 's/__VERSION__/'${VERSION}'/g' "${TARGET_DIRECTORY}/darwin/Resources/"*.html
	sed -i '' -e 's/__PRODUCT__/'${PRODUCT}'/g' "${TARGET_DIRECTORY}/darwin/Resources/"*.html
	chmod -R 755 "${TARGET_DIRECTORY}/darwin/Resources/"

	rm -rf "${TARGET_DIRECTORY}/darwinpkg"
	mkdir -p "${TARGET_DIRECTORY}/darwinpkg"

	# Copy cellery product to ${PRODUCT}
	mkdir -p "${TARGET_DIRECTORY}/darwinpkg/tmp"
	mkdir -p "${TARGET_DIRECTORY}/darwinpkg/usr/local/lib"
	mkdir -p "${TARGET_DIRECTORY}/darwinpkg/usr/local/include/lib${PRODUCT}"

	cp -a "$SCRIPTPATH/application"/FindAWH.cmake "${TARGET_DIRECTORY}/darwinpkg/tmp"/FindAWH.cmake
	cp -a "$SCRIPTPATH/application"/lib${PRODUCT}.a "${TARGET_DIRECTORY}/darwinpkg/usr/local/lib"/lib${PRODUCT}.a
	cp -a "$SCRIPTPATH/application"/lib${PRODUCT}.dylib "${TARGET_DIRECTORY}/darwinpkg/usr/local/lib"/lib${PRODUCT}.dylib
	cp -r "$SCRIPTPATH/application/include"/* "${TARGET_DIRECTORY}/darwinpkg/usr/local/include/lib${PRODUCT}"/

	chmod -R 755 "${TARGET_DIRECTORY}/darwinpkg/"

	rm -rf "${TARGET_DIRECTORY}/package"
	mkdir -p "${TARGET_DIRECTORY}/package"
	chmod -R 755 "${TARGET_DIRECTORY}/package"

	rm -rf "${TARGET_DIRECTORY}/pkg"
	mkdir -p "${TARGET_DIRECTORY}/pkg"
	chmod -R 755 "${TARGET_DIRECTORY}/pkg"
}

function buildPackage() {
	log_info "Application installer package building started.(1/4)"
	pkgbuild \
	 --identifier "org.${PRODUCT}.${VERSION}" \
	 --version "${VERSION}" \
	 --scripts "${TARGET_DIRECTORY}/darwin/Scripts" \
	 --root "${TARGET_DIRECTORY}/darwinpkg" \
	 "${TARGET_DIRECTORY}/package/${PRODUCT}.pkg" > /dev/null 2>&1
}

function buildProduct() {
	log_info "Application installer product building started.(2/4)"
	productbuild \
	 --distribution "${TARGET_DIRECTORY}/darwin/Distribution" \
	 --resources "${TARGET_DIRECTORY}/darwin/Resources" \
	 --package-path "${TARGET_DIRECTORY}/package" \
	 "${TARGET_DIRECTORY}/pkg/$1" > /dev/null 2>&1
}

function signProduct() {
	log_info "Application installer signing process started.(3/4)"
	mkdir -pv "${TARGET_DIRECTORY}/pkg-signed"
	chmod -R 755 "${TARGET_DIRECTORY}/pkg-signed"

	productsign \
	 --sign "Developer ID Installer: Aleksandr Kireev (8NGCRP2X4C)" \
	 "${TARGET_DIRECTORY}/pkg/$1" \
	 "${TARGET_DIRECTORY}/pkg-signed/$1"

	pkgutil --check-signature "${TARGET_DIRECTORY}/pkg-signed/$1"
}

function notarizeProduct() {
	log_info "Application installer notarize process started.(4/4)"

	xcrun altool \
	 -t osx \
	 --notarize-app \
	 --primary-bundle-id "com.pangeoradar.${PRODUCT}" \
	 --username "sixteeneighteen@icloud.com" \
	 --password "nrbi-mhbo-ntyz-rnqn" \
	 --file "${TARGET_DIRECTORY}/pkg-signed/$1" \
	 --asc-provider "8NGCRP2X4C"
}

function createInstaller() {
	log_info "Application installer generation process started.(4 Steps)"
	buildPackage
	buildProduct ${PRODUCT}-macos-installer-${AARCH}-${VERSION}.pkg
	# signProduct ${PRODUCT}-macos-installer-${AARCH}-${VERSION}.pkg
	# notarizeProduct ${PRODUCT}-macos-installer-${AARCH}-${VERSION}.pkg
	log_info "Application installer generation steps finished."
}

function createUninstaller(){
	cp "$SCRIPTPATH/darwin/Resources/uninstall.sh" "${TARGET_DIRECTORY}/darwinpkg/usr/local/include/lib${PRODUCT}"
	sed -i '' -e "s/__VERSION__/${VERSION}/g" "${TARGET_DIRECTORY}/darwinpkg/usr/local/include/lib${PRODUCT}/uninstall.sh"
	sed -i '' -e "s/__PRODUCT__/${PRODUCT}/g" "${TARGET_DIRECTORY}/darwinpkg/usr/local/include/lib${PRODUCT}/uninstall.sh"
}

#Pre-requisites
command -v mvn -v >/dev/null 2>&1 || {
	log_warn "Apache Maven was not found. Please install Maven first."z
	# exit 1
}

command -v ballerina >/dev/null 2>&1 || {
	log_warn "Ballerina was not found. Please install ballerina first."
	# exit 1
}

#Main script
log_info "Installer generating process started."

copyDarwinDirectory
copyBuildDirectory
createUninstaller
createInstaller

log_info "Installer generating process finished"
exit 0

#!/bin/sh

SOURCE_FOLDER="${MESON_BUILD_ROOT}/doc/Snapd-2"
DESTINATION_FOLDER="${DESTDIR}/${MESON_INSTALL_PREFIX}/share/doc/snapd/html"

if [ -d "${SOURCE_FOLDER}" ]; then
        mkdir -p "${DESTINATION_FOLDER}"
        cp -a "${SOURCE_FOLDER}"/* "${DESTINATION_FOLDER}"/
fi

#!/bin/bash

P="$( dirname "$( readlink -f "$0" )" )"

version="1.0.0"
distindex="package_walkino_index.json"
distfile="walkino-${version}.tar.gz"
#distfile="walkino-${version}.zip"

mkdir -p "${P}/dist/${version}"
cp -R "${P}/hardware/walkera/xmega/"* "${P}/dist/${version}"
mv "${P}/dist/${version}/platform.txt.board" "${P}/dist/${version}/platform.txt"

cd "${P}/dist"
tar czvf "${P}/${distfile}" "${version}" > /dev/null
#zip -r "${P}/${distfile}" "${version}" > /dev/null
cd "$P"

disthash=$(sha256sum "${distfile}" | awk '{ print $1 }')
distsize=$(stat -c%s "${distfile}")

jq \
	--arg hash "SHA-256:$disthash" \
	--arg size "$distsize" \
	--indent 2 \
	'.packages[0].platforms[0].checksum = $hash | .packages[0].platforms[0].size = $size' \
	"${distindex}" \
	> "${distindex}.tmp" && \
	mv "${distindex}.tmp" "${distindex}"


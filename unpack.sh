#!/bin/bash

set -e
set -u

SIEVE=$1
DSTDIR=$2
SIEVE_MAP=$SIEVE.map
SIEVE_LZ4=$SIEVE.lz4
SIEVE_LZO=$SIEVE.lzo
SIEVE_GZ=$SIEVE.gz

if ! test -d "${DSTDIR}"; then
	echo "target directory does not exist"
	exit 1
fi

if test -r "${SIEVE_MAP}"; then
	echo "found uncompressed ${SIEVE_MAP}"
	cp --update "${SIEVE_MAP}" "${DSTDIR}" || :
	exit 0
fi

if test -r "${SIEVE_LZ4}" && type lz4 > /dev/null 2> /dev/null; then
	echo "found LZ4 archive for ${SIEVE_MAP}"
	lz4 -d "${SIEVE_LZ4}" "${DSTDIR}"/"${SIEVE_MAP}"
	exit 0
fi

if test -r "${SIEVE_LZO}" && type lzop > /dev/null 2> /dev/null; then
	echo "found LZO archive for ${SIEVE_MAP}"
	lzop -d -o "${DSTDIR}"/"${SIEVE_MAP}" "${SIEVE_LZO}"
	exit 0
fi

if test -r "${SIEVE_GZ}" && type gzip > /dev/null 2> /dev/null; then
	echo "found GZ archive for ${SIEVE_MAP}"
	gzip -d -c "${SIEVE_GZ}" > "${DSTDIR}"/"${SIEVE_MAP}"
	exit 0
fi


echo "${SIEVE_MAP} not found"
exit 1

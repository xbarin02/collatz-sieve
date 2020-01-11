#!/bin/bash

set -e
set -u

SIEVE=$1
SIEVE_MAP=$SIEVE.map
SIEVE_LZ4=$SIEVE.lz4
SIEVE_LZO=$SIEVE.lzo
SIEVE_GZ=$SIEVE.gz

if test ! -r ${SIEVE_MAP}; then
	echo "the sieve does not exist"
	exit 1
fi

if test ! -r ${SIEVE_LZ4}; then
	echo "creating LZ4 archive..."
	lz4 --best ${SIEVE_MAP} ${SIEVE_LZ4}
	echo "testing LZ4 archive..."
	lz4 -d ${SIEVE_LZ4} temp
	diff ${SIEVE_MAP} temp
	echo "LZ4 archive OK"
	rm temp
fi

if test ! -r ${SIEVE_LZO}; then
	echo "creating LZO archive..."
	lzop --best -o ${SIEVE_LZO} ${SIEVE_MAP}
	echo "testing LZO archive..."
	lzop -d -o temp ${SIEVE_LZO}
	diff ${SIEVE_MAP} temp
	echo "LZO archive OK"
	rm temp
fi

if test ! -r ${SIEVE_GZ}; then
	echo "creating GZ archive..."
	gzip --best -c ${SIEVE_MAP} > ${SIEVE_GZ}
	echo "testing GZ archive..."
	gzip -d -c ${SIEVE_GZ} > temp
	diff ${SIEVE_MAP} temp
	echo "GZ archive OK"
	rm temp
fi

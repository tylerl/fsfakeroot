#!/bin/bash
# prune_proxy.sh
#
# Copyright (c) 2010, Tyler Larson <devel@tlarson.com>
#
# This software is licensed under the terms of the MIT License.
# See the included file "LICENSE" for more information.
#

die() { 
	echo $* >&2
	exit 1
}
usage() {
	{ 
		echo "usage: $1 <fake_root> <proxy_dir>"
		echo "  <fake_root>: The root directory of your fsfakeroot operations."
		echo "  <proxy_dir>: The FSFR_PROXY_DIR you want to clean out."
	} >&2
	exit 2
}

[ $# == 2 ] || usage $0
FAKE_ROOT=$1
PROXY_DIR=$2
[ -d "$FAKE_ROOT" ] || die "$0: <fake_root> is not a directory"
[ -d "$PROXY_DIR" ] || die "$0: <proxy_dir> is not a directory"

TMPFILE=`mktemp` || die "$0: failed to create temporary file"
find "$FAKE_ROOT" -type l -printf "%i\n" > "$TMPFILE"

find "$FAKE_ROOT" -name '*.fsfr' -type f | while read file; do
	FILENAME=${file##*/}
	INODE=${FILENAME%.*}
	grep -q "^$INODE$" "$TMPFILE" && continue
	rm -f "$file"
done

rm -f "$TMPFILE"

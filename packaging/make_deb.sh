#!/bin/bash

DIR=`dirname "${BASH_SOURCE[0]}"`
ABSDIR=`cd "$DIR"; pwd`

cd "$DIR"/..

VERSION=`cat version`

./autogen.sh
./configure
make dist

rm -rf deb
mkdir deb
cp tg-expert-"$VERSION".tar.gz deb/tg-expert_"$VERSION".orig.tar.gz

cd deb

tar xzf tg-expert_"$VERSION".orig.tar.gz
cp -r "$ABSDIR"/debian tg-expert-"$VERSION"
cd tg-expert-"$VERSION"
# debuild resets PATH to the system folders. Typst, needed for the manual,
# is usually installed by hand in /usr/local/bin, so that folder is added.
if ! command -v typst >/dev/null 2>&1
then
	echo "Warning: typst not found, the package will not include the manual." >&2
fi
debuild --prepend-path=/usr/local/bin -us -uc

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
debuild -us -uc

#!/bin/bash

set -e

if test ! -d "$1"
then
	echo "Usage: $0 resources-dir"
	echo
	echo "resources-dir must contain runtime files to be bundled (DLLs, data files, etc.)."
	exit 1
fi

DIR=`dirname "${BASH_SOURCE[0]}"`
ABSDIR=`cd "$DIR"; pwd`

if ! command -v heat >/dev/null 2>&1 || ! command -v candle >/dev/null 2>&1 || ! command -v light >/dev/null 2>&1
then
	echo "Error: WiX Toolset v3 commands not found in PATH (heat, candle, light)."
	echo "Install WiX Toolset and ensure its bin directory is available in the shell PATH."
	exit 1
fi

if ! command -v uuidgen >/dev/null 2>&1
then
	echo "Error: uuidgen not found in PATH; required by packaging/make_wxs.sh."
	exit 1
fi

RESOURCES=`cd "$1"; pwd`
TARGET="$ABSDIR/../msi"

cd "$ABSDIR"/..

VERSION=`cat version`

"$ABSDIR/make_wxs.sh"

./configure
make

rm -rf "$TARGET"
mkdir -p "$TARGET"

# The user manuals, in English, Spanish and German: built with Typst when it
# is in PATH, else taken from tg-expert-manual-VERSION[-lang].pdf in the
# source root. Installed as share/doc/tg-expert/tg-expert-manual[-lang].pdf.
for lang in en es de
do
	if test $lang = en; then src=main.typ; suffix=""; else src=main-$lang.typ; suffix="-$lang"; fi
	prebuilt="$ABSDIR/../tg-expert-manual-${VERSION}${suffix}.pdf"
	out="$TARGET/tg-expert-manual${suffix}.pdf"
	if command -v typst >/dev/null 2>&1
	then
		typst compile --root "$ABSDIR/.." "$ABSDIR/../docs/manual/$src" "$out"
	elif test -f "$prebuilt"
	then
		cp "$prebuilt" "$out"
	else
		echo "Error: typst is not in PATH and $prebuilt does not exist."
		echo "Build it with: typst compile --root . docs/manual/$src tg-expert-manual-${VERSION}${suffix}.pdf"
		exit 1
	fi
done

# Runtime files plus the translations of the interface (built by make in po/),
# harvested together into Resources.wxs
STAGE="$TARGET/resources-stage"
mkdir -p "$STAGE"
cp -r "$RESOURCES"/. "$STAGE"
for lang in es de
do
	if test ! -f "$ABSDIR/../po/$lang.gmo"
	then
		echo "Error: po/$lang.gmo not found. Install gettext (msgfmt) and run configure and make again."
		exit 1
	fi
	mkdir -p "$STAGE/share/locale/$lang/LC_MESSAGES"
	cp "$ABSDIR/../po/$lang.gmo" "$STAGE/share/locale/$lang/LC_MESSAGES/tg-expert.mo"
done

cp "$ABSDIR/../tg-expert.exe" "$TARGET/tg-expert.exe"
cd "$TARGET"
cp "$ABSDIR/tg-expert.wxs" "$TARGET"
cp "$ABSDIR/LICENSE.rtf" "$TARGET"
cp "$ABSDIR/../README.md" "$TARGET"
cp "$ABSDIR/../LICENSE" "$TARGET"
cp "$ABSDIR/../icons/tg-document.ico" "$TARGET"
cp -r "$STAGE"/. "$TARGET"
heat dir "$STAGE" -srd -gg -sreg -dr INSTALLDIR -cg Resources -out "$TARGET/Resources.wxs"
rm -rf "$STAGE"

# WixUtilExtension provides WixShellExec, which launches the program from the last dialog
candle -ext WixUtilExtension tg-expert.wxs Resources.wxs
light -out tg-expert_${VERSION}.msi -ext WixUIExtension -ext WixUtilExtension tg-expert.wixobj Resources.wixobj

echo "MSI generated at: $TARGET/tg-expert_${VERSION}.msi"

#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
app="$PWD/dist/tg-expert.app"
contents="$app/Contents"
exe="$contents/MacOS/tg-expert-bin"
libs="$contents/Frameworks"
share="$contents/Resources/share"
modules="$libs/pixbuf-loaders"
mkdir -p dist
rm -rf -- "$app"
mkdir -p "$contents/MacOS" "$modules" "$share"
install -m755 tg-expert "$exe"
install -m644 LICENSE "$contents/Resources/LICENSE"

prefix=$(brew --prefix)
while IFS= read -r path || [[ -n "$path" ]]; do
    path=${path%$'\r'}
    path=${path//\\//}
    case "$path" in
        */icon-theme.cache) continue ;;
        share/icons/*|share/locale/*) ;;
        *) continue ;;
    esac
    case "/$path/" in */../*|*/./*) exit 1 ;; esac
    if [[ ! -f "$prefix/$path" ]]; then
        printf 'Resource not found: %s\n' "$path" >&2
        continue
    fi
    dest="$contents/Resources/$path"
    mkdir -p "$(dirname "$dest")"
    cp -L "$prefix/$path" "$dest"
done < packaging/tg-expert_resources
for theme in Adwaita hicolor; do
    test -f "$share/icons/$theme/index.theme" || {
        echo "Missing icon theme: $theme" >&2; exit 1;
    }
done
for lang in es de; do
    dest="$share/locale/$lang/LC_MESSAGES"
    mkdir -p "$dest"
    install -m644 "po/$lang.gmo" "$dest/tg-expert.mo"
done

query="$(brew --prefix gdk-pixbuf)/bin/gdk-pixbuf-query-loaders"
svg=$(find "$(brew --prefix librsvg)/" -type f \( -name '*pixbufloader*svg*.so' -o -name '*pixbufloader*svg*.dylib' \) -print | sed -n '1p')
[[ -n "$svg" ]] || { echo 'librsvg SVG loader not found' >&2; exit 1; }
module="$modules/$(basename "$svg")"
cp -L "$svg" "$module"
# Query the original module before rewriting its dependencies.
"$query" "$svg" | awk -v name="$(basename "$svg")" '
    /^#/ { next }
    /^".*"$/ && !seen { print "\"@TG_PIXBUF_MODULE@/" name "\""; seen=1; next }
    { print }
    END { if (!seen) exit 1 }
' > "$contents/Resources/pixbuf-loaders.template"
grep -Fq 'image/svg+xml' "$contents/Resources/pixbuf-loaders.template"
# Bundle both Mach-O roots in one invocation so they share Frameworks.
dylibbundler -of -b -x "$exe" -x "$module" -d "$libs/" \
    -p '@executable_path/../Frameworks/'
for binary in "$exe" "$module" "$libs"/*.dylib; do
    refs=$(otool -L "$binary")
    printf '%s\n' "$refs"
    if grep -Eq '^[[:space:]]+(/opt/homebrew/|/usr/local/(opt|Cellar)/)' <<< "$refs"; then
        echo "Homebrew dependency remains in $binary" >&2; exit 1
    fi
done

cat > "$contents/MacOS/tg-expert" <<'LAUNCHER'
#!/bin/bash
set -euo pipefail
contents=$(cd "$(dirname "$0")/.." && pwd -P)
resources="$contents/Resources"
export XDG_DATA_DIRS="$resources/share:${XDG_DATA_DIRS:-/usr/local/share:/usr/share}"
cache=$(mktemp -d "${TMPDIR:-/tmp}/tg-expert-pixbuf.XXXXXX")
trap 'rm -rf -- "$cache"' EXIT
module_dir="$contents/Frameworks/pixbuf-loaders"
module_dir=${module_dir//\\/\\\\}
module_dir=${module_dir//\"/\\\"}
while IFS= read -r line; do
    printf '%s\n' "${line//@TG_PIXBUF_MODULE@/$module_dir}"
done < "$resources/pixbuf-loaders.template" > "$cache/loaders.cache"
export GDK_PIXBUF_MODULE_FILE="$cache/loaders.cache"
"$contents/MacOS/tg-expert-bin" "$@"
LAUNCHER
chmod 755 "$contents/MacOS/tg-expert"
python3 - "$contents/Info.plist" <<'PY'
import plistlib
import sys
from pathlib import Path

version = Path('version').read_text().strip()
with open(sys.argv[1], 'wb') as f:
    plistlib.dump(dict(
        CFBundleIdentifier='io.github.jefebus.tg-expert',
        CFBundleName='TG Expert',
        CFBundleExecutable='tg-expert',
        CFBundlePackageType='APPL',
        CFBundleVersion=version,
        CFBundleShortVersionString=version,
        NSMicrophoneUsageDescription='TG Expert uses audio input to measure mechanical watches.',
    ), f)
PY
plutil -lint "$contents/Info.plist"
for binary in "$libs"/*.dylib "$module" "$exe"; do
    codesign --force --sign - "$binary"
done
codesign --force --sign - "$app"
codesign --verify --deep --strict "$app"
ditto -c -k --keepParent "$app" dist/tg-expert-macos-arm64-preview.app.zip

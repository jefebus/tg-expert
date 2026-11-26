#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "Uso: bash $0 directorio-de-los-TSV" >&2
    exit 2
fi

audit=$1
packages="$audit/windows-package-licenses.tsv"
manifest="$audit/windows-runtime-manifest.tsv"
prefix=${MINGW_PREFIX:?Ejecuta este script desde MINGW64}

for file in "$packages" "$manifest"; do
    [[ -s "$file" ]] || { printf 'Falta el TSV: %s\n' "$file" >&2; exit 1; }
done

work=$(mktemp -d)
catalog="$work/third-party"
report="$work/REVIEW.tsv"
notices="$catalog/THIRD-PARTY-NOTICES.txt"

mkdir -p -- "$catalog/licenses"
cp -- "$packages" "$manifest" "$work/"
printf 'component\tpackage\tversion\tstatus\n' > "$report"

cat > "$notices" <<'EOF'
tg-expert — Third-party notices
==============================

This catalogue covers third-party components distributed with the Windows
and macOS editions. Not every component is included in every edition.
The original licence texts and accompanying notices are grouped by
component in the licenses/ directory.
EOF

while IFS=$'\t' read -r pkg version declared paths; do
    [[ -n "$pkg" && "$pkg" != package ]] || continue
    paths=${paths%$'\r'}

    # Stable upstream names, independent of the package manager.
    component=${pkg#mingw-w64-x86_64-}
    case "$component" in
        atk)             component=at-spi2-core ;;
        gdk-pixbuf2)     component=gdk-pixbuf ;;
        gettext-runtime) component=gettext ;;
        glib2)           component=glib ;;
        libgcc|libstdc++) component=gcc ;;
        libwinpthread)   component=winpthreads ;;
        libdatrie)       component=libdatrie ;;
    esac

    # Do not copy texts from a newer installation against an older TSV.
    if ! current=$(pacman -Q -- "$pkg" 2>/dev/null) ||
       [[ "$current" != "$pkg $version" ]]; then
        printf '%s\t%s\t%s\tPACKAGE_VERSION_CHANGED\n' \
            "$component" "$pkg" "$version" >> "$report"
        continue
    fi

    if [[ "$component" == portaudio && "$paths" == "-" ]]; then
        paths="$prefix/share/doc/portaudio/LICENSE.txt"
    fi

    if [[ "$paths" == "-" || -z "$paths" ]]; then
        printf '%s\t%s\t%s\tMISSING_TEXT\n' \
            "$component" "$pkg" "$version" >> "$report"
        continue
    fi

    IFS=';' read -r -a files <<< "$paths"
    status=COLLECTED

    for src in "${files[@]}"; do
        if [[ ! -f "$src" ]]; then
            printf 'No se encuentra: %s\n' "$src" >&2
            status=MISSING_FILE
            continue
        fi

        # Preserve subdirectories such as gettext's intl/ and libasprintf/.
        case "$src" in
            "$prefix"/share/licenses/*)
                rest=${src#"$prefix"/share/licenses/}
                relative=${rest#*/}
                ;;
            *)
                relative=${src##*/}
                ;;
        esac

        dest="$catalog/licenses/$component/$relative"
        mkdir -p -- "$(dirname -- "$dest")"

        if [[ -f "$dest" ]] && ! cmp -s -- "$src" "$dest"; then
            printf 'Textos distintos para %s: %s\n' "$dest" "$src" >&2
            status=TEXT_CONFLICT
            continue
        fi

        cp -- "$src" "$dest"
    done

    printf '%s\t%s\t%s\t%s\n' \
        "$component" "$pkg" "$version" "$status" >> "$report"
done < "$packages"

{
    printf '\nComponents with collected texts\n'
    printf '%s\n' '------------------------------'
    for dir in "$catalog"/licenses/*/; do
        [[ -d "$dir" ]] || continue
        name=${dir%/}
        name=${name##*/}
        printf '  %s — licenses/%s/\n' "$name" "$name"
    done
} >> "$notices"

printf '\nCatálogo preparado: %s\n' "$(cygpath -w "$catalog")"
printf 'Informe de revisión: %s\n' "$(cygpath -w "$report")"

printf '\nPaquetes pendientes:\n'
awk -F '\t' 'NR > 1 && $4 != "COLLECTED"' "$report"

printf '\nRecursos pendientes de atribución:\n'
awk -F '\t' 'NR > 1 && $6 != "OK"' "$manifest"

printf '\nNo se ha modificado packaging/third-party/.\n'
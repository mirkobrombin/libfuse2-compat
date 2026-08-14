#!/usr/bin/env bash
set -euo pipefail

project_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
fixture_dir="$project_root/tests/fixtures"
fixture="$fixture_dir/obsolete-appimagetool-x86_64.AppImage"
url=https://github.com/AppImage/AppImageKit/releases/download/13/obsolete-appimagetool-x86_64.AppImage
expected=df3baf5ca5facbecfc2f3fa6713c29ab9cefa8fd8c1eac5d283b79cab33e4acb

mkdir -p "$fixture_dir"
if [[ -f $fixture ]]; then
    actual=$(sha256sum "$fixture" | awk '{ print $1 }')
    if [[ $actual == "$expected" ]]; then
        printf '%s\n' "$fixture"
        exit 0
    fi
fi

curl --fail --location --silent --show-error --output "$fixture.tmp" "$url"
actual=$(sha256sum "$fixture.tmp" | awk '{ print $1 }')
if [[ $actual != "$expected" ]]; then
    printf 'fixture checksum mismatch: expected %s, got %s\n' \
        "$expected" "$actual" >&2
    rm -f "$fixture.tmp"
    exit 1
fi
mv "$fixture.tmp" "$fixture"
chmod +x "$fixture"
printf '%s\n' "$fixture"

#!/bin/sh
# Fetch the engines and fonts at the exact commits the benchmark was run with.
# Everything lands in third_party/ (git-ignored).
set -e
cd "$(dirname "$0")"
mkdir -p third_party
cd third_party

clone() { # <dir> <url> <commit>
  if [ ! -d "$1" ]; then git clone -q "$2" "$1"; fi
  git -C "$1" checkout -q "$3"
}

clone RmlUi https://github.com/mikke89/RmlUi ba95ffe8bfb6370efb2cdcca927eaad4710c5413        # tag 6.3
clone litehtml https://github.com/litehtml/litehtml 735a7b391bf8c70fe6279e9a0eaedb44ae3b56cf  # master 2026-10-24
clone quickjs https://github.com/bellard/quickjs 7d1c13c5d8f8c6b32893786ed35cdcebd88194d2     # master 2026-10-20
# Local patches (see patches/README.md). Skipped when already applied.
apply() { # <dir> <patch>
  if git -C "$1" apply --check "../../patches/$2" 2>/dev/null; then git -C "$1" apply "../../patches/$2"; fi
}
apply quickjs quickjs-vita.patch
apply RmlUi rmlui-sdl-straight-alpha.patch

# Noto Sans JP (SIL OFL 1.1) from google/fonts, reduced to a static Regular instance.
if [ ! -f fonts/NotoSansJP-Regular.ttf ]; then
  mkdir -p fonts
  if [ ! -d gfonts ]; then
    git clone -q --filter=blob:none --sparse https://github.com/google/fonts gfonts
    git -C gfonts sparse-checkout set ofl/notosansjp
  fi
  git -C gfonts checkout -q e7db6ca603fd4862010383ff17ffd6cc4ffac1c0
  cp gfonts/ofl/notosansjp/OFL.txt fonts/NotoSansJP-OFL.txt
  python3 -m fontTools.varLib.instancer "gfonts/ofl/notosansjp/NotoSansJP[wght].ttf" wght=400 \
    -o fonts/NotoSansJP-Regular.ttf
fi

# Stack C: Preact 11 + hooks + htm, ES modules from npm (QuickJS loads ES modules natively).
if [ ! -f js/preact.mjs ]; then
  mkdir -p js npm
  (cd npm && npm pack -s preact@11.0.2 htm@3.1.1 >/dev/null)
  tar -xzf npm/preact-11.0.2.tgz -C npm
  cp npm/package/dist/preact.mjs js/preact.mjs && cp npm/package/hooks/dist/hooks.mjs js/hooks.mjs
  rm -rf npm/package
  tar -xzf npm/htm-3.1.1.tgz -C npm && cp npm/package/dist/htm.mjs js/htm.mjs && rm -rf npm/package
fi
echo "third_party ready"
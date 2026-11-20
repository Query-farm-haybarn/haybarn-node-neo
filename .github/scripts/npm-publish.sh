#!/usr/bin/env bash
# Publish one @haybarn/* package directory to npm through trusted publishing.
#
# Usage: npm-publish.sh <package-dir>
# Env:   NPM_DIST_TAG (required), DRY_RUN ("true" → npm publish --dry-run)
#
# Auth is the job's GitHub OIDC identity (permissions: id-token: write); no
# token. Each package's trusted publisher on npmjs.com must name
# Query-farm-haybarn/haybarn-node-neo and the workflow file haybarn-node-neo.yml.
# The OIDC exchange needs npm >= 11.5.1 (Node 24 bundles npm 11).
#
# `pnpm pack` first because the packages declare `workspace:*` dependencies:
# pnpm rewrites them to the real version in the tarball, npm would not.
# The tarball is then published with npm, which does the OIDC exchange.
set -euo pipefail

dir="${1:?usage: npm-publish.sh <package-dir>}"
: "${NPM_DIST_TAG:?NPM_DIST_TAG must be set}"

npm_version="$(npm --version)"
if [ "$(printf '%s\n' 11.5.1 "$npm_version" | sort -V | head -n1)" != 11.5.1 ]; then
  echo "::error::npm $npm_version is too old for trusted publishing (needs >= 11.5.1)"
  exit 1
fi

# Skip a version that is already on the registry, so a tag run that is
# repeated after a partial publish only publishes what is missing.
name="$(node -p "require('./$dir/package.json').name")"
version="$(node -p "require('./$dir/package.json').version")"
if [ -n "$(npm view "$name@$version" version 2>/dev/null || true)" ]; then
  echo ">>> $name@$version is already on npm; skipping"
  exit 0
fi

out="$(mktemp -d)"
(cd "$dir" && pnpm pack --pack-destination "$out")
tarball="$(ls "$out"/*.tgz)"

dry_run=()
[ "${DRY_RUN:-false}" = "true" ] && dry_run=(--dry-run)

echo ">>> npm publish $(basename "$tarball") (dist-tag: $NPM_DIST_TAG${dry_run:+, dry run})"
# ${a[@]+"${a[@]}"}: an empty array under `set -u` is an error in bash 3.2
# (macOS runners), which "${dry_run[@]}" alone would trip.
npm publish "$tarball" ${dry_run[@]+"${dry_run[@]}"} --provenance --access public --tag "$NPM_DIST_TAG" --ignore-scripts

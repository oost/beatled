#!/usr/bin/env bash
#
# Wire scripts/git-hooks/ into the local checkout as the active hooks
# directory. Run once after a fresh clone. Uses git's core.hooksPath
# rather than copying files into .git/hooks/ so a `git pull` updates
# every contributor's hooks automatically.

set -euo pipefail

repo_root=$(git rev-parse --show-toplevel)
hooks_dir="$repo_root/scripts/git-hooks"

if [ ! -d "$hooks_dir" ]; then
    echo "Expected hooks dir at $hooks_dir" >&2
    exit 1
fi

chmod +x "$hooks_dir"/pre-commit

git config core.hooksPath "$hooks_dir"

echo "Hooks installed:"
git config --get core.hooksPath
echo
echo "Skip a one-off commit with: BEATLED_SKIP_HOOKS=1 git commit ..."

#!/bin/sh
# usage: xmerge.sh <base> <branch> ; run inside the platforms worktree
set -e
base=$1; br=$2; theirs=$(git rev-parse "$br")
out=$(git -c merge.conflictStyle=zdiff3 merge-tree --write-tree --merge-base="$base" HEAD "$theirs" || true)
tree=$(echo "$out" | head -1)
echo "$out" | sed -n '2,/^$/p' | sed '/^$/d' | awk '{print $NF}' | sort -u > ../<notes-dir>/conflicts-$(echo $br | tr / _).txt
echo "$out" | grep 'CONFLICT' > ../<notes-dir>/messages-$(echo $br | tr / _).txt || true
git read-tree -u --reset "$tree"
gd=$(git rev-parse --git-dir)
echo "$theirs" > "$gd/MERGE_HEAD"
printf "Merge %s into platforms\n\nMerged with explicit base %s so only the branch's own commits are\nmerged, not the older bobtista trunk it was built on.\n" "$br" "$(git rev-parse --short $base)" > "$gd/MERGE_MSG"
echo "tree $tree; conflicted files: $(wc -l < ../<notes-dir>/conflicts-$(echo $br | tr / _).txt)"

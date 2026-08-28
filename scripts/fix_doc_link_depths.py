#!/usr/bin/env python3
"""scripts/fix_doc_link_depths.py — fix broken internal markdown link depths.

After Wave 1 reorganization (commit e6a4519), many links that should point
to repo-root files (tests/, examples/, src/, include/, CONTRIBUTING.md,
PLATFORMS.md, CMakeLists.txt) used the wrong depth — `../../X` instead of
`../../../X` — because they were originally written when the doc lived at
depth 2 (docs/X.md) rather than depth 3 (docs/category/X.md).

This script walks every markdown file in the repo, extracts internal
markdown links, resolves each link's target relative to the source file's
directory, and if the target does not exist, tries to fix the depth by
inserting one more `../` (i.e., going one level higher). If the corrected
path resolves to an existing file, the link is rewritten in place.

Special cases that cannot be auto-fixed are reported for manual review:
  - Links whose target does not exist at any plausible depth (the target
    file is missing entirely — e.g., forward-looking references to a
    research/reports/ archive that was never created).
  - Links whose target was renamed+relocated (the original target no
    longer exists, but a different sibling does).

Idempotent: running twice produces zero changes on the second run.
"""

from __future__ import annotations

import os
import re
import sys
from pathlib import Path
from urllib.parse import unquote

REPO_ROOT = Path(__file__).resolve().parent.parent
LINK_RE = re.compile(r'\]\(([^)]+)\)')
FENCED_RE = re.compile(r'```.*?```', re.DOTALL)
INLINE_CODE_RE = re.compile(r'`[^`]*`')

# Patterns we never rewrite (external URLs and other URL schemes).
EXTERNAL_PREFIXES = ('http://', 'https://', 'mailto:', 'ftp://',
                     'git://', 'ssh://', 'tel:')

# Forward-looking references to a research archive that was never created.
# Convert these from markdown links to plain text so they don't pretend
# to be resolvable.
DEAD_TARGET_HINTS = (
    'research/reports/',
)


def strip_code(text: str) -> str:
    """Remove fenced and inline code blocks so we don't rewrite example URLs."""
    text = FENCED_RE.sub('', text)
    text = INLINE_CODE_RE.sub('', text)
    return text


def link_target_exists(src_file: Path, target: str) -> bool:
    """Return True if `target` resolves to an existing file/dir from src_file's dir."""
    if not target or target.startswith('#'):
        return True  # in-file anchor or empty
    if target.startswith(EXTERNAL_PREFIXES):
        return True  # external, not checked
    # Strip any #fragment.
    path_part = target.split('#', 1)[0]
    if not path_part:
        return True
    candidate = (src_file.parent / path_part).resolve()
    return candidate.exists()


def fix_link_depth(src_file: Path, target: str) -> str | None:
    """Return a corrected target if `target`'s depth is wrong, else None.

    Strategy: original target doesn't resolve. Try inserting one more `../`
    (i.e., go one level higher). Repeat up to 5 times. If a deeper prefix
    resolves, return the corrected target string. Else return None.
    """
    if not target or target.startswith('#') or target.startswith(EXTERNAL_PREFIXES):
        return None
    path_part = target.split('#', 1)[0]
    fragment = target[len(path_part):]
    if not path_part:
        return None

    # If the original already resolves, no fix needed.
    if (src_file.parent / path_part).resolve().exists():
        return None

    # Try increasing depths until one resolves.
    candidate = path_part
    for _ in range(5):
        if candidate.startswith('../'):
            candidate = '../' + candidate
        elif candidate.startswith('./'):
            candidate = '../' + candidate[2:]
        else:
            candidate = '../' + candidate
        if (src_file.parent / candidate).resolve().exists():
            return candidate + fragment
    return None


def is_dead_target(target: str) -> bool:
    """Return True if the target is a known forward-looking reference."""
    return any(hint in target for hint in DEAD_TARGET_HINTS)


def fix_file(src_file: Path) -> tuple[int, int, list[str]]:
    """Rewrite broken links in src_file. Return (links_fixed, links_dead, list-of-manual)."""
    text = src_file.read_text(encoding='utf-8')
    stripped = strip_code(text)

    # Build a per-position fix map so we can rewrite the original text safely.
    fixes = {}  # span_start -> (span_end, new_target, reason)
    fixed_count = 0
    dead_count = 0
    manual: list[str] = []

    for m in LINK_RE.finditer(stripped):
        target = unquote(m.group(1))
        if link_target_exists(src_file, target):
            continue

        if is_dead_target(target):
            # Convert to plain text — strip the markdown link wrapper.
            # Find the original span in the un-stripped text. Because we
            # only stripped code spans, the offsets are stable.
            # The link span is `[text](target)` — we need to find it.
            # For simplicity, we re-scan the original text around m.start()
            # for the matching `[`.
            fixes[m.start()] = (m.end(), None, 'dead_target')
            dead_count += 1
            continue

        fixed = fix_link_depth(src_file, target)
        if fixed is not None:
            # Strip the #fragment off the existing target, then re-add it.
            old_fragment = target[len(target.split('#', 1)[0]):]
            new_target = fixed + old_fragment
            fixes[m.start()] = (m.end(), new_target, 'depth_fix')
            fixed_count += 1
        else:
            manual.append(f"{src_file} :: {target}")

    if not fixes:
        return 0, 0, manual

    # Apply fixes. We need to find the `[text](target)` pattern in the
    # ORIGINAL (un-stripped) text and rewrite it. Because code-span
    # stripping only removed text (and didn't shift offsets in the original),
    # we can use the stripped-text offsets directly to slice the original.

    # Actually — code-span stripping DID shrink the text, so offsets in
    # `stripped` do not match offsets in `text`. We need to re-scan the
    # original text for the same patterns.
    new_text = text
    # Walk original text and apply fixes by re-matching.
    # Simplest correct approach: replace `[text](old_target)` with
    # `[text](new_target)` for each broken link we found.

    # Re-parse the original text for all markdown links, find their spans.
    # Because we're going to rewrite using string-replace, we need to be
    # careful about duplicates. Use position-based replacement.
    # Build a sorted list of (start, end, new_target_or_None, reason).
    link_spans = []
    pos = 0
    for m in re.finditer(r'\[([^\]]*)\]\(([^)]+)\)', text):
        link_spans.append((m.start(), m.end(), m.group(1), m.group(2)))

    # For each broken target, find a matching span and rewrite.
    out = []
    last = 0
    for start, end, link_text, target in link_spans:
        if link_target_exists(src_file, target):
            out.append(text[last:end])
            last = end
            continue
        if is_dead_target(target):
            # Convert `[text](target)` → `text` (plain text, no link).
            out.append(text[last:start])
            out.append(link_text)
            last = end
            continue
        fixed = fix_link_depth(src_file, target)
        if fixed is not None:
            old_fragment = target[len(target.split('#', 1)[0]):]
            new_target = fixed + old_fragment
            out.append(text[last:start])
            out.append(f'[{link_text}]({new_target})')
            last = end
        else:
            out.append(text[last:end])
            last = end
            manual.append(f"{src_file} :: {target}")
    out.append(text[last:])
    new_text = ''.join(out)

    if new_text != text:
        src_file.write_text(new_text, encoding='utf-8')

    return fixed_count, dead_count, manual


def main() -> int:
    md_files = []
    for root, dirs, files in os.walk(REPO_ROOT):
        # Skip .git, build, scripts, node_modules
        dirs[:] = [d for d in dirs if d not in ('.git', 'build', 'node_modules', 'target', '__pycache__')]
        for f in files:
            if f.endswith('.md'):
                md_files.append(Path(root) / f)
    md_files.sort()

    total_fixed = 0
    total_dead = 0
    all_manual = []
    for f in md_files:
        rel = f.relative_to(REPO_ROOT)
        try:
            fixed, dead, manual = fix_file(f)
        except Exception as e:
            print(f"  ERROR: {rel}: {e}", file=sys.stderr)
            continue
        if fixed or dead:
            print(f"  {rel}: {fixed} link(s) depth-fixed, {dead} dead link(s) converted to plain text")
        total_fixed += fixed
        total_dead += dead
        all_manual.extend(manual)

    print()
    print(f"Total: {total_fixed} link depth fixes applied, {total_dead} dead links converted to plain text.")
    if all_manual:
        print(f"\n{len(all_manual)} link(s) require MANUAL review (could not auto-fix):")
        for m in all_manual:
            print(f"  {m}")
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())

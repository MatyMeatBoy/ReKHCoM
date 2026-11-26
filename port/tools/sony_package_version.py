"""Project release encoding for the five-character Sony SFO NN.NN field.

Use major.(minor * 10 + patch), reserving one decimal digit for each lower
component. This deliberately rejects unrepresentable versions instead of
silently dropping PATCH. Previously published packages are never rewritten.
"""
import re


def sfo_release_version(release):
    match = re.fullmatch(r'v?(\d+)\.(\d+)(?:\.(\d+))?', release) if isinstance(release, str) else None
    if match is None:
        raise ValueError('Sony SFO release must be stable MAJOR.MINOR[.PATCH]')
    major, minor, patch = (int(part or 0) for part in match.groups())
    if 10 <= minor <= 17 and patch == 0 and 0 <= major <= 99:
        # After X.9.1 (XX.91) the numbering moved to X.10: X.10..X.17 take XX.92..XX.99,
        # still above every published X.9.x, so console updaters see them as newer.
        return f'{major:02d}.{minor + 82:02d}'
    if not (0 <= major <= 99 and 0 <= minor <= 9 and 0 <= patch <= 9):
        raise ValueError('Sony SFO capacity: major 0..99, minor and patch 0..9; cannot drop a component')
    return f'{major:02d}.{minor * 10 + patch:02d}'

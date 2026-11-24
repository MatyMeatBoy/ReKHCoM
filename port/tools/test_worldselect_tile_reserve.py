"""The parked world pictures must stay hidden throughout the tutorial."""

from pathlib import Path

source = (Path(__file__).resolve().parents[2] / 'src/menu/mode_worldselect.c').read_text()
assert 'sWorldselectSlotCount >= 4' in source
assert 'for (int i = 2; i < sWorldselectSlotCount - 1; i++)' in source
assert 'if ((u8)(facing - 65) <= 126)' in source

for count in (4, 5):
    for slot in range(2, count - 1):
        angle = (256 // count * slot - 128) & 255
        for bob in range(-256, 257):
            facing = ((bob * 3 >> 7) + angle) & 255
            assert ((facing - 65) & 255) > 126, (count, slot, angle, bob, facing)

print('PASS: reserved pictures stay outside the drawn range for every bob value')

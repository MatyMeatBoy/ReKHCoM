"""Grade measured real-game frame clocks, not a fixed-FPS movie or a connection.

Run against a fresh public-relay benchmark directory containing PHASES.json,
frames.csv and the two logs. Original handheld cadence is59.7275Hz.
"""
import argparse
import csv
import json
from pathlib import Path
import re
import statistics

GBA_HZ = 59.7275


def grade(root, phase='baseline'):
    phases = json.loads((root/'PHASES.json').read_text())
    begin, end = phases[phase+'_begin'], phases[phase+'_end']
    report = {'phase': phase, 'target_hz': GBA_HZ, 'players': []}
    for i in range(2):
        with (root/f'p{i+1}/frames.csv').open() as stream:
            rows = list(csv.DictReader(stream))
        rows = rows[begin['rows'][i]:end['rows'][i]]
        intervals = sorted(float(row['interval_ms']) for row in rows)
        log = end['logs'][i]
        checks = re.findall(r'netlink: (\d+) state checks, (\d+) desync', log)
        battles = re.findall(r'netlink: battle (\d+) frames, (\d+) waited', log)
        fps = 1000/statistics.mean(intervals)
        player = {'player': i+1, 'frames': len(rows), 'measured_seconds': sum(intervals)/1000,
                  'fps': fps, 'speed_percent': fps/GBA_HZ*100,
                  'p99_ms': intervals[int(len(intervals)*.99)], 'max_ms': max(intervals),
                  'intervals_over25ms': sum(x>25 for x in intervals),
                  'intervals_ge50ms': sum(x>=50 for x in intervals),
                  'state_checks': int(checks[-1][0]) if checks else 0,
                  'desync_count': int(checks[-1][1]) if checks else None,
                  'battle_waited_frames': int(battles[-1][1]) if battles else None}
        # Require the actual simulation clock to stay within0.1% of its original
        # rate, not merely the display counter rounding to60. Exclude startup,
        # connection menus and deliberate teardown using measured phase bounds.
        player['pass'] = len(rows)>=1200 and fps>=GBA_HZ*.999 and player['p99_ms']<25 and \
                         player['max_ms']<50 and player['state_checks']>=10 and \
                         player['desync_count']==0 and player['battle_waited_frames']==0 and \
                         'netlink: DESYNC' not in log and 'mode: mode_sioError' not in log
        report['players'].append(player)
    report['pass'] = all(p['pass'] for p in report['players'])
    return report


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--run', type=Path, required=True)
    ap.add_argument('--phase', default='baseline', choices=['baseline','record'])
    a = ap.parse_args()
    report = grade(a.run, a.phase)
    (a.run/('FULLSPEED-'+a.phase+'.json')).write_text(json.dumps(report, indent=2))
    print(json.dumps(report, indent=2))
    return 0 if report['pass'] else 1


if __name__ == '__main__':
    raise SystemExit(main())

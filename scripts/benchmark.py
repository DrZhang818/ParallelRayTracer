#!/usr/bin/env python3
"""Run repeatable performance experiments and write CSV results.

Example:
  python3 scripts/benchmark.py --exe build/raytracer --scene scenes/many_spheres.scene
"""
import argparse
import csv
import os
import re
import subprocess
import sys
from pathlib import Path


def parse_output(text):
    data = {}
    for line in text.splitlines():
        if '=' in line:
            key, value = line.strip().split('=', 1)
            data[key] = value
    return data


def run_case(exe, scene, out_dir, width, height, spp, depth, threads, use_bvh, scheduler):
    out = out_dir / f"bench_t{threads}_{'bvh' if use_bvh else 'nobvh'}_{scheduler}.ppm"
    cmd = [
        exe,
        '--scene', scene,
        '--output', str(out),
        '--width', str(width),
        '--height', str(height),
        '--spp', str(spp),
        '--depth', str(depth),
        '--threads', str(threads),
        '--scheduler', scheduler,
        '--tile', '16',
    ]
    if not use_bvh:
        cmd.append('--no-bvh')
    proc = subprocess.run(cmd, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if proc.returncode != 0:
        print(proc.stdout)
        print(proc.stderr, file=sys.stderr)
        raise RuntimeError(f"command failed: {' '.join(cmd)}")
    info = parse_output(proc.stdout)
    return {
        'threads': threads,
        'use_bvh': use_bvh,
        'scheduler': scheduler,
        'width': width,
        'height': height,
        'spp': spp,
        'depth': depth,
        'primitives': info.get('primitives', '').split()[0],
        'bvh_build_seconds': info.get('bvh_build_seconds', ''),
        'render_seconds': info.get('render_seconds', ''),
        'output': str(out),
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--exe', default='build/raytracer')
    parser.add_argument('--scene', default='scenes/many_spheres.scene')
    parser.add_argument('--out-dir', default='results/bench')
    parser.add_argument('--width', type=int, default=400)
    parser.add_argument('--height', type=int, default=225)
    parser.add_argument('--spp', type=int, default=4)
    parser.add_argument('--depth', type=int, default=4)
    parser.add_argument('--threads', default='1,2,4,8')
    parser.add_argument('--schedulers', default='dynamic')
    parser.add_argument('--skip-nobvh', action='store_true')
    args = parser.parse_args()

    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    threads = [int(x) for x in args.threads.split(',') if x]
    schedulers = [x for x in args.schedulers.split(',') if x]

    rows = []
    for scheduler in schedulers:
        for t in threads:
            rows.append(run_case(args.exe, args.scene, out_dir, args.width, args.height, args.spp, args.depth, t, True, scheduler))
        if not args.skip_nobvh:
            # Brute force is expensive; run it only on one thread and max requested threads.
            for t in sorted(set([1, threads[-1]])):
                rows.append(run_case(args.exe, args.scene, out_dir, args.width, args.height, args.spp, args.depth, t, False, scheduler))

    csv_path = out_dir / 'benchmark.csv'
    with csv_path.open('w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)

    print(f'wrote {csv_path}')
    for row in rows:
        print(row)


if __name__ == '__main__':
    main()

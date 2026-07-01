#!/usr/bin/env bash
set -euo pipefail
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
mkdir -p results
./build/raytracer --scene scenes/spheres.scene --output results/spheres.ppm --width 400 --height 225 --spp 8 --depth 6 --threads 4
./build/raytracer --scene scenes/mesh_demo.scene --output results/mesh_demo.ppm --width 400 --height 225 --spp 8 --depth 6 --threads 4
python3 scripts/benchmark.py --exe build/raytracer --scene scenes/many_spheres.scene --width 320 --height 180 --spp 2 --depth 3 --threads 1,2,4 --skip-nobvh

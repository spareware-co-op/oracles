#!/usr/bin/env python3
"""Cherchi, Pellacini, Attene and Livesu's booleans, for the Spare Kit geometry kernel's mesh jobs.

One JSON job per line on standard input, as `oracle jobs cherchi` writes them; one JSON answer per
line on standard output. The program reads and writes OBJ files, in decimals, so its result is
rounded where it leaves the program: `tolerance` states that rounding, the arithmetic inside being
exact. A job it refuses or fails on answers `"volume": null`.
"""
import json
import math
import subprocess
import sys
import tempfile
from pathlib import Path

PROGRAM = "/src/build/mesh_booleans"
# OBJ's written places.
TOLERANCE = 1e-6
OPS = {"union": "union", "intersection": "intersection", "difference": "subtraction"}


def write_obj(path, solid):
    lines = [f"v {x} {y} {z}" for x, y, z in solid["vertices"]]
    lines += [f"f {a + 1} {b + 1} {c + 1}" for a, b, c in solid["triangles"]]
    path.write_text("\n".join(lines) + "\n")


def read_obj(path):
    vertices, triangles = [], []
    for line in path.read_text().splitlines():
        parts = line.split()
        if parts[:1] == ["v"]:
            vertices.append([float(x) for x in parts[1:4]])
        elif parts[:1] == ["f"]:
            corners = [int(p.split("/")[0]) - 1 for p in parts[1:]]
            triangles += [
                [vertices[corners[0]], vertices[corners[k]], vertices[corners[k + 1]]]
                for k in range(1, len(corners) - 1)
            ]
    return triangles


def minus(a, b):
    return [p - q for p, q in zip(a, b)]


def cross(a, b):
    return [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]]


def measured(triangles):
    volume = area = 0.0
    for a, b, c in triangles:
        turned = cross(minus(b, a), minus(c, a))
        area += math.sqrt(sum(x * x for x in turned)) / 2
        volume += sum(p * q for p, q in zip(a, cross(b, c))) / 6
    return volume, area


def answer_of(job):
    answer = {"id": job["id"], "volume": None}
    with tempfile.TemporaryDirectory() as folder:
        folder = Path(folder)
        first, second, result = folder / "a.obj", folder / "b.obj", folder / "r.obj"
        write_obj(first, job["a"])
        write_obj(second, job["b"])
        ran = subprocess.run(
            [PROGRAM, OPS[job["op"]], str(first), str(second), str(result)],
            capture_output=True,
            timeout=600,
            check=False,
        )
        if ran.returncode != 0 or not result.exists():
            return answer
        triangles = read_obj(result)
    volume, area = measured(triangles)
    answer.update(
        volume=repr(volume),
        area=repr(area),
        tolerance=repr(TOLERANCE),
        triangles=[[[repr(x) for x in corner] for corner in t] for t in triangles],
    )
    return answer


def main():
    for line in sys.stdin:
        print(json.dumps(answer_of(json.loads(line))), flush=True)


if __name__ == "__main__":
    main()

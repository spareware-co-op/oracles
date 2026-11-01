#!/usr/bin/env python3
"""geomdl's answers to the Spare Kit geometry kernel's NURBS jobs.

One JSON job per line on standard input, as `oracle jobs geomdl` writes them; one JSON answer per
line on standard output, every double written by repr, which reads back to the same bits. A job
geomdl refuses answers `"values": null`: Python's own errors on a job it cannot evaluate, and
geomdl's GeomdlException, which it raises for a knot inserted more times than the degree allows
(the kernel refuses that insertion too, so both refusing agree).
"""
import json
import sys

from geomdl import BSpline, NURBS, helpers, operations
from geomdl.exceptions import GeomdlException


def curve_of(job):
    knots = [float(k) for k in job["knots"]]
    points = [[float(x) for x in p] for p in job["points"]]
    if job["weights"] is None:
        curve = BSpline.Curve(normalize_kv=False)
    else:
        curve = NURBS.Curve(normalize_kv=False)
    curve.degree = job["degree"]
    curve.ctrlpts = points
    if job["weights"] is not None:
        curve.weights = [float(w) for w in job["weights"]]
    curve.knotvector = knots
    return curve


def values_of(job):
    at = float(job["at"])
    knots = [float(k) for k in job["knots"]]
    if job["op"] == "basis":
        degree = job["degree"]
        span = helpers.find_span_linear(degree, knots, len(job["points"]), at)
        return helpers.basis_function(degree, knots, span, at)
    curve = curve_of(job)
    if job["op"] == "point":
        return curve.evaluate_single(at)
    operations.insert_knot(curve, [at], [1])
    points = curve.ctrlpts if job["weights"] is None else [p[:-1] for p in curve.ctrlptsw]
    return list(curve.knotvector) + [x for p in points for x in p]


def main():
    for line in sys.stdin:
        job = json.loads(line)
        try:
            values = [repr(float(x)) for x in values_of(job)]
        except (
            GeomdlException, ValueError, IndexError, ZeroDivisionError, TypeError, AttributeError
        ):
            values = None
        print(json.dumps({"id": job["id"], "values": values}))


if __name__ == "__main__":
    main()

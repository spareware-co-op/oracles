//! fiksi's answers to the Spare Kit geometry kernel's sketch jobs.
//!
//! One JSON job per line on standard input, as `oracle jobs fiksi` writes them: its `points` (where
//! each starts) and `rules` on them by place, only of kinds fiksi states. Each job is built as a
//! fiksi system and solved with its default options. One JSON answer per line on standard output:
//! the `id`, and `points` as an `[x, y]` pair of decimal strings for each point, or `null` where a
//! rule has no fiksi form or fiksi panicked.
//!
//! The kernel's level and plumb lines become lines parallel to a fixed axis as long as the sketch is
//! wide; its angle in half-turns
//! is fiksi's in radians, both from the first line to the second. Its point-line distance, positive
//! on the left, is fiksi's as it stands: fiksi's residual is the cross product of the line's direction
//! with the point's offset over the line's length (`constraints/expressions.rs`), positive on the left
//! when y points up; its documentation's "negative on the left" reads y as pointing down.

use std::io::{BufRead, Write};
use std::panic;

use fiksi::{System, constraints, elements};
use serde_json::{Value, json};

fn number(value: &Value) -> Option<f64> {
    value.as_str()?.parse::<f64>().ok().filter(|x| x.is_finite())
}

fn index(value: &Value) -> Option<usize> {
    usize::try_from(value.as_u64()?).ok()
}

fn solved(job: &Value) -> Option<Vec<[String; 2]>> {
    let mut s = System::new();
    let mut points = Vec::new();
    for p in job["points"].as_array()? {
        points.push(elements::Point::create(&mut s, number(&p[0])?, number(&p[1])?));
    }
    let point = |rule: &Value, key: &str| points.get(index(&rule[key])?).copied();
    // fiksi's parallelism residual is the cross product of the two directions, unnormalised, so
    // an axis line as long as the sketch is wide holds a level or plumb line as firmly as the
    // sketch's own lines hold each other; a unit axis would let it tilt.
    let reach = job["points"]
        .as_array()?
        .iter()
        .flat_map(|p| [number(&p[0]), number(&p[1])])
        .try_fold(1.0_f64, |m, x| Some(m.max(x?.abs())))?;
    let mut axes = None;
    for rule in job["rules"].as_array()? {
        let quad = |rule: &Value, key: &str| -> Option<[fiksi::ElementHandle<elements::Point>; 4]> {
            let l = rule[key].as_array()?;
            Some([
                points.get(index(l.first()?)?).copied()?,
                points.get(index(l.get(1)?)?).copied()?,
                points.get(index(l.get(2)?)?).copied()?,
                points.get(index(l.get(3)?)?).copied()?,
            ])
        };
        match rule["kind"].as_str()? {
            "fix" => point(rule, "point")?.fix(&mut s),
            "distance" => {
                let (a, b) = (point(rule, "a")?, point(rule, "b")?);
                constraints::PointPointDistance::create(&mut s, a, b, number(&rule["value"])?);
            }
            kind @ ("level" | "plumb") => {
                let (x_axis, y_axis) = *axes.get_or_insert_with(|| {
                    let o = elements::Point::create(&mut s, 0.0, 0.0);
                    let x = elements::Point::create(&mut s, reach, 0.0);
                    let y = elements::Point::create(&mut s, 0.0, reach);
                    for p in [o, x, y] {
                        p.fix(&mut s);
                    }
                    (
                        elements::Line::create(&mut s, o, x),
                        elements::Line::create(&mut s, o, y),
                    )
                });
                let line = elements::Line::create(&mut s, point(rule, "a")?, point(rule, "b")?);
                let axis = if kind == "level" { x_axis } else { y_axis };
                constraints::LineLineParallelism::create(&mut s, line, axis);
            }
            kind @ ("parallel" | "perpendicular" | "angle") => {
                let [a, b, c, d] = quad(rule, "lines")?;
                let (first, second) = (
                    elements::Line::create(&mut s, a, b),
                    elements::Line::create(&mut s, c, d),
                );
                match kind {
                    "parallel" => {
                        constraints::LineLineParallelism::create(&mut s, first, second);
                    }
                    "perpendicular" => {
                        constraints::LineLinePerpendicularity::create(&mut s, first, second);
                    }
                    _ => {
                        let angle = number(&rule["half_turns"])? * std::f64::consts::PI;
                        constraints::LineLineAngle::create(&mut s, first, second, angle);
                    }
                }
            }
            "from_line" => {
                let l = rule["line"].as_array()?;
                let (a, b) = (
                    points.get(index(l.first()?)?).copied()?,
                    points.get(index(l.get(1)?)?).copied()?,
                );
                let line = elements::Line::create(&mut s, a, b);
                let p = point(rule, "point")?;
                constraints::PointLineDistance::create(&mut s, p, line, number(&rule["signed"])?);
            }
            "equal_length" => {
                let [a, b, c, d] = quad(rule, "lines")?;
                constraints::SegmentSegmentLengthEquality::create(&mut s, a, b, c, d);
            }
            _ => return None,
        }
    }
    s.solve(fiksi::SolvingOptions::default());
    Some(
        points
            .iter()
            .map(|p| {
                let at = p.get_value(&s);
                [at.x.to_string(), at.y.to_string()]
            })
            .collect(),
    )
}

fn main() {
    // A panic in the solver answers null; its message still goes to the log.
    let stdin = std::io::stdin();
    let mut out = std::io::stdout().lock();
    for line in stdin.lock().lines() {
        let line = line.expect("standard input is readable");
        let job: Value = serde_json::from_str(&line).expect("each job is one JSON line");
        let points = panic::catch_unwind(|| solved(&job)).ok().flatten();
        let answer = json!({ "id": job["id"], "points": points });
        writeln!(out, "{answer}").expect("standard output is writable");
    }
}

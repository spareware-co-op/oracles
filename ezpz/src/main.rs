//! ezpz's answers to the Spare Kit geometry kernel's sketch jobs.
//!
//! One JSON job per line on standard input, as `oracle jobs ezpz` writes them: its `points` (where
//! each starts) and `rules` on them by place. Each job is written in ezpz's own text problem form and
//! solved there. One JSON answer per line on standard output: the `id`, and `points` as an `[x, y]`
//! pair of decimal strings for each point, or `null` where ezpz refused the problem, could not solve
//! it, or panicked (its library paths can: the kernel's chapter 15.16 names them).
//!
//! The forms are matched on ezpz's source at this release (`textual/parser.rs`, `constraints.rs`):
//! `vertical` holds x, `lines_at_angle` turns the first line onto the second, and
//! `point_line_distance` is positive on the left of the line, as the kernel's rules are.

use std::fmt::Write as _;
use std::io::{BufRead, Write};
use std::panic;

use ezpz::textual::Problem;
use serde_json::{Value, json};

fn number(value: &Value) -> Option<f64> {
    value.as_str()?.parse::<f64>().ok().filter(|x| x.is_finite())
}

fn index(value: &Value) -> Option<usize> {
    usize::try_from(value.as_u64()?).ok()
}

fn label(i: usize) -> String {
    format!("p{i}")
}

fn labels(list: &Value) -> Option<Vec<String>> {
    list.as_array()?.iter().map(|v| index(v).map(label)).collect()
}

/// The job in ezpz's text problem form.
fn problem(job: &Value) -> Option<String> {
    let starts: Vec<(f64, f64)> = job["points"]
        .as_array()?
        .iter()
        .map(|p| Some((number(&p[0])?, number(&p[1])?)))
        .collect::<Option<_>>()?;
    let mut text = String::from("# constraints\n");
    for i in 0..starts.len() {
        writeln!(text, "point {}", label(i)).ok()?;
    }
    let mut lines = Vec::new();
    for rule in job["rules"].as_array()? {
        let p = |key: &str| index(&rule[key]).map(label);
        let line = match rule["kind"].as_str()? {
            "fix" => {
                let i = index(&rule["point"])?;
                let (x, y) = *starts.get(i)?;
                format!("{0}.x = {x}\n{0}.y = {y}", label(i))
            }
            "distance" => format!("distance({}, {}, {})", p("a")?, p("b")?, number(&rule["value"])?),
            "level" => format!("horizontal({}, {})", p("a")?, p("b")?),
            "plumb" => format!("vertical({}, {})", p("a")?, p("b")?),
            "parallel" => format!("parallel({})", labels(&rule["lines"])?.join(", ")),
            "perpendicular" => format!("perpendicular({})", labels(&rule["lines"])?.join(", ")),
            "angle" => format!(
                "lines_at_angle({}, {}deg)",
                labels(&rule["lines"])?.join(", "),
                number(&rule["half_turns"])? * 180.0
            ),
            "from_line" => {
                let l = labels(&rule["line"])?;
                format!(
                    "point_line_distance({}, {}, {}, {})",
                    p("point")?,
                    l.first()?,
                    l.get(1)?,
                    number(&rule["signed"])?
                )
            }
            "midpoint" => format!("midpoint({}, {}, {})", p("a")?, p("b")?, p("point")?),
            "mirror" => {
                let l = labels(&rule["line"])?;
                format!("symmetric({}, {}, {}, {})", l.first()?, l.get(1)?, p("a")?, p("b")?)
            }
            "equal_length" => format!("lines_equal_length({})", labels(&rule["lines"])?.join(", ")),
            _ => return None,
        };
        lines.push(line);
    }
    text.push_str(&lines.join("\n"));
    text.push_str("\n\n# guesses\n");
    let guesses: Vec<String> = starts
        .iter()
        .enumerate()
        .map(|(i, (x, y))| format!("{} roughly ({x}, {y})", label(i)))
        .collect();
    text.push_str(&guesses.join("\n"));
    text.push('\n');
    Some(text)
}

fn solved(job: &Value) -> Option<Vec<[String; 2]>> {
    let text = problem(job)?;
    let count = job["points"].as_array()?.len();
    let problem: Problem = text.parse().ok()?;
    let system = problem.to_constraint_system().ok()?;
    let outcome = system.solve().ok()?;
    (0..count)
        .map(|i| {
            let point = outcome.get_point(&label(i))?;
            Some([point.x.to_string(), point.y.to_string()])
        })
        .collect()
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

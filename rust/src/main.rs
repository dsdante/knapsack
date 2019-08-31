use std::io;
use std::io::BufRead;
extern crate num_cpus;

const DEFAULT_FILENAME: &str = "input"; // Can be overridden with an argument.
const PARALLELIZE: bool = true;

/// Recursive knapsack calculation routine
///   depth: the number of the items considered so far (the current recursion depth)
///   sum: the sum of the taken items among considered
///   mask: the bit mask of the taken items
fn knapsack(limit: f64, items: &Vec<f64>, depth: usize, sum: &mut f64, mask: &mut usize) {
    if depth == items.len() {
        return;
    }

    let mut sum_b = *sum + items[depth];
    if sum_b > limit {
        // Recursion pruning
        *mask <<= items.len() - depth;
        return;
    }
    *mask <<= 1;
    let mut mask_b = *mask | 0x1;

    knapsack(limit, items, depth + 1, sum, mask);
    knapsack(limit, items, depth + 1, &mut sum_b, &mut mask_b);
    if sum_b > *sum {
        *sum = sum_b;
        *mask = mask_b;
    }
}

/// Parallel version of the knapsack calculation routine
///   depth: the number of the items considered so far (the current recursion depth)
///   spawn_depth: the maximum depth of recursive thread spawning
///   sum: the sum of the taken items among considered
///   mask: the bit mask of the taken items
fn knapsack_parallel(
    limit: f64,
    items: &Vec<f64>,
    depth: usize,
    spawn_depth: usize,
    sum: &mut f64,
    mask: &mut usize,
) {
    if depth == items.len() {
        return;
    }
    if depth == spawn_depth {
        // No more branching.
        knapsack(limit, items, depth, sum, mask);
        return;
    }

    let mut sum_b = *sum + items[depth];
    if sum_b > limit {
        // Recursion pruning
        *mask <<= items.len() - depth;
        return;
    }
    *mask <<= 1;
    let mut mask_b = *mask | 0x1;

    // Spawn A, run B synchronously.
    crossbeam::scope(|sc| {
        sc.spawn(|_| knapsack_parallel(limit, items, depth + 1, spawn_depth, sum, mask));
        knapsack_parallel(
            limit,
            items,
            depth + 1,
            spawn_depth,
            &mut sum_b,
            &mut mask_b,
        );
    })
    .expect("Thread error");

    if sum_b > *sum {
        *sum = sum_b;
        *mask = mask_b;
    }
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let filename = std::env::args()
        .nth(1)
        .unwrap_or(String::from(DEFAULT_FILENAME));
    println!("Running '{}'...", filename);

    let limit: f64;
    let mut items: Vec<f64>;

    {
        let file = std::fs::File::open(&filename)?;
        let lines = io::BufReader::new(file).lines().flat_map(|l| l);
        let mut numbers = lines
            .flat_map(|l| l.parse::<f64>())
            .filter(|n| n.is_finite());
        limit = numbers.next().ok_or(io::Error::new(
            io::ErrorKind::UnexpectedEof,
            "No numbers found in the input file",
        ))?;
        items = numbers.collect();
    } // Drop file.

    let max_items = 8 * std::mem::size_of::<usize>();
    if items.len() > max_items {
        Err(io::Error::new(
            io::ErrorKind::InvalidData,
            format!("{} items found; can use {} at most", items.len(), max_items),
        ))?;
    }
    // Keeping bigger items at the end allows pruning at the later stages,
    // thus balancing the recursion tree
    items.sort_unstable_by(|x, y| x.partial_cmp(y).unwrap()); // No NaNs by now.

    let mut sum = 0.0;
    let mut mask = 0x0;
    let spawn_depth = match PARALLELIZE {
        false => 0,
        true => num_cpus::get().next_power_of_two().trailing_zeros() as usize,
    };
    knapsack_parallel(limit, &items, 0, spawn_depth, &mut sum, &mut mask);

    println!("Sum: {} / {}", sum, limit);
    println!("Used items: {} / {}", mask.count_ones(), items.len());
    for i in items.iter().rev() {
        if mask & 0x1 != 0 {
            println!("{:.9}", i);
        }
        mask >>= 1;
    }

    Ok(())
}

# 🚀 Heistogram: Blazing Fast Histograms for Quantile Estimation 🚀

[![License](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)
[![Single Header](https://img.shields.io/badge/Single%20Header-Yes-brightgreen.svg)](.)
[![Zero Dependencies](https://img.shields.io/badge/Dependencies-None-brightgreen.svg)](.)
[![C/C++](https://img.shields.io/badge/Language-C/C++-orange.svg)](.)

**Stop wasting time on slow histograms! Heistogram is a revolutionary C library that redefines quantile estimation, delivering *unprecedented speed and efficiency* for latency-sensitive applications, database analytics, and beyond.**

Imagine querying percentiles directly from serialized data, merging histograms in the blink of an eye, and achieving all this with a tiny memory footprint.  That's Heistogram.

## ✨ Why Heistogram is a Game Changer ✨

As developers dealing with latency data, performance monitoring, or large datasets, we know histograms are crucial.  But existing solutions like T-Digest and Q-Digest often fall short when you need:

* **Raw Speed:** Millisecond latency for percentile queries is ancient history. We're talking *nanoseconds*.
* **Minimal Footprint:**  Bloated histograms are a no-go for databases and resource-constrained environments.  We need compact, efficient storage.
* **Database Integration:** Deserializing histograms *before* querying in a database?  Seriously inefficient.  We deserve better.

**Heistogram delivers the answer.  Here's what makes it stand out:**

* **🚀  Unrivaled Performance:**  **Up to 142x faster merges, 13x faster queries, and 14x faster insertions** compared to T-Digest. Benchmarks don't lie (see below!).
* **💾  Extreme Space Efficiency:**  Serialized histograms are incredibly compact, scaling *sublinearly* with data range. Perfect for persistence and memory-limited systems.
* **⚡️  Serialized Query Magic:** **Query percentiles DIRECTLY from serialized histograms.**  No more deserialization bottlenecks, especially crucial for databases. This is a first!
* **📦  Single-Header Simplicity:**  Just drop `heistogram.h` into your project. **Zero dependencies.**  Integration couldn't be easier.
* **📏  Mathematically Sound:**  Proven upper error bound.  Reliability you can trust.
* **🎯  Optimized for Latency Data:**  Specifically designed for integer-based latency measurements (microseconds, nanoseconds).

**In short, Heistogram isn't just *another* histogram library. It's a fundamentally better way to handle quantile estimation.**

## 🛠 Core Features -  Under the Hood Awesomeness

Let's dive a bit deeper into what makes Heistogram tick:

* **Revolutionary Serialized Percentile Queries:**
    * **No Deserialization Tax:** Query directly on serialized data.  Ideal for database storage and retrieval.
    * **3.4x Faster Queries on Serialized Data:**  Benchmark proven performance boost.
    * **Database Friendly:**  Reduces memory usage, CPU load, and query latency within database systems.

* **Mathematically Optimized Algorithms for Speed:**
    * **Logarithmic Bucket Placement:**  Dynamic, efficient bucket allocation avoids costly re-balancing.
    * **Fast-Path Computations:**  Bit manipulation and integer arithmetic where others use slower floating-point operations.
    * **Minimal Memory Operations:**  Cache-friendly data structures to maximize performance.

* **Merge Powerhouse:**
    * **142x Faster Merges:**  Unleash the power of distributed data analysis.
    * **Real-time Aggregation:**  Merge histograms from multiple sources in microseconds.
    * **Dimensional Analysis:**  Combine histograms across various dimensions (time, region, user segment) for powerful insights.

* **Single-Header & Zero Dependencies:**
    * **`heistogram.h` is all you need.**  Include and go!
    * **No external libraries to wrestle with.**  Clean, simple integration.
    * **Maximum portability.**  Works anywhere C/C++ compiles.

## ⚙️ Performance Benchmarks - Numbers Don't Lie!

We rigorously benchmarked Heistogram against T-Digest (using RedisBloom's implementation) with a 10 million point log-normal distribution (mu = 0.0 and sigma=0.7) – simulating real-world latency data.  The results speak for themselves:

| Operation         | T-Digest Latency (ns) | Heistogram Latency (ns) | Heistogram Advantage |
|-----------------|----------------------|-------------------------|----------------------|
| **Insert**        | 669.205              | 12.922                 | **51x faster**       |
| **Merge**         | 63,769.003           | 199.27                 | **318x faster**      |
| **Quantile Query** | 651.503              | 37.198                  | **17x faster**       |
| **Serialized Query**| N/A                 | 89.651                 | **Unique Capability, 8x faster vs. Deserialize-then-Query** |

**Serialized Query Performance:**

| Operation                        | Time (ns) | Advantage        |
|---------------------------------|----------|-----------------|
| Heistogram serialize then query  | 744.934  |                 |
| Heistogram query serialized data | 89.651   | **8x faster** |

**Space Efficiency (Serialized Size vs. Data Spread):**

| Data Spread | Serialized Size |
|-------------|-----------------|
| 10²         | 278 bytes       |
| 10³         | 615 bytes       |
| 10⁴         | 891 bytes     |
| 10⁵         | 1,002 bytes     |
| 10⁶         | 1,120 bytes     |

**Heistogram isn't just faster in benchmarks; it translates to real-world application improvements.**  From database analytics to live benchmarking and streaming telemetry, Heistogram delivers consistent performance gains.

## 🚀 Getting Started -  It's a Breeze!

1. **Grab the Header:** Download `heistogram.h` and place it in your project.

2. **Include:** heistogram.h and you're good to go.

```c
#include <stdio.h>
#include "heistogram.h"

int main() {
    heistogram_t *h = heistogram_create(); 

    for (int i = 0; i < 1000; ++i) {
        heistogram_insert(h, i);
    }

    double p90 = heistogram_percentile(h, 90.0);
    printf("90th percentile: %f\n", p90); // Output will be close to 900

    size_t serialized_size;
    void *serialized_data = heistogram_serialize(h, &serialized_size);

    // ... later, maybe in a database ...

    heistogram_t *h_deserialized = heistogram_deserialize(serialized_data, serialized_size);
    double p99_serialized = heistogram_percentile(h_deserialized, 99.0);
    printf("99th percentile from serialized: %f\n", p99_serialized); // Output close to 990

    // or for a much faster result
    double p99_direct_from_serialized = histogram_percentile_serialized(seiralized_data, 99.0);
    printf("99th percentile directly from serialized data: %f\n", p99_direct_from_serialized); // Output close to 990

    heistogram_free(h);
    heistogram_free(h_deserialized);
    free(serialized_data);

    return 0;
}
```

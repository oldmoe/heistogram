#include <math.h>
#include <time.h>
#include <sys/time.h>
#include "../src/heistogram.h"

static uint64_t BENCH_ITERATIONS = 1000000;

// Utility function to get current time in microseconds
static uint64_t get_microseconds() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000ULL + tv.tv_usec;
}

// Utility function to generate random numbers
static void generate_random_array(uint64_t* arr, size_t size) {
    for (size_t i = 0; i < size; i++) {
        arr[i] = rand() % (uint64_t)(pow(10, 6) + 1); 
    }
}

static void generate_uniform_array(uint64_t* arr, size_t size) {
    for (size_t i = 0; i < size; i++) {
        // For true uniform distribution, we need to handle the bias
        // that comes from RAND_MAX not being a multiple of our range
        uint64_t rand_val;
        uint64_t max_value = pow(10, 9); // 0 to 1M inclusive
        
        // Find the largest multiple of (max_value + 1) that is <= RAND_MAX
        uint64_t limit = RAND_MAX - (RAND_MAX % (max_value + 1));
        
        // Keep generating random numbers until we get one <= limit
        do {
            rand_val = rand();
        } while (rand_val > limit);
        
        // Now we can safely use modulo without bias
        arr[i] = rand_val % (uint64_t)(max_value + 1);
        //arr[i] = 1000;
    }
}

static void generate_lognormal_array(uint64_t* arr, size_t size) {
    // Parameters for the lognormal distribution
    // Parameters for the lognormal distribution - adjusted for more central peak
    double mu = 0.0; // Reduced from 0.5 to shift peak leftward
    double sigma = 0.7; // Reduced from 1.0 for less spread/skew
    uint64_t min_val = 0;
    uint64_t max_val = pow(10, 9);
    // Calculate the min and max values of the lognormal distribution before scaling
    double min_lognormal = (double)min_val;
    double max_lognormal = (double)max_val;
    
    for (size_t i = 0; i < size; i++) {
        // Generate a normally distributed random number
        double u1 = (double)rand() / RAND_MAX;
        double u2 = (double)rand() / RAND_MAX;
        
        // Box-Muller transform to get normal distribution
        double z = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
        
        // Convert to lognormal
        double lognormal = exp(mu + sigma * z);
        
        // Calculate midpoint of the range
        double range_midpoint = (min_val + max_val) / 2.0;
        
        // Scale and constrain the value to center around the midpoint
        double range = max_lognormal - min_lognormal;
        double normalized = (lognormal - exp(mu - 3*sigma)) / (exp(mu + 3*sigma) - exp(mu - 3*sigma));
        
        // Adjust the scaling to favor values closer to the center
        double constrained;
        if (normalized < 0.5) {
            // For the lower half, scale to go from min_val to midpoint
            constrained = min_lognormal + (normalized * 2.0) * (range_midpoint - min_lognormal);
        } else {
            // For the upper half, scale to go from midpoint to max_val
            constrained = range_midpoint + ((normalized - 0.5) * 2.0) * (max_lognormal - range_midpoint);
        }
        
        // Round to nearest integer and ensure it's within bounds
        double rounded = round(constrained);
        
        // Final bounds check to ensure we're exactly within range
        if (rounded < min_val) {
            rounded = min_val;
        } else if (rounded > max_val) {
            rounded = max_val;
        }
        
        arr[i] = (uint64_t)rounded;
    }
}
// Extend the BenchmarkResult structure
typedef struct {
    size_t data_size;
    float error_margin;
    uint64_t insert_time;
    uint64_t percentile_time;
    uint64_t percentiles_time;
    uint64_t serialize_time;
    uint64_t deserialize_time;
    size_t serialized_size;
    uint64_t serialized_percentile_time;
    uint64_t serialized_percentiles_time;
    uint64_t merge_time;
    uint64_t merge_inplace_time;
    uint64_t merge_serialized_time;
    uint64_t merge_inplace_serialized_time;
    uint64_t merge_two_serialized_time;
    uint64_t prank_time;
    double p50, p75, p90, p95, p99, p999, p9999;
    double m_p50, m_p75, m_p90, m_p95, m_p99, m_p999, m_p9999;
    double s_p50, s_p75, s_p90, s_p95, s_p99, s_p999, s_p9999;
    double sm_p50, sm_p75, sm_p90, sm_p95, sm_p99, sm_p999, sm_p9999;
} BenchmarkResult;

// Function to run a single benchmark
static BenchmarkResult run_benchmark(uint64_t* data, size_t size, float error_margin) {
    BenchmarkResult result = {
        .data_size = size,
        .error_margin = error_margin
    };

    printf("Benchmarking inserts .. ");
    // Measure insertion time
    uint64_t start = get_microseconds();
    Heistogram* h = heistogram_create();
    for (size_t i = 0; i < size; i++) {
        heistogram_add(h, data[i]);
    }
    result.insert_time = get_microseconds() - start;
    printf("done\n");

    printf("Min value is %lu, max value is %lu, bucket_count is %u, min bucket id is %u\n", h->min, h->max, h->capacity, h->min_bucket_id);

    printf("Benchmarking percentile .. ");
    // Measure percentile calculations (1M times)
    start = get_microseconds();
    for (size_t i = 0; i < BENCH_ITERATIONS; i++) {
        result.p50 = heistogram_percentile(h, 50);
        result.p75 = heistogram_percentile(h, 75);
        result.p90 = heistogram_percentile(h, 90);
        result.p95 = heistogram_percentile(h, 95);
        result.p99 = heistogram_percentile(h, 99);
        result.p999 = heistogram_percentile(h, 99.9);
        result.p9999 = heistogram_percentile(h, 99.99);
    }
    result.percentile_time = get_microseconds() - start;
    printf("done\n");
    /*
    printf("Benchmarking percentiles .. ");
    // Measure multiple percentiles calculation (1M times)
    double percentiles[] = {50, 75, 90, 95, 99, 99.9, 99.99};
    double results[7];
    start = get_microseconds();
    for (size_t i = 0; i < BENCH_ITERATIONS; i++) {
        heistogram_percentiles(h, percentiles, 7, results);
    }
    result.percentiles_time = get_microseconds() - start;
    result.m_p50 = results[0];
    result.m_p75 = results[1];
    result.m_p90 = results[2];
    result.m_p95 = results[3];
    result.m_p99 = results[4];
    result.m_p999 = results[5];
    result.m_p9999 = results[6];
    printf("done\n");
    */
    printf("Benchmarking serialize .. ");

    // Measure serialization (10K times)
    size_t ser_size;
    void* serialized = NULL;
    start = get_microseconds();
    for (size_t i = 0; i < BENCH_ITERATIONS; i++) {
        if (serialized) free(serialized);
        serialized = heistogram_serialize(h, &ser_size);
    }
    result.serialize_time = get_microseconds() - start;
    result.serialized_size = ser_size;
    printf("done\n");
    printf("Benchmarking percentile_serialized .. ");

    // Measure serialized percentile calculations (1M times)
    start = get_microseconds();
    for (size_t i = 0; i < BENCH_ITERATIONS; i++) {
        result.s_p50 = heistogram_percentile_serialized(serialized, ser_size, 50);
        result.s_p75 = heistogram_percentile_serialized(serialized, ser_size, 75);
        result.s_p90 = heistogram_percentile_serialized(serialized, ser_size, 90);
        result.s_p95 = heistogram_percentile_serialized(serialized, ser_size, 95);
        result.s_p99 = heistogram_percentile_serialized(serialized, ser_size, 99);
        result.s_p999 = heistogram_percentile_serialized(serialized, ser_size, 99.9);
        result.s_p9999 = heistogram_percentile_serialized(serialized, ser_size, 99.99);
    }
    result.serialized_percentile_time = get_microseconds() - start;
    printf("done\n");
    /*
    printf("Benchmarking percentiles_serialized .. ");

    // Measure multiple serialized percentiles calculation (1M times)
    //start = get_microseconds();
    //for (size_t i = 0; i < BENCH_ITERATIONS; i++) {
        //heistogram_percentiles_serialized(serialized, ser_size, percentiles, 7, results);
    //}
    //result.serialized_percentiles_time = get_microseconds() - start;
    result.sm_p50 = results[0];
    result.sm_p75 = results[1];
    result.sm_p90 = results[2];
    result.sm_p95 = results[3];
    result.sm_p99 = results[4];
    result.sm_p999 = results[5];
    result.sm_p9999 = results[6];
    printf("done\n");
    */
    printf("Benchmarking deserialize .. ");

    // Measure deserialization (10K times)
    start = get_microseconds();
    for (size_t i = 0; i < BENCH_ITERATIONS; i++) {
        Heistogram* deserialized = heistogram_deserialize(serialized, ser_size);
        heistogram_free(deserialized);
    }
    result.deserialize_time = get_microseconds() - start;
    printf("done\n");
    printf("Benchmarking merge .. ");

    // Measure merge operation (10K times)
    Heistogram* h2 = heistogram_create();
    for (size_t i = 0; i < size/2; i++) {
        heistogram_add(h2, data[i]);
    }

    start = get_microseconds();
    for (size_t i = 0; i < BENCH_ITERATIONS; i++) {
        Heistogram* merged = heistogram_merge(h, h2);
        heistogram_free(merged);
    }
    result.merge_time = get_microseconds() - start;
    printf("done\n");
    printf("Benchmarking merge_inplace .. ");

    // Measure in-place merge operation (10K times)
    start = get_microseconds();
    for (size_t i = 0; i < BENCH_ITERATIONS; i++) {
        heistogram_merge_inplace(h, h2);
    }
    result.merge_inplace_time = get_microseconds() - start;
    printf("done\n");
    printf("Benchmarking merge_serialized .. ");

    // Measure merge with serialized data (10K times)
    start = get_microseconds();
    for (size_t i = 0; i < BENCH_ITERATIONS; i++) {
        Heistogram* merged = heistogram_merge_serialized(h, serialized, ser_size);
        heistogram_free(merged);
    }
    result.merge_serialized_time = get_microseconds() - start;
    printf("done\n");
    printf("Benchmarking merge_inplace_serialized .. ");

    // Measure in-place merge with serialized data (10K times)
    start = get_microseconds();
    for (size_t i = 0; i < BENCH_ITERATIONS; i++) {
        heistogram_merge_inplace_serialized(h, serialized, ser_size);
    }
    result.merge_inplace_serialized_time = get_microseconds() - start;
    printf("done\n");
    printf("Benchmarking merge_two_serialized .. ");

    // Measure merge of two serialized histograms (10K times)
    void* serialized2 = heistogram_serialize(h2, &ser_size);
    start = get_microseconds();
    for (size_t i = 0; i < BENCH_ITERATIONS; i++) {
        Heistogram* merged = heistogram_merge_two_serialized(serialized, ser_size, serialized2, ser_size);
        heistogram_free(merged);
    }
    result.merge_two_serialized_time = get_microseconds() - start;
    free(serialized2);
    printf("done\n");
    printf("Benchmarking merge_prank .. ");

    // Measure prank calculation (1M times)
    start = get_microseconds();
    for (size_t i = 0; i < BENCH_ITERATIONS; i++) {
        heistogram_prank(h, data[i % size]);
    }
    result.prank_time = get_microseconds() - start;
    printf("done\n");

    // Cleanup
    if (serialized) free(serialized);
    //heistogram_free(h);
    //heistogram_free(h2);

    return result;
}

// Function to print results
static void print_results(const BenchmarkResult* result) {
    printf("\nBenchmark Results for size=%zu, error_margin=%.3f\n",
           result->data_size, result->error_margin);
    printf("----------------------------------------\n");

    printf("Insertion:\n");
    printf("  Time: %.3f ms (%.3f ns per operation)\n",
           result->insert_time / 1000.0,
           (double)result->insert_time * 1000 / (result->data_size));

    printf("\nPercentile Calculation (%lu iterations):\n", 7 * BENCH_ITERATIONS);
    printf("  Total Time: %.3f ms (%.3f ns per operation)\n",
           result->percentile_time / 1000.0,
           (result->percentile_time * 1000.0) / (7 * BENCH_ITERATIONS));
    printf("  Values: p50=%.2f, p75=%.2f, p90=%.2f, p95=%.2f, p99=%.2f, p99.9=%.2F, p99.99=%.2F\n",
           result->p50, result->p75, result->p90, result->p95, result->p99, result->p999, result->p9999);
    /*
    printf("\nMultiple Percentiles Calculation (%lu iterations):\n", 7 * BENCH_ITERATIONS);
    printf("  Total Time: %.3f ms (%.3f ns per operation)\n",
           result->percentiles_time / 1000.0,
           (result->percentiles_time * 1000.0) / (7 * BENCH_ITERATIONS));
           printf("  Values: p50=%.2f, p75=%.2f, p90=%.2f, p95=%.2f, p99=%.2F, p99.9=%.2F, p99.99=%.2F\n",
            result->m_p50, result->m_p75, result->m_p90, result->m_p95, result->m_p99, result->m_p999, result->m_p9999);
    */
    printf("\nMerge Operation (%lu iterations):\n", BENCH_ITERATIONS);
    printf("  Total Time: %.3f ms (%.3f ns per operation)\n",
            result->merge_time / 1000.0,
            (double)result->merge_time * 1000 / BENCH_ITERATIONS);

    printf("\nIn-Place Merge Operation (%lu iterations):\n", BENCH_ITERATIONS);
    printf("  Total Time: %.3f ms (%.3f ns per operation)\n",
            result->merge_inplace_time / 1000.0,
            (double)result->merge_inplace_time * 1000 / BENCH_ITERATIONS);
       
    printf("\nSerialization (%lu iterations):\n", BENCH_ITERATIONS);
    printf("  Total Time: %.3f ms (%.3f ns per operation)\n",
           result->serialize_time / 1000.0,
           (double)result->serialize_time * 1000 / BENCH_ITERATIONS);
    printf("  Serialized Size: %zu bytes\n", result->serialized_size);

    printf("\nDeserialization (%lu iterations):\n", BENCH_ITERATIONS);
    printf("  Total Time: %.3f ms (%.3f ns per operation)\n",
           result->deserialize_time / 1000.0,
           (double)result->deserialize_time * 1000 / BENCH_ITERATIONS);

    printf("\nSerialized Percentile Calculation (%lu iterations):\n", 7 * BENCH_ITERATIONS);
    printf("  Total Time: %.3f ms (%.3f ns per operation)\n",
           result->serialized_percentile_time / 1000.0,
           (result->serialized_percentile_time * 1000.0) / (7 * BENCH_ITERATIONS));
    printf("  Values: p50=%.2f, p75=%.2f, p90=%.2f, p95=%.2f, p99=%.2f, p99.9=%.2f, p99.99=%.2f\n",
           result->s_p50, result->s_p75, result->s_p90, result->s_p95, result->s_p99, result->s_p999, result->s_p9999);
    /*
    printf("\nMultiple Serialized Percentiles Calculation (%lu iterations):\n", 7 * BENCH_ITERATIONS);
    printf("  Total Time: %.3f ms (%.3f ns per operation)\n",
           result->serialized_percentiles_time / 1000.0,
           (result->serialized_percentiles_time * 1000.0) / (7 * BENCH_ITERATIONS));

           printf("  Values: p50=%.2f, p75=%.2f, p90=%.2f, p95=%.2f, p99=%.2f, p99.9=%.2f, p99.99=%.2f\n",
            result->sm_p50, result->sm_p75, result->sm_p90, result->sm_p95, result->sm_p99, result->sm_p999, result->sm_p9999);
    */
    printf("\nMerge with Serialized Data (%lu iterations):\n", BENCH_ITERATIONS);
    printf("  Total Time: %.3f ms (%.3f ns per operation)\n",
           result->merge_serialized_time / 1000.0,
           (double)result->merge_serialized_time * 1000 / BENCH_ITERATIONS);

    printf("\nIn-Place Merge with Serialized Data (%lu iterations):\n", BENCH_ITERATIONS);
    printf("  Total Time: %.3f ms (%.3f ns per operation)\n",
           result->merge_inplace_serialized_time / 1000.0,
           (double)result->merge_inplace_serialized_time * 1000 / BENCH_ITERATIONS);

    printf("\nMerge of Two Serialized Heistograms (%lu iterations):\n", BENCH_ITERATIONS);
    printf("  Total Time: %.3f ms (%.3f ns per operation)\n",
           result->merge_two_serialized_time / 1000.0,
           (double)result->merge_two_serialized_time * 1000 / BENCH_ITERATIONS);

    printf("\nPrank Calculation (%lu iterations):\n", BENCH_ITERATIONS);
    printf("  Total Time: %.3f ms (%.3f ns per operation)\n",
           result->prank_time / 1000.0,
           (double)result->prank_time * 1000 / BENCH_ITERATIONS);

    printf("----------------------------------------\n");
}

int main() {
    srand(time(NULL));
    
    // Define test sizes
    size_t sizes[] = {10000000};
    //size_t sizes[] = {10000, 100000, 1000000, 10000000};
    float margins[] = {0.01f};
    
    // Allocate maximum needed buffer
    uint64_t* data = malloc(sizeof(uint64_t) * sizes[0]);
    if (!data) {
        printf("Failed to allocate memory\n");
        return 1;
    }
    
    // Run benchmarks for each size and error margin
    for (size_t i = 0; i < sizeof(sizes)/sizeof(sizes[0]); i++) {
        // Generate data
        generate_random_array(data, sizes[i]);
        
        // Run benchmarks for each error margin
        for (size_t j = 0; j < sizeof(margins)/sizeof(margins[0]); j++) {
            BenchmarkResult result = run_benchmark(data, sizes[i], margins[j]);
            print_results(&result);
        }
    }
    
    free(data);
    return 0;
}
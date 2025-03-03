#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

// Include the Heistogram library
#include "../src/heistogram.h"

// Test helper function to verify that two double values are approximately equal
static int double_equals(double a, double b, double epsilon) {
    return fabs(a - b) < epsilon;
}

// Helper function to print histogram statistics
static void print_heistogram_stats(const Heistogram* h, const char* label) {
    printf("\n--- %s ---\n", label);
    printf("Count: %lu\n", heistogram_count(h));
    printf("Min: %lu\n", heistogram_min(h));
    printf("Max: %lu\n", heistogram_max(h));
    printf("Memory Size: %u bytes\n", heistogram_memory_size(h));
    printf("Percentiles:\n");
    printf("  P50: %.2f\n", heistogram_percentile(h, 50.0));
    printf("  P90: %.2f\n", heistogram_percentile(h, 90.0));
    printf("  P95: %.2f\n", heistogram_percentile(h, 95.0));
    printf("  P99: %.2f\n", heistogram_percentile(h, 99.0));
}

// Helper function to compare histograms for approximate equality
static int histograms_equal(const Heistogram* h1, const Heistogram* h2, double epsilon) {
    if (heistogram_count(h1) != heistogram_count(h2)) return 0;
    if (heistogram_min(h1) != heistogram_min(h2)) return 0;
    if (heistogram_max(h1) != heistogram_max(h2)) return 0;
    
    // Compare percentiles
    double p50_1 = heistogram_percentile(h1, 50.0);
    double p50_2 = heistogram_percentile(h2, 50.0);
    if (!double_equals(p50_1, p50_2, epsilon)) {
        printf("P50 mismatch: %.2f vs %.2f\n", p50_1, p50_2);
        return 0;
    }
    
    double p90_1 = heistogram_percentile(h1, 90.0);
    double p90_2 = heistogram_percentile(h2, 90.0);
    if (!double_equals(p90_1, p90_2, epsilon)) {
        printf("P90 mismatch: %.2f vs %.2f\n", p90_1, p90_2);
        return 0;
    }
    
    double p99_1 = heistogram_percentile(h1, 99.0);
    double p99_2 = heistogram_percentile(h2, 99.0);
    if (!double_equals(p99_1, p99_2, epsilon)) {
        printf("P99 mismatch: %.2f vs %.2f\n", p99_1, p99_2);
        return 0;
    }
    
    return 1;
}

// Test basic histogram functionality
static void test_basic_functionality() {
    printf("\n=== Testing Basic Functionality ===\n");
    
    Heistogram* h = heistogram_create();
    assert(h != NULL);
    
    // Test initial state
    assert(heistogram_count(h) == 0);
    assert(heistogram_min(h) == 0);
    assert(heistogram_max(h) == 0);
    
    // Add some values
    heistogram_add(h, 100);
    heistogram_add(h, 200);
    heistogram_add(h, 300);
    heistogram_add(h, 400);
    heistogram_add(h, 500);
    
    // Test count and min/max
    assert(heistogram_count(h) == 5);
    assert(heistogram_min(h) == 100);
    assert(heistogram_max(h) == 500);
    
    // Add extreme values
    heistogram_add(h, 5);
    heistogram_add(h, 5000);
    
    assert(heistogram_count(h) == 7);
    assert(heistogram_min(h) == 5);
    assert(heistogram_max(h) == 5000);
    
    // Test percentiles
    double p50 = heistogram_percentile(h, 50.0);
    double p90 = heistogram_percentile(h, 90.0);
    
    printf("P50: %.2f\n", p50);
    printf("P90: %.2f\n", p90);
        
    // Test prank
    double rank = heistogram_prank(h, 30);
    printf("Rank of 30: %.2f%%\n", rank);
    
    heistogram_free(h);
    printf("Basic functionality test passed!\n");
}

// Test serialization and deserialization
static void test_serialization() {
    printf("\n=== Testing Serialization/Deserialization ===\n");
    
    Heistogram* h1 = heistogram_create();
    assert(h1 != NULL);
    // Add values with a wide range
    for (int i = 1; i <= 1000; i++) {
        heistogram_add(h1, i);
    }
    printf("123\n");
    // Add some higher values
    heistogram_add(h1, 500);
    heistogram_add(h1, 1000);
    heistogram_add(h1, 2000);
    
    print_heistogram_stats(h1, "Original Heistogram");
    
    // Serialize
    size_t size;
    void* serialized = heistogram_serialize(h1, &size);
    assert(serialized != NULL);
    printf("Serialized size: %zu bytes\n", size);
    
    // Test percentile on serialized data directly
    double p50_serialized = heistogram_percentile_serialized(serialized, size, 50.0);
    double p90_serialized = heistogram_percentile_serialized(serialized, size, 90.0);
    double p99_serialized = heistogram_percentile_serialized(serialized, size, 99.0);
    
    printf("Serialized P50: %.2f\n", p50_serialized);
    printf("Serialized P90: %.2f\n", p90_serialized);
    printf("Serialized P99: %.2f\n", p99_serialized);
    
    
    // Deserialize
    Heistogram* h2 = heistogram_deserialize(serialized, size);
    assert(h2 != NULL);
    
    print_heistogram_stats(h2, "Deserialized Heistogram");
    
    // Compare original and deserialized
    assert(histograms_equal(h1, h2, 0.1));
    
    // Compare direct percentile calculations on serialized data
    assert(double_equals(heistogram_percentile(h1, 50.0), p50_serialized, 0.1));
    assert(double_equals(heistogram_percentile(h1, 90.0), p90_serialized, 0.1));
    assert(double_equals(heistogram_percentile(h1, 99.0), p99_serialized, 0.1));
    
    
    free(serialized);
    heistogram_free(h1);
    heistogram_free(h2);
    
    printf("Serialization test passed!\n");
}

// Test merging two histograms
static void test_merge() {
    printf("\n=== Testing Heistogram Merging ===\n");
    
    // Create two histograms with different data ranges
    Heistogram* h1 = heistogram_create();
    Heistogram* h2 = heistogram_create();
    
    // Add values to first histogram (lower range)
    for (int i = 1; i <= 50; i++) {
        heistogram_add(h1, i);
    }
    
    // Add values to second histogram (higher range)
    for (int i = 51; i <= 100; i++) {
        heistogram_add(h2, i);
    }
    
    print_heistogram_stats(h1, "Heistogram 1");
    print_heistogram_stats(h2, "Heistogram 2");
    
    // Merge h1 and h2 into a new histogram
    Heistogram* h_merged = heistogram_merge(h1, h2);
    assert(h_merged != NULL);
    
    print_heistogram_stats(h_merged, "Merged Heistogram");
    
    // Verify merged histogram properties
    assert(heistogram_count(h_merged) == heistogram_count(h1) + heistogram_count(h2));
    assert(heistogram_min(h_merged) == heistogram_min(h1));
    assert(heistogram_max(h_merged) == heistogram_max(h2));
    
    // Clone h1 for in-place merge test
    size_t h1_size;
    void* h1_serialized = heistogram_serialize(h1, &h1_size);
    Heistogram* h1_clone = heistogram_deserialize(h1_serialized, h1_size);
    free(h1_serialized);
    
    // Test in-place merge
    int merge_result = heistogram_merge_inplace(h1_clone, h2);
    assert(merge_result == 1);
    
    print_heistogram_stats(h1_clone, "In-place Merged Heistogram");
    
    // Verify in-place merged histogram matches the regular merged one
    assert(histograms_equal(h_merged, h1_clone, 0.1));
    
    heistogram_free(h1);
    heistogram_free(h2);
    heistogram_free(h_merged);
    heistogram_free(h1_clone);
    
    printf("Merge test passed!\n");
}

// Test merging with serialized histograms
static void test_serialized_merge() {
    printf("\n=== Testing Serialized Heistogram Merging ===\n");
    
    // Create two histograms with different data ranges
    Heistogram* h1 = heistogram_create();
    Heistogram* h2 = heistogram_create();
    
    // Add values to first histogram (lower range)
    for (int i = 1; i <= 500; i++) {
        heistogram_add(h1, i);
    }
    
    // Add values to second histogram (higher range)
    for (int i = 501; i <= 1000; i++) {
        heistogram_add(h2, i);
    }
    
    // Serialize both histograms
    size_t h1_size, h2_size;
    void* h1_serialized = heistogram_serialize(h1, &h1_size);
    void* h2_serialized = heistogram_serialize(h2, &h2_size);
    
    assert(h1_serialized != NULL);
    assert(h2_serialized != NULL);
    
    // Test merging in-memory with serialized
    Heistogram* h_merged1 = heistogram_merge_serialized(h1, h2_serialized, h2_size);
    assert(h_merged1 != NULL);
    
    print_heistogram_stats(h_merged1, "Memory+Serialized Merged Heistogram");
    
    // Test merging two serialized histograms
    Heistogram* h_merged2 = heistogram_merge_two_serialized(h1_serialized, h1_size, h2_serialized, h2_size);
    assert(h_merged2 != NULL);
    
    print_heistogram_stats(h_merged2, "Two Serialized Merged Heistogram");
    
    // Verify both merged histograms are equivalent
    assert(histograms_equal(h_merged1, h_merged2, 0.1));
    
    // Test in-place merge with serialized
    Heistogram* h1_clone = heistogram_deserialize(h1_serialized, h1_size);
    int merge_result = heistogram_merge_inplace_serialized(h1_clone, h2_serialized, h2_size);
    assert(merge_result == 1);
    
    print_heistogram_stats(h1_clone, "In-place Serialized Merged Heistogram");
    
    // Verify all merged histograms are equivalent
    assert(histograms_equal(h_merged1, h1_clone, 0.1));
    
    free(h1_serialized);
    free(h2_serialized);
    heistogram_free(h1);
    heistogram_free(h2);
    heistogram_free(h_merged1);
    heistogram_free(h_merged2);
    heistogram_free(h1_clone);
    
    printf("Serialized merge test passed!\n");
}

// Test with random data and multiple operations
static void test_random_data() {
    printf("\n=== Testing with Random Data ===\n");
    
    srand(time(NULL));
    
    // Create a histogram and add random values
    Heistogram* h1 = heistogram_create();
    assert(h1 != NULL);
    
    const int count = 10000;
    const int max_value = 10000;
    printf("Adding %d random values...\n", count);
    
    for (int i = 0; i < count; i++) {
        double value = rand() % max_value;
        heistogram_add(h1, value);
    }
    
    print_heistogram_stats(h1, "Random Data Heistogram");
    
    // Serialize and deserialize
    size_t size;
    void* serialized = heistogram_serialize(h1, &size);
    assert(serialized != NULL);
    
    Heistogram* h2 = heistogram_deserialize(serialized, size);
    assert(h2 != NULL);
    
    print_heistogram_stats(h2, "Deserialized Random Heistogram");
    
    // Verify serialization/deserialization preserved the data
    assert(histograms_equal(h1, h2, 0.5)); // Using larger epsilon for random data
    
    
    // Cleanup
    free(serialized);
    heistogram_free(h1);
    heistogram_free(h2);
    
    printf("Random data test passed!\n");
}

// Test extreme values and edge cases
static void test_edge_cases() {
    printf("\n=== Testing Edge Cases ===\n");
    
    Heistogram* h = heistogram_create();
    assert(h != NULL);
    
    // Test with very small values
    heistogram_add(h, 0.1);
    heistogram_add(h, 0.01);
    heistogram_add(h, 0.001);
    
    // Test with very large values
    heistogram_add(h, 1000000);
    heistogram_add(h, 10000000);
    
    // Test with zero
    heistogram_add(h, 0);
    
    print_heistogram_stats(h, "Edge Case Heistogram");
    
    // Serialize and test
    size_t size;
    void* serialized = heistogram_serialize(h, &size);
    assert(serialized != NULL);
    
    // Deserialize and verify
    Heistogram* h2 = heistogram_deserialize(serialized, size);
    assert(h2 != NULL);
    
    print_heistogram_stats(h2, "Deserialized Edge Case Heistogram");
    
    // Clean up
    free(serialized);
    heistogram_free(h);
    heistogram_free(h2);
    
    printf("Edge case test passed!\n");
}

// Test with concentration of values
static void test_value_concentration() {
    printf("\n=== Testing Value Concentration ===\n");
    
    Heistogram* h = heistogram_create();
    assert(h != NULL);
    
    // Add many values concentrated in a narrow range
    for (int i = 0; i < 1000; i++) {
        heistogram_add(h, 100);
    }
    
    for (int i = 0; i < 100; i++) {
        heistogram_add(h, 101);
    }
    
    for (int i = 0; i < 10; i++) {
        heistogram_add(h, 102);
    }
    
    // Add some outliers
    heistogram_add(h, 1);
    heistogram_add(h, 1000);
    
    print_heistogram_stats(h, "Concentrated Value Heistogram");
    
    // Test percentile calculations
    double p50 = heistogram_percentile(h, 50.0);
    double p90 = heistogram_percentile(h, 90.0);
    double p99 = heistogram_percentile(h, 99.0);
    
    printf("Expected: P50 = 100, P90 = 100, P99 ≈ 101\n");
    printf("Actual: P50 = %.2f, P90 = %.2f, P99 = %.2f\n", p50, p90, p99);
    
    // Serialize, deserialize, and verify
    size_t size;
    void* serialized = heistogram_serialize(h, &size);
    assert(serialized != NULL);
    
    Heistogram* h2 = heistogram_deserialize(serialized, size);
    assert(h2 != NULL);
    
    // Verify percentiles are preserved
    assert(double_equals(p50, heistogram_percentile(h2, 50.0), 0.1));
    assert(double_equals(p90, heistogram_percentile(h2, 90.0), 0.1));
    assert(double_equals(p99, heistogram_percentile(h2, 99.0), 0.1));
    
    // Clean up
    free(serialized);
    heistogram_free(h);
    heistogram_free(h2);
    
    printf("Value concentration test passed!\n");
}

// Comprehensive multi-step test
static void test_multi_step_workflow() {
    printf("\n=== Testing Multi-step Workflow ===\n");
    
    // Create multiple histograms for different "time periods"
    Heistogram* h_period1 = heistogram_create();
    Heistogram* h_period2 = heistogram_create();
    Heistogram* h_period3 = heistogram_create();
    
    // Add values with different distributions
    printf("Adding period 1 data (low values)...\n");
    for (int i = 0; i < 1000; i++) {
        heistogram_add(h_period1, rand() % 100);
    }
    
    printf("Adding period 2 data (medium values)...\n");
    for (int i = 0; i < 1000; i++) {
        heistogram_add(h_period2, 100 + rand() % 100);
    }
    
    printf("Adding period 3 data (high values)...\n");
    for (int i = 0; i < 1000; i++) {
        heistogram_add(h_period3, 200 + rand() % 100);
    }
    
    // First, serialize period 1
    size_t size1;
    void* serialized1 = heistogram_serialize(h_period1, &size1);
    assert(serialized1 != NULL);
    
    // Then, merge period 2 into period 1 and serialize the result
    Heistogram* merged_1_2 = heistogram_merge(h_period1, h_period2);
    assert(merged_1_2 != NULL);
    
    size_t size1_2;
    void* serialized1_2 = heistogram_serialize(merged_1_2, &size1_2);
    assert(serialized1_2 != NULL);
    
    // Finally, merge period 3 with the serialized (period 1 + period 2)
    Heistogram* final_merged = heistogram_merge_serialized(h_period3, serialized1_2, size1_2);
    assert(final_merged != NULL);
    
    // Also test merging all three periods directly
    Heistogram* direct_merged = heistogram_merge(h_period1, h_period2);
    assert(direct_merged != NULL);
    assert(heistogram_merge_inplace(direct_merged, h_period3) == 1);
    
    // Compare results of the two merge approaches
    print_heistogram_stats(final_merged, "Step-by-step Merged Heistogram");
    print_heistogram_stats(direct_merged, "Directly Merged Heistogram");
    
    // Verify results match
    assert(histograms_equal(final_merged, direct_merged, 0.5));
    
    // Clean up
    free(serialized1);
    free(serialized1_2);
    heistogram_free(h_period1);
    heistogram_free(h_period2);
    heistogram_free(h_period3);
    heistogram_free(merged_1_2);
    heistogram_free(final_merged);
    heistogram_free(direct_merged);
    
    printf("Multi-step workflow test passed!\n");
}

// Extended test for small and extreme percentiles
static void test_extreme_percentiles() {
    printf("\n=== Testing Extreme Percentiles ===\n");
    
    Heistogram* h = heistogram_create();
    assert(h != NULL);
    
    // Add a wide range of values with different distributions
    // Add some very low outliers
    heistogram_add(h, 1);
    heistogram_add(h, 2);
    heistogram_add(h, 3);
    
    // Add bulk of values in middle range
    for (int i = 0; i < 10000; i++) {
        double value = 100 + (rand() % 900);
        heistogram_add(h, value);
    }
    
    // Add some very high outliers
    heistogram_add(h, 9995);
    heistogram_add(h, 9998);
    heistogram_add(h, 10000);
    
    print_heistogram_stats(h, "Extreme Percentile Heistogram");
    
    // Test extreme low percentiles
    double p0 = heistogram_percentile(h, 0.0);
    double p0_1 = heistogram_percentile(h, 0.1);
    double p1 = heistogram_percentile(h, 1.0);
    
    // Test extreme high percentiles
    double p99_9 = heistogram_percentile(h, 99.9);
    double p99_99 = heistogram_percentile(h, 99.99);
    double p99_999 = heistogram_percentile(h, 99.999);
    double p100 = heistogram_percentile(h, 100.0);
    
    printf("Extreme Low Percentiles:\n");
    printf("  P0: %.2f\n", p0);
    printf("  P0.1: %.2f\n", p0_1);
    printf("  P1: %.2f\n", p1);
    
    printf("Extreme High Percentiles:\n");
    printf("  P99.9: %.2f\n", p99_9);
    printf("  P99.99: %.2f\n", p99_99);
    printf("  P99.999: %.2f\n", p99_999);
    printf("  P100: %.2f\n", p100);
        
    // Verify min/max against p0/p100
    assert(double_equals(heistogram_min(h), p0, 0.01));
    assert(double_equals(heistogram_max(h), p100, 0.01));
    
    // Serialize and test extreme percentiles on serialized data
    size_t size;
    void* serialized = heistogram_serialize(h, &size);
    assert(serialized != NULL);
    
    // Test extreme percentiles on serialized data
    double p0_s = heistogram_percentile_serialized(serialized, size, 0.0);
    double p0_1_s = heistogram_percentile_serialized(serialized, size, 0.1);
    double p99_99_s = heistogram_percentile_serialized(serialized, size, 99.99);
    double p100_s = heistogram_percentile_serialized(serialized, size, 100.0);
    
    printf("Serialized Extreme Percentiles:\n");
    printf("  P0: %.2f\n", p0_s);
    printf("  P0.1: %.2f\n", p0_1_s);
    printf("  P99.99: %.2f\n", p99_99_s);
    printf("  P100: %.2f\n", p100_s);
    
    // Verify serialized results match in-memory results
    assert(double_equals(p0, p0_s, 0.01));
    assert(double_equals(p0_1, p0_1_s, 0.01));
    assert(double_equals(p99_99, p99_99_s, 0.01));
    assert(double_equals(p100, p100_s, 0.01));
    
    
    // Clean up
    free(serialized);
    heistogram_free(h);
    
    printf("Extreme percentile test passed!\n");
}

// Test extreme percentiles with skewed distributions
static void test_skewed_distributions() {
    printf("\n=== Testing Extreme Percentiles with Skewed Distributions ===\n");
    
    // Create a right-skewed distribution (many low values, few high values)
    Heistogram* h_right_skewed = heistogram_create();
    assert(h_right_skewed != NULL);
    
    printf("Creating right-skewed distribution...\n");
    for (int i = 0; i < 9900; i++) {
        double value = 10 + (rand() % 90);  // Most values between 10-100
        heistogram_add(h_right_skewed, value);
    }
    
    for (int i = 0; i < 100; i++) {
        double value = 1000 + (rand() % 9000);  // Few values between 1000-10000
        heistogram_add(h_right_skewed, value);
    }
    
    // Create a left-skewed distribution (few low values, many high values)
    Heistogram* h_left_skewed = heistogram_create();
    assert(h_left_skewed != NULL);
    
    printf("Creating left-skewed distribution...\n");
    for (int i = 0; i < 100; i++) {
        double value = 10 + (rand() % 90);  // Few values between 10-100
        heistogram_add(h_left_skewed, value);
    }
    
    for (int i = 0; i < 9900; i++) {
        double value = 1000 + (rand() % 9000);  // Most values between 1000-10000
        heistogram_add(h_left_skewed, value);
    }
    
    // Test extreme percentiles for both distributions
    double extreme_percentiles[] = {0.0, 0.1, 1.0, 50.0, 99.0, 99.9, 99.99, 100.0};
    
    printf("Right-skewed distribution (many low values, few high values):\n");
    print_heistogram_stats(h_right_skewed, "Right-skewed Heistogram");
    for (int i = 0; i < 8; i++) {
        printf("  P%.3f: %.2f\n", extreme_percentiles[i], heistogram_percentile(h_right_skewed, extreme_percentiles[i]));
    }
    
    printf("\nLeft-skewed distribution (few low values, many high values):\n");
    print_heistogram_stats(h_left_skewed, "Left-skewed Heistogram");
    for (int i = 0; i < 8; i++) {
        printf("  P%.3f: %.2f\n", extreme_percentiles[i], heistogram_percentile(h_left_skewed, extreme_percentiles[i]));
    }
    
    // Clean up
    heistogram_free(h_right_skewed);
    heistogram_free(h_left_skewed);
    
    printf("Skewed distribution test passed!\n");
}

// Main test function
int main() {
    printf("Starting Heistogram tests...\n");
    
    test_basic_functionality();
    test_serialization();
    test_merge();
    test_serialized_merge();
    test_random_data();
    test_edge_cases();
    test_value_concentration();
    test_multi_step_workflow();
    test_skewed_distributions();
    test_extreme_percentiles();
    
    printf("\n=== All tests passed! ===\n");
    return 0;
}
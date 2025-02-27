## Heistogram C API Summary

This document provides a concise overview of the Heistogram C API, based on the `heistogram.h` header file.

### 1. Data Structures

*   **`Heistogram`**:
    *   The core structure representing a Heistogram. It's an opaque struct, meaning you primarily interact with it through the provided functions.
    *   Internally, it manages:
        *   `capacity`:  Allocated size for buckets.
        *   `min_bucket_id`: Optimization for serialization.
        *   `total_count`: Total data points added.
        *   `min`, `max`: Minimum and maximum values seen.
        *   `buckets`: An array of `Bucket` structs.

*   **`Bucket`**:
    *   A simple structure representing a histogram bucket.
    *   Contains a single member:
        *   `count`:  Number of data points falling into this bucket.

### 2. Core API Functions

This section summarizes the main functions for interacting with Heistogram.

#### 2.1 Creation and Destruction

*   **`Heistogram* heistogram_create(void)`**:
    *   **Description:** Creates a new, empty Heistogram object.
    *   **Parameters:** None.
    *   **Returns:** A pointer to the newly created `Heistogram` on success, `NULL` on failure (memory allocation error).  *Remember to check for `NULL` return!*

*   **`void heistogram_free(Heistogram* h)`**:
    *   **Description:** Frees the memory allocated for a `Heistogram` object.
    *   **Parameters:**
        *   `h`:  A pointer to the `Heistogram` object to be freed.
    *   **Returns:** `void`. *It is crucial to call this function to avoid memory leaks when you are finished using a Heistogram.*

#### 2.2 Information Retrieval

*   **`uint64_t heistogram_count(const Heistogram* h)`**:
    *   **Description:** Returns the total number of data points inserted into the histogram.
    *   **Parameters:**
        *   `h`: A pointer to a `Heistogram` object (constant, will not be modified).
    *   **Returns:** The total count of values, or `0` if `h` is `NULL`.

*   **`uint64_t heistogram_max(const Heistogram* h)`**:
    *   **Description:** Returns the maximum value inserted into the histogram.
    *   **Parameters:**
        *   `h`: A pointer to a `Heistogram` object (constant).
    *   **Returns:** The maximum value, or `0` if `h` is `NULL` or no data has been inserted.

*   **`uint64_t heistogram_min(const Heistogram* h)`**:
    *   **Description:** Returns the minimum value inserted into the histogram.
    *   **Parameters:**
        *   `h`: A pointer to a `Heistogram` object (constant).
    *   **Returns:** The minimum value, or `0` if `h` is `NULL` or no data has been inserted.

*   **`uint32_t heistogram_memory_size(const Heistogram* h)`**:
    *   **Description:** Returns the approximate memory size (in bytes) used by the Heistogram object and its internal bucket storage.
    *   **Parameters:**
        *   `h`: A pointer to a `Heistogram` object (constant).
    *   **Returns:** The memory size in bytes, or `0` if `h` is `NULL`.

#### 2.3 Data Insertion

*   **`void heistogram_add(Heistogram* h, uint64_t value)`**:
    *   **Description:** Adds a single integer value to the Heistogram.
    *   **Parameters:**
        *   `h`: A pointer to the `Heistogram` object.
        *   `value`: The `uint64_t` value to be inserted. Values should be non-negative.
    *   **Returns:** `void`.

#### 2.4 Histogram Merging

*   **`Heistogram* heistogram_merge(const Heistogram* h1, const Heistogram* h2)`**:
    *   **Description:** Merges two in-memory Heistograms (`h1` and `h2`) into a *new* Heistogram object.  This is a non-destructive merge; `h1` and `h2` are unchanged.
    *   **Parameters:**
        *   `h1`: Pointer to the first `Heistogram` (constant).
        *   `h2`: Pointer to the second `Heistogram` (constant).
    *   **Returns:** A pointer to a new `Heistogram` object representing the merged result, or `NULL` on error (e.g., memory allocation failure or if either input is `NULL`). *Remember to free the returned histogram when done!*

*   **`int heistogram_merge_in_place(Heistogram* h1, const Heistogram* h2)`**:
    *   **Description:** Merges the contents of `h2` into `h1`, modifying `h1` directly. This is an in-place merge. `h2` remains unchanged.
    *   **Parameters:**
        *   `h1`: Pointer to the *destination* `Heistogram` (will be modified).
        *   `h2`: Pointer to the `Heistogram` to merge *from* (constant).
    *   **Returns:** `1` on success, `0` on failure (e.g., if either input is `NULL` or memory reallocation fails if `h1` needs to grow).

*   **`Heistogram* heistogram_merge_serialized(const Heistogram* h, const void* buffer, size_t size)`**:
    *   **Description:** Merges a serialized Heistogram (from `buffer` and `size`) into an in-memory Heistogram `h`. Creates a *new* Heistogram object as the result.
    *   **Parameters:**
        *   `h`: Pointer to the in-memory `Heistogram`.
        *   `buffer`: Pointer to the serialized Heistogram data.
        *   `size`: Size of the serialized Heistogram data in bytes.
    *   **Returns:** A pointer to a new `Heistogram` object representing the merged result, or `NULL` on error (e.g., if inputs are invalid, deserialization fails, or memory allocation fails). *Remember to free the returned histogram when done!*

*   **`Heistogram* heistogram_merge_two_serialized(const void* buffer1, size_t size1, const void* buffer2, size_t size2)`**:
    *   **Description:** Merges two *serialized* Heistograms (from `buffer1`/`size1` and `buffer2`/`size2`) into a *new* Heistogram object.
    *   **Parameters:**
        *   `buffer1`, `size1`:  Serialized data and size for the first Heistogram.
        *   `buffer2`, `size2`:  Serialized data and size for the second Heistogram.
    *   **Returns:** A pointer to a new `Heistogram` object representing the merged result, or `NULL` on error (e.g., if inputs are invalid, deserialization fails, or memory allocation fails). *Remember to free the returned histogram when done!*

*   **`int heistogram_merge_inplace_serialized(Heistogram* h, const void* buffer, size_t size)`**:
    *   **Description:** Merges a serialized Heistogram (from `buffer` and `size`) *in-place* into an existing in-memory Heistogram `h`, modifying `h` directly.
    *   **Parameters:**
        *   `h`: Pointer to the destination in-memory `Heistogram` (will be modified).
        *   `buffer`: Pointer to the serialized Heistogram data.
        *   `size`: Size of the serialized Heistogram data in bytes.
    *   **Returns:** `1` on success, `0` on failure (e.g., if inputs are invalid, deserialization fails, or memory reallocation fails if `h` needs to grow).

#### 2.5 Percentile and Rank Queries

*   **`double heistogram_percentile(const Heistogram* h, double p)`**:
    *   **Description:** Calculates the approximate p-th percentile from the Heistogram data.
    *   **Parameters:**
        *   `h`: A pointer to a `Heistogram` object (constant).
        *   `p`: The percentile value to calculate (a double between 0.0 and 100.0, inclusive).
    *   **Returns:** The approximate p-th percentile value as a `double`. Returns `0` if `h` is `NULL` or `p` is out of range.

*   **`void heistogram_percentiles(const Heistogram* h, const double* percentiles, size_t num_percentiles, double* results)`**:
    *   **Description:** Calculates multiple percentiles in a single pass for efficiency.
    *   **Parameters:**
        *   `h`: A pointer to a `Heistogram` object (constant).
        *   `percentiles`: An array of `double` percentile values (between 0.0 and 100.0).
        *   `num_percentiles`: The number of percentiles in the `percentiles` array.
        *   `results`: A pre-allocated array of `double` of size `num_percentiles` where the results will be stored.
    *   **Returns:** `void`. The calculated percentiles are placed in the `results` array, in the same order as the input `percentiles` array.

*   **`double heistogram_prank(const Heistogram* h, double value)`**:
    *   **Description:**  Calculates the percentile rank (or p-rank) of a given value within the Heistogram's distribution. This is the approximate percentile below which the given `value` falls.
    *   **Parameters:**
        *   `h`: A pointer to a `Heistogram` object (constant).
        *   `value`: The value for which to calculate the percentile rank (a `double`).
    *   **Returns:** The percentile rank of the value as a `double` (between 0.0 and 100.0). Returns `0` if `h` is `NULL` or `value` is negative, returns `100` if value is greater than or equal to max value in histogram.

#### 2.6 Serialization and Deserialization

*   **`void* heistogram_serialize(const Heistogram* h, size_t* size)`**:
    *   **Description:** Serializes a Heistogram object into a byte buffer for storage or transmission.
    *   **Parameters:**
        *   `h`: A pointer to the `Heistogram` object to serialize (constant).
        *   `size`: A pointer to a `size_t` variable where the size (in bytes) of the serialized data will be written.
    *   **Returns:** A pointer to a dynamically allocated byte buffer containing the serialized Heistogram data, or `NULL` on error (e.g., if `h` or `size` is `NULL` or memory allocation fails). *You are responsible for freeing this buffer using `free()` when you are finished with it.*

*   **`Heistogram* heistogram_deserialize(const void* buffer, size_t size)`**:
    *   **Description:** Deserializes a Heistogram object from a byte buffer.
    *   **Parameters:**
        *   `buffer`: A pointer to the byte buffer containing the serialized Heistogram data.
        *   `size`: The size of the serialized data in bytes.
    *   **Returns:** A pointer to a newly created `Heistogram` object deserialized from the buffer, or `NULL` on error (e.g., if `buffer` is `NULL`, deserialization fails, or memory allocation fails). *Remember to free the returned histogram when done!*

#### 2.7 Serialized Data Queries

*   **`double heistogram_percentile_serialized(const void* buffer, size_t size, double p)`**:
    *   **Description:** Calculates the approximate p-th percentile directly from serialized Heistogram data, without deserializing the entire histogram. This is a key performance feature.
    *   **Parameters:**
        *   `buffer`: A pointer to the byte buffer containing the serialized Heistogram data.
        *   `size`: The size of the serialized data in bytes.
        *   `p`: The percentile value to calculate (a double between 0.0 and 100.0).
    *   **Returns:** The approximate p-th percentile value as a `double`. Returns `0` if inputs are invalid or deserialization fails.

*   **`void heistogram_percentiles_serialized(const void* buffer, size_t size, const double* percentiles, size_t num_percentiles, double* results)`**:
    *   **Description:** Calculates multiple percentiles directly from serialized Heistogram data in a single pass, without full deserialization. Optimized for bulk percentile queries on serialized data.
    *   **Parameters:**
        *   `buffer`: A pointer to the byte buffer containing the serialized Heistogram data.
        *   `size`: The size of the serialized data in bytes.
        *   `percentiles`: An array of `double` percentile values.
        *   `num_percentiles`: The number of percentiles to calculate.
        *   `results`: A pre-allocated `double` array to store the percentile results.
    *   **Returns:** `void`. Results are written into the `results` array.

### 3. Important Notes

*   **Error Handling:**  Many functions return `NULL` or `0` on failure. *Always check return values, especially from `heistogram_create`, `heistogram_deserialize`, `heistogram_merge`, `heistogram_merge_serialized`, and `heistogram_serialize` to handle potential errors (like memory allocation failures).*
*   **Memory Management:**  You are responsible for freeing memory allocated by `heistogram_create`, `heistogram_deserialize`, `heistogram_merge`, `heistogram_merge_serialized`, and `heistogram_serialize` using `heistogram_free()` and `free()` respectively.
*   **Integer Data:** Heistogram is optimized for integer data (`uint64_t`). If you have floating-point data, consider scaling and rounding to integers before inserting.
*   **Growth Factor (`HEIST_GROWTH_FACTOR`):**  The header defines a `HEIST_GROWTH_FACTOR` (currently `0.02`). This parameter controls the accuracy and size of the histogram. *While it's defined as a static global, it's likely intended to be configurable at compile time if you need to adjust the error bound.  Consult documentation for recommended values and implications of changing it.*
*   **Varint Encoding:** Heistogram uses varint encoding for serialization, which is efficient for compressing integer counts and metadata. This is handled internally.

This summary should provide a good starting point for understanding and using the Heistogram C API. For more detailed information, please refer to the complete documentation and examples (when available).

#ifndef HEISTOGRAM_H
#define HEISTOGRAM_H

#include <math.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static float HEIST_GROWTH_FACTOR = 0.02;
static float HEIST_INV_LOG_GROWTH_FACTOR = 35.00278878;//116.2767475;
static uint16_t HEIST_MAX_UNMAPPED_BUCKET = 57; // after this point multiple values can fall in the same bucket
static uint16_t HEIST_BUCKET_MAPPING_DELTA = 147; // 205 - 57 - 1; //shift the mappings to continue from the last artificial one

// Simplified Bucket structure - only stores count
typedef struct {
    uint64_t count;
} Bucket;

// Updated Heistogram structure
typedef struct {
    uint16_t capacity;       // Current capacity of buckets array
    uint16_t min_bucket_id;  // Smalles bucket id, for optimizing serialization
    uint64_t total_count;    // Total count of all values
    uint64_t min;            // Minimum value
    uint64_t max;            // Maximum value
    Bucket* buckets;         // Array of buckets, index = bucket ID
} Heistogram;


/*************************/
/* VARINT HELPER METHODS */
/*************************/

// Returns number of bytes written
static inline size_t encode_varint(uint64_t val, uint8_t* buf) {
    if (val <= 240) {
        buf[0] = (uint8_t)val;
        return 1;
    }
    if (val <= 2287) {
        val -= 240;
        buf[0] = (uint8_t)((val >> 8) + 241);
        buf[1] = (uint8_t)(val & 0xFF);
        return 2;
    }
    if (val <= 67823) {
        val -= 2288;
        buf[0] = 249;
        buf[1] = (uint8_t)(val >> 8);
        buf[2] = (uint8_t)(val & 0xFF);
        return 3;
    }
    
    // Determine number of bytes needed
    int bytes;
    if (val <= 0xFFFFFF) bytes = 3;
    else if (val <= 0xFFFFFFFF) bytes = 4;
    else if (val <= 0xFFFFFFFFFF) bytes = 5;
    else if (val <= 0xFFFFFFFFFFFF) bytes = 6;
    else if (val <= 0xFFFFFFFFFFFFFF) bytes = 7;
    else bytes = 8;
    
    buf[0] = 249 + bytes; // 250-255 based on size
    for (int i = bytes; i > 0; i--) {
        buf[i] = (uint8_t)(val & 0xFF);
        val >>= 8;
    }
    return bytes + 1;
}

// Returns number of bytes read
static inline size_t decode_varint(const uint8_t* buf, uint64_t* val) {
    uint8_t a0 = buf[0];
    
    if (a0 <= 240) {
        *val = a0;
        return 1;
    }
    if (a0 <= 248) {
        *val = 240 + 256 * (a0 - 241) + buf[1];
        return 2;
    }
    if (a0 == 249) {
        *val = 2288 + 256 * buf[1] + buf[2];
        return 3;
    }
    
    int bytes = a0 - 249; // Number of bytes following
    uint64_t result = 0;
    for (int i = 1; i <= bytes; i++) {
        result = (result << 8) | buf[i];
    }
    *val = result;
    return bytes + 1;
}

/***********************/
/* MATH HELPER METHODS */
/***********************/

static inline double fast_pow_int(float x, int y) {
    if (y == 0) return 1.0f;
    if (y < 0) return 1.0f / fast_pow_int(x, -y); // Handle negative exponents

    double result = 1.0f;
    while (y > 0) {
        if (y & 1) { // If y is odd
            result *= x;
        }
        x *= x;      // Square x
        y >>= 1;      // Divide y by 2 (right shift)
    }
    return result;
}

/*****************************/
/* HEISTOGRAM HELPER METHODS */
/*****************************/

static inline int16_t get_bucket_id(double value) {
    if (value <= HEIST_MAX_UNMAPPED_BUCKET) return value;
    return (int16_t)((float)log2(value) * HEIST_INV_LOG_GROWTH_FACTOR) - HEIST_BUCKET_MAPPING_DELTA;
}

static inline uint64_t get_bucket_min(uint16_t bid) {
    if(bid <= HEIST_MAX_UNMAPPED_BUCKET) return bid;
    return (uint64_t)ceil(fast_pow_int(1 + HEIST_GROWTH_FACTOR, bid + HEIST_BUCKET_MAPPING_DELTA));
}

static inline uint64_t get_bucket_max(uint32_t min) {
    if(min <= HEIST_MAX_UNMAPPED_BUCKET) return min;
    return (uint64_t)(min * HEIST_GROWTH_FACTOR) + min;
}

static inline size_t encode_bucket(uint32_t count, uint8_t* buffer) {
    uint8_t* ptr = buffer;
    ptr += encode_varint(count, ptr);
    return ptr - buffer;
}

static inline size_t encode_empty_buckets(uint32_t count, uint8_t* buffer) {
    for(int i = 0; i < count; i++){
        buffer[i] = 0;
    }
    return count;
}

static inline size_t decode_header(const uint8_t* buffer, 
    uint16_t* bucket_count, uint64_t* total_count, uint64_t* min, uint64_t* max, uint16_t* min_bucket_id) {
    const uint8_t* ptr = buffer;
    size_t bytes_read;
    uint64_t temp;

    // Read bucket count
    bytes_read = decode_varint(ptr, &temp);
    if (bytes_read == 0) return 0;
    *bucket_count = (uint16_t)temp;
    ptr += bytes_read;

    // Read total count
    bytes_read = decode_varint(ptr, &temp);
    if (bytes_read == 0) return 0;
    *total_count = (uint64_t)temp;
    ptr += bytes_read;

    // Read min value
    bytes_read = decode_varint(ptr, &temp);
    if (bytes_read == 0) return 0;
    *min = (uint64_t)temp;
    ptr += bytes_read;

    // Read max delta value
    bytes_read = decode_varint(ptr, &temp);
    if (bytes_read == 0) return 0;
    *max = (uint64_t)temp + *min;
    ptr += bytes_read;

    // Read max bucket id
    bytes_read = decode_varint(ptr, &temp);
    if (bytes_read == 0) return 0;
    *min_bucket_id = (uint16_t)temp;
    ptr += bytes_read;

    return ptr - buffer;
}

static inline size_t decode_bucket(const uint8_t* buffer, uint64_t* count) {
    const uint8_t* ptr = buffer;
    size_t bytes_read;
    uint64_t temp;

    bytes_read = decode_varint(ptr, &temp);
    if (bytes_read == 0) return 0;
    *count = (uint64_t)temp;
    ptr += bytes_read;

    return ptr - buffer;
}

/**************************/
/* HEISTOGRAM API METHODS */
/**************************/

static Heistogram* heistogram_create(void) {
    Heistogram* h = malloc(sizeof(Heistogram));
    if (!h) return NULL;
    
    h->capacity = 16;
    h->total_count = 0;
    h->max = 0;
    h->min = 0;
    h->min_bucket_id = 0;
    
    // Initialize all buckets with zero count
    h->buckets = calloc(h->capacity, sizeof(Bucket));
    if (!h->buckets) {
        free(h);
        return NULL;
    }
    
    return h;
}

static void heistogram_free(Heistogram* h) {
    if (!h) return;
    free(h->buckets);
    free(h);
}

static uint64_t heistogram_count(const Heistogram* h) {
    return h ? h->total_count : 0;
}

static uint64_t heistogram_max(const Heistogram* h) {
    return h ? h->max : 0;
}

static uint64_t heistogram_min(const Heistogram* h) {
    return h ? h->min : 0;
}

static uint32_t heistogram_memory_size(const Heistogram* h) {
    return h ? sizeof(Heistogram) + (sizeof(Bucket) * h->capacity) : 0;
}

static void heistogram_add(Heistogram* h, uint64_t value) {
    if (!h || value < 0) return;
    
    int16_t bid = get_bucket_id(value);
    if (h->total_count == 0) {
        h->min = value;
        h->max = value;
        h->min_bucket_id = bid;
    } else {
        if (h->min > value) h->min = value;
        if (h->max < value) h->max = value;
        if(bid < h->min_bucket_id) h->min_bucket_id = bid;
    }
    
    // Expand array if needed
    if (bid >= h->capacity) {
        size_t new_capacity = bid + 16;// + 16; // Add some extra space
        Bucket* new_buckets = realloc(h->buckets, new_capacity * sizeof(Bucket));
        if (!new_buckets) return;
        
        // Initialize new buckets to zero
        memset(new_buckets + h->capacity, 0, (new_capacity - h->capacity) * sizeof(Bucket));
        
        h->buckets = new_buckets;
        h->capacity = new_capacity;
    }
    
    // Increment count in the appropriate bucket
    h->buckets[bid].count++;
    h->total_count++;
}

static Heistogram* heistogram_merge(const Heistogram* h1, const Heistogram* h2) {
    if (!h1 || !h2) return NULL;
        
    Heistogram* result = heistogram_create();
    if (!result) return NULL;
    
    // Find the maximum capacity needed
    uint16_t max_capacity = h1->capacity > h2->capacity ? h1->capacity : h2->capacity;
    
    // Resize result to accommodate all buckets
    if (max_capacity > result->capacity) {
        Bucket* new_buckets = realloc(result->buckets, max_capacity * sizeof(Bucket));
        if (!new_buckets) {
            heistogram_free(result);
            return NULL;
        }
        
        // Initialize new buckets to zero
        memset(new_buckets + result->capacity, 0, (max_capacity - result->capacity) * sizeof(Bucket));
        
        result->buckets = new_buckets;
        result->capacity = max_capacity;
    }
    
    // Merge buckets by adding counts
    for (uint16_t i = 0; i < h1->capacity; i++) {
        result->buckets[i].count += h1->buckets[i].count;
    }
    
    for (uint16_t i = 0; i < h2->capacity; i++) {
        result->buckets[i].count += h2->buckets[i].count;
    }
    
    // Update result metadata
    result->total_count = h1->total_count + h2->total_count;
    result->min = h1->min < h2->min ? h1->min : h2->min;
    result->max = h1->max > h2->max ? h1->max : h2->max;
    result->min_bucket_id = h1->min_bucket_id < h2->min_bucket_id ? h1->min_bucket_id : h2->min_bucket_id;

    return result;
}

// NEW: In-place merge of h2 into h1
static int heistogram_merge_inplace(Heistogram* h1, const Heistogram* h2) {
    if (!h1 || !h2) return 0;
    
    // Expand h1 if needed to accommodate h2's buckets
    if (h2->capacity > h1->capacity) {
        Bucket* new_buckets = realloc(h1->buckets, h2->capacity * sizeof(Bucket));
        if (!new_buckets) return 0;
        
        // Initialize new buckets to zero
        memset(new_buckets + h1->capacity, 0, (h2->capacity - h1->capacity) * sizeof(Bucket));
        
        h1->buckets = new_buckets;
        h1->capacity = h2->capacity;
    }
    
    // Merge buckets by adding counts
    for (uint16_t i = 0; i < h2->capacity; i++) {
        h1->buckets[i].count += h2->buckets[i].count;
    }
    
    // Update h1 metadata
    h1->total_count += h2->total_count;
    if (h2->min < h1->min) h1->min = h2->min;
    if (h2->max > h1->max) h1->max = h2->max;
    if (h2->min_bucket_id < h1->min_bucket_id) h1->min_bucket_id = h2->min_bucket_id;
    
    return 1;
}

static double heistogram_percentile(const Heistogram* h, double p) {
    if (!h || p < 0 || p > 100) return 0;

    double target = ((100.0 - p) / 100.0) * h->total_count;
    uint64_t cumsum = 0;
    double pos;
    uint64_t min_val;
    uint64_t max_val;
    //printf("total min is %u, max is %u\n", h->min, h->max);
    for (int16_t i = h->capacity - 1; i >= 0; i--) {
        if (h->buckets[i].count > 0) {
            if (cumsum + h->buckets[i].count >= target) {
                pos = ((double)(target - cumsum)) / (double)h->buckets[i].count;
                min_val = get_bucket_min(i);
                max_val = get_bucket_max(min_val);

                if (max_val > h->max) max_val = h->max;
                if (min_val < h->min) min_val = h->min;
                //printf("in bucket %u, max_bucket_id is %u, looking for pos %f in count %u, min is %u, max is %u\n", i, h->capacity - 1, pos, h->buckets[i].count, min_val, max_val);

                return (max_val) - pos * (max_val - min_val);
            }
            cumsum += h->buckets[i].count;
        }
    }
    
    return h->min;
}

// New function to calculate multiple percentiles in one pass
static void heistogram_percentiles(const Heistogram* h, const double* percentiles, size_t num_percentiles, double* results) {
    if (!h || !percentiles || !results || num_percentiles == 0) return;
    
    // Copy and sort percentiles in descending order
    double* p_sorted = malloc(num_percentiles * sizeof(double));
    if (!p_sorted) return;
    
    memcpy(p_sorted, percentiles, num_percentiles * sizeof(double));
    for (size_t i = 0; i < num_percentiles; i++) {
        for (size_t j = i + 1; j < num_percentiles; j++) {
            if (p_sorted[i] < p_sorted[j]) {
                double temp = p_sorted[i];
                p_sorted[i] = p_sorted[j];
                p_sorted[j] = temp;
            }
        }
    }
    
    // Create a mapping from sorted to original positions
    size_t* p_indices = malloc(num_percentiles * sizeof(size_t));
    if (!p_indices) {
        free(p_sorted);
        return;
    }
    
    for (size_t i = 0; i < num_percentiles; i++) {
        for (size_t j = 0; j < num_percentiles; j++) {
            if (p_sorted[i] == percentiles[j]) {
                p_indices[i] = j;
                break;
            }
        }
    }
    
    // Process all percentiles in one pass, from higher percentiles to lower
    size_t current_p = 0;
    uint64_t cumsum = 0;
    
    for (int16_t i = h->capacity - 1; i >= 0 && current_p < num_percentiles; i--) {
        if (h->buckets[i].count > 0) {
            while (current_p < num_percentiles) {
                double target = ((100.0 - p_sorted[current_p]) / 100.0) * h->total_count;
                
                if (cumsum + h->buckets[i].count >= target) {
                    double pos = ((double)(target - cumsum)) / (double)h->buckets[i].count;
                    uint64_t min_val = get_bucket_min(i);
                    uint64_t max_val = get_bucket_max(min_val);
                    if (max_val > h->max) max_val = h->max;
                    if (min_val < h->min) min_val = h->min;
                    
                    results[p_indices[current_p]] = max_val - pos * (max_val - min_val);
                    current_p++;
                } else {
                    break;
                }
            }
            cumsum += h->buckets[i].count;
        }
    }
    
    // Handle remaining percentiles (if any)
    while (current_p < num_percentiles) {
        results[p_indices[current_p]] = h->min;
        current_p++;
    }
    
    free(p_sorted);
    free(p_indices);
}

static double heistogram_prank(const Heistogram* h, double value) {
    if (!h || value < 0) return 0;
    if (value >= h->max) return h->max;
    
    int16_t bid = get_bucket_id(value);
    if (bid >= h->capacity) return 100;
    
    uint64_t cumsum = 0;
    for (int16_t i = 0; i < bid; i++) {
        cumsum += h->buckets[i].count;
    }
    
    // Calculate position within the bucket
    uint64_t min_val = get_bucket_min(bid);
    uint64_t max_val = get_bucket_max(min_val);
    double pos;
    if (max_val == min_val) {
        pos = 0.5;
    } else {
        pos = ((value) - (min_val)) / ((max_val) - (min_val));
    }
    
    return 100.0 * (cumsum + pos * h->buckets[bid].count) / (h->total_count);
}

static inline void* heistogram_serialize(const Heistogram* h, size_t* size) {
    if (!h || !size) return NULL;
    
    // Find the highest used bucket ID
    int16_t max_bucket_id = h->capacity - 1;
    //printf("original bucket count %u\n", h->capacity);
   
    while (max_bucket_id >= 0 && h->buckets[max_bucket_id].count == 0) {
        max_bucket_id--;
    }

    //printf("max bucket id is %u and its count is %lu\n", max_bucket_id, h->buckets[max_bucket_id].count);
    //printf("min bucket id is %u\n", h->min_bucket_id);

    // Max varint size is 9 bytes, allocate maximum possible size
    size_t max_var_size = 9;
    // We store: error_margin, max_bucket_id, total_count, min, max
    size_t header_size = 5 * max_var_size;
    // For each bucket we store the count
    size_t bucket_size = max_var_size;
    size_t max_total_size = header_size + ((max_bucket_id - h->min_bucket_id + 1) * bucket_size);
    
    uint8_t* buffer = malloc(max_total_size);
    if (!buffer) return NULL;
    
    uint8_t* ptr = buffer;
    //printf("encoding bucket count %u\n", max_bucket_id - h->min_bucket_id + 1);
    ptr += encode_varint(max_bucket_id - h->min_bucket_id + 1, ptr); // Number of buckets to store
    ptr += encode_varint(h->total_count, ptr);
    ptr += encode_varint(h->min, ptr);
    ptr += encode_varint(h->max - h->min, ptr);
    ptr += encode_varint(h->min_bucket_id, ptr);
    
    // Write buckets in reverse order (higher ids first)
    for (int16_t i = max_bucket_id; i >= h->min_bucket_id; i--) {
        //printf("%lu-", h->buckets[i].count);
        ptr += encode_bucket(h->buckets[i].count, ptr);
    }
    //printf("\n");
    // Calculate actual size used
    *size = ptr - buffer;
    
    // Reallocate to actual size
    buffer = realloc(buffer, *size);
    return buffer;
}

static inline Heistogram* heistogram_deserialize(const void* buffer, size_t size) {
    if (!buffer) return NULL;
    
    const uint8_t* ptr = buffer;
    uint16_t bucket_count;
    uint64_t total_count;
    uint64_t min;
    uint64_t max;
    uint16_t min_bucket_id;
    
    // Read header
    size_t bytes_read = decode_header(ptr, &bucket_count, &total_count, &min, &max, &min_bucket_id);
    if (bytes_read == 0) return NULL;
    
    ptr += bytes_read;
    
    uint16_t max_bucket_id = min_bucket_id + bucket_count - 1;

    Heistogram* h = heistogram_create();
    if (!h) return NULL;
    
    // Ensure we have enough capacity
    if (max_bucket_id >= h->capacity) {
        Bucket* new_buckets = realloc(h->buckets, (max_bucket_id + 1) * sizeof(Bucket));
        if (!new_buckets) {
            heistogram_free(h);
            return NULL;
        }
        
        // Initialize new buckets to zero
        memset(new_buckets + h->capacity, 0, (max_bucket_id + 1 - h->capacity) * sizeof(Bucket));
        
        h->buckets = new_buckets;
        h->capacity = max_bucket_id + 1;
    }
    
    h->total_count = total_count;
    h->min = min;
    h->max = max;
    
    // Read buckets in reverse order
    uint64_t count;
    for (int16_t i = max_bucket_id; i >= min_bucket_id; i--) {
        bytes_read = decode_bucket(ptr, &count);
        if (bytes_read == 0) {
            heistogram_free(h);
            return NULL;
        }
        ptr += bytes_read;
        h->buckets[i].count = count;
    }
    for (int16_t i = 0; i < min_bucket_id && i < 16; i++){
        h->buckets[i].count = 0;
    }
    return h;
}

/*********************************************
    FUNCTIONS OPERATING ON SERIALIZED DATA
**********************************************/

// Updated heistogram_percentile_serialized function
static double heistogram_percentile_serialized(const void* buffer, size_t size, double p) {
    if (!buffer || size < 3) return 0;  // Minimum size check for header

    const uint8_t* ptr = buffer;
    uint16_t bucket_count;   
    uint64_t total_count;
    uint64_t min;
    uint64_t max;
    uint16_t min_bucket_id;
    // Decode the header
    size_t bytes_read = decode_header(ptr, &bucket_count, &total_count, &min, &max, &min_bucket_id);
    if (bytes_read == 0) return 0;

    ptr += bytes_read;

    uint16_t max_bucket_id = min_bucket_id + bucket_count - 1;
    
    double target = ((100.0 - p) / 100.0) * total_count;
    uint64_t cumsum = 0;
    uint64_t count;
    //printf("total min is %u, max is %u\n", min, max);

    // Process buckets in reverse order (higher IDs first)
    for (int16_t i = max_bucket_id; i >= min_bucket_id; i--) {
        bytes_read = decode_bucket(ptr, &count);
        if (bytes_read == 0) return 0;
        ptr += bytes_read;
        
        if (count > 0 && cumsum + count >= target) {
            double pos = ((double)(target - cumsum)) / (double)count;
            uint64_t min_val = get_bucket_min(i);
            uint64_t max_val = get_bucket_max(min_val);
            if (max_val > max) max_val = max;
            if (min_val < min) min_val = min;
            //printf("in bucket %u, max_bucket_id is %u, looking for pos %f in count %u, min is %u, max is %u\n", i, max_bucket_id, pos, count, min_val, max_val);
            return max_val - pos * (max_val - min_val);
        }
        cumsum += count;
    }

    return min;
}

// New function to calculate multiple percentiles from serialized data in one pass
// Fixed function to calculate multiple percentiles from serialized data in one pass
static void heistogram_percentiles_serialized(const void* buffer, size_t size, const double* percentiles, size_t num_percentiles, double* results) {
    if (!buffer || size < 3 || !percentiles || !results || num_percentiles == 0) return;
    
    const uint8_t* ptr = buffer;
    uint16_t bucket_count;   
    uint64_t total_count;
    uint64_t min;
    uint64_t max;
    uint16_t min_bucket_id;
    
    // Decode the header
    size_t bytes_read = decode_header(ptr, &bucket_count, &total_count, &min, &max, &min_bucket_id);
    if (bytes_read == 0) return;
    ptr += bytes_read;
    
    uint16_t max_bucket_id = min_bucket_id + bucket_count - 1;
    
    // Copy and sort percentiles in descending order
    double* p_sorted = malloc(num_percentiles * sizeof(double));
    if (!p_sorted) return;
    
    memcpy(p_sorted, percentiles, num_percentiles * sizeof(double));
    for (size_t i = 0; i < num_percentiles; i++) {
        for (size_t j = i + 1; j < num_percentiles; j++) {
            if (p_sorted[i] < p_sorted[j]) {
                double temp = p_sorted[i];
                p_sorted[i] = p_sorted[j];
                p_sorted[j] = temp;
            }
        }
    }
    
    // Create a mapping from sorted to original positions
    size_t* p_indices = malloc(num_percentiles * sizeof(size_t));
    if (!p_indices) {
        free(p_sorted);
        return;
    }
    
    for (size_t i = 0; i < num_percentiles; i++) {
        for (size_t j = 0; j < num_percentiles; j++) {
            if (p_sorted[i] == percentiles[j]) {
                p_indices[i] = j;
                break;
            }
        }
    }
    
    // Process all percentiles in one pass
    size_t current_p = 0;
    uint64_t cumsum = 0;
    uint64_t count;
    
    // Process buckets in reverse order (higher IDs first)
    for (int16_t i = max_bucket_id; i >= min_bucket_id && current_p < num_percentiles; i--) {
        bytes_read = decode_bucket(ptr, &count);
        if (bytes_read == 0) {
            free(p_sorted);
            free(p_indices);
            return;
        }
        ptr += bytes_read;
        
        if (count > 0) {
            while (current_p < num_percentiles) {
                double target = ((100.0 - p_sorted[current_p]) / 100.0) * total_count;
                
                if (cumsum + count >= target) {
                    double pos = ((double)(target - cumsum)) / (double)count;
                    uint64_t min_val = get_bucket_min(i);
                    uint64_t max_val = get_bucket_max(min_val);
                    if (max_val > max) max_val = max;
                    if (min_val < min) min_val = min;
                    
                    results[p_indices[current_p]] = max_val - pos * (max_val - min_val);
                    current_p++;
                } else {
                    break;
                }
            }
            cumsum += count;
        }
    }
    
    // Handle remaining percentiles (if any)
    while (current_p < num_percentiles) {
        results[p_indices[current_p]] = min;
        current_p++;
    }
    
    free(p_sorted);
    free(p_indices);
}

// Fixed function to merge in-memory Heistogram with serialized Heistogram
static Heistogram* heistogram_merge_serialized(const Heistogram* h, const void* buffer, size_t size) {
    if (!h || !buffer || size < 3) return NULL;
    
    const uint8_t* ptr = buffer;
    uint16_t bucket_count;
    uint64_t total_count;
    uint64_t min;
    uint64_t max;
    uint16_t min_bucket_id;
    
    // Decode the header
    size_t bytes_read = decode_header(ptr, &bucket_count, &total_count, &min, &max, &min_bucket_id);
    if (bytes_read == 0) return NULL;
    ptr += bytes_read;
    
    uint16_t max_bucket_id = min_bucket_id + bucket_count - 1;
    
    // Create a new Heistogram for the result
    Heistogram* result = heistogram_create();
    if (!result) return NULL;
    
    // Find the maximum capacity needed
    uint16_t new_capacity = h->capacity > (max_bucket_id + 1) ? h->capacity : (max_bucket_id + 1);
    
    // Resize result to accommodate all buckets
    if (new_capacity > result->capacity) {
        Bucket* new_buckets = realloc(result->buckets, new_capacity * sizeof(Bucket));
        if (!new_buckets) {
            heistogram_free(result);
            return NULL;
        }
        
        // Initialize new buckets to zero
        memset(new_buckets + result->capacity, 0, (new_capacity - result->capacity) * sizeof(Bucket));
        
        result->buckets = new_buckets;
        result->capacity = new_capacity;
    }
    
    // Copy counts from h
    for (uint16_t i = 0; i < h->capacity; i++) {
        result->buckets[i].count += h->buckets[i].count;
    }
    
    // Add counts from serialized data
    uint64_t count;
    for (int16_t i = max_bucket_id; i >= min_bucket_id; i--) {
        bytes_read = decode_bucket(ptr, &count);
        if (bytes_read == 0) {
            heistogram_free(result);
            return NULL;
        }
        ptr += bytes_read;
        
        result->buckets[i].count += count;
    }
    
    // Update result metadata
    result->total_count = h->total_count + total_count;
    result->min = h->min < min ? h->min : min;
    result->max = h->max > max ? h->max : max;
    result->min_bucket_id = h->min_bucket_id < min_bucket_id ? h->min_bucket_id : min_bucket_id;
    
    return result;
}

// Fixed function to merge two serialized Heistograms
static Heistogram* heistogram_merge_two_serialized(const void* buffer1, size_t size1, const void* buffer2, size_t size2) {
    if (!buffer1 || size1 < 3 || !buffer2 || size2 < 3) return NULL;
    
    // Decode headers
    const uint8_t* ptr1 = buffer1;
    uint16_t bucket_count1;
    uint64_t total_count1;
    uint64_t min1;
    uint64_t max1;
    uint16_t min_bucket_id1;
    
    size_t bytes_read1 = decode_header(ptr1, &bucket_count1, &total_count1, &min1, &max1, &min_bucket_id1);
    if (bytes_read1 == 0) return NULL;
    ptr1 += bytes_read1;
    
    uint16_t max_bucket_id1 = min_bucket_id1 + bucket_count1 - 1;
    
    const uint8_t* ptr2 = buffer2;
    uint16_t bucket_count2;
    uint64_t total_count2;
    uint64_t min2;
    uint64_t max2;
    uint16_t min_bucket_id2;
    
    size_t bytes_read2 = decode_header(ptr2, &bucket_count2, &total_count2, &min2, &max2, &min_bucket_id2);
    if (bytes_read2 == 0) return NULL;
    ptr2 += bytes_read2;
    
    uint16_t max_bucket_id2 = min_bucket_id2 + bucket_count2 - 1;
    
    // Create a new Heistogram for the result
    Heistogram* result = heistogram_create();
    if (!result) return NULL;
    
    // Find the maximum capacity needed
    uint16_t max_bucket_id = max_bucket_id1 > max_bucket_id2 ? max_bucket_id1 : max_bucket_id2;
    uint16_t min_bucket_id = min_bucket_id1 < min_bucket_id2 ? min_bucket_id1 : min_bucket_id2;
    uint16_t new_capacity = max_bucket_id + 1;
    
    // Resize result to accommodate all buckets
    if (new_capacity > result->capacity) {
        Bucket* new_buckets = realloc(result->buckets, new_capacity * sizeof(Bucket));
        if (!new_buckets) {
            heistogram_free(result);
            return NULL;
        }
        
        // Initialize new buckets to zero
        memset(new_buckets + result->capacity, 0, (new_capacity - result->capacity) * sizeof(Bucket));
        
        result->buckets = new_buckets;
        result->capacity = new_capacity;
    }
    
    // Read counts from first serialized Heistogram
    uint64_t count;
    for (int16_t i = max_bucket_id1; i >= min_bucket_id1; i--) {
        bytes_read1 = decode_bucket(ptr1, &count);
        if (bytes_read1 == 0) {
            heistogram_free(result);
            return NULL;
        }
        ptr1 += bytes_read1;
        result->buckets[i].count += count;
    }
    
    // Read counts from second serialized Heistogram
    for (int16_t i = max_bucket_id2; i >= min_bucket_id2; i--) {
        bytes_read2 = decode_bucket(ptr2, &count);
        if (bytes_read2 == 0) {
            heistogram_free(result);
            return NULL;
        }
        ptr2 += bytes_read2;
        result->buckets[i].count += count;
    }
    
    // Update result metadata
    result->total_count = total_count1 + total_count2;
    result->min = min1 < min2 ? min1 : min2;
    result->max = max1 > max2 ? max1 : max2;
    result->min_bucket_id = min_bucket_id;
    
    return result;
}

// New function to merge serialized Heistogram into an existing in-memory Heistogram
static int heistogram_merge_inplace_serialized(Heistogram* h, const void* buffer, size_t size) {
    if (!h || !buffer || size < 3) return 0;
    
    const uint8_t* ptr = buffer;
    uint16_t bucket_count;
    uint64_t total_count;
    uint64_t min;
    uint64_t max;
    uint16_t min_bucket_id;
    
    // Decode the header
    size_t bytes_read = decode_header(ptr, &bucket_count, &total_count, &min, &max, &min_bucket_id);
    if (bytes_read == 0) return 0;
    ptr += bytes_read;
    
    uint16_t max_bucket_id = min_bucket_id + bucket_count - 1;
    
    // Expand h if needed to accommodate serialized Heistogram's buckets
    if (max_bucket_id >= h->capacity) {
        uint16_t new_capacity = max_bucket_id + 1;
        Bucket* new_buckets = realloc(h->buckets, new_capacity * sizeof(Bucket));
        if (!new_buckets) return 0;
        
        // Initialize new buckets to zero
        memset(new_buckets + h->capacity, 0, (new_capacity - h->capacity) * sizeof(Bucket));
        
        h->buckets = new_buckets;
        h->capacity = new_capacity;
    }
    
    // Add counts from serialized data
    uint64_t count;
    for (int16_t i = max_bucket_id; i >= min_bucket_id; i--) {
        bytes_read = decode_bucket(ptr, &count);
        if (bytes_read == 0) return 0;
        ptr += bytes_read;
        
        h->buckets[i].count += count;
    }
    
    // Update h metadata
    h->total_count += total_count;
    if (min < h->min) h->min = min;
    if (max > h->max) h->max = max;
    if (min_bucket_id < h->min_bucket_id) h->min_bucket_id = min_bucket_id;
    
    return 1;
}

#endif /* HEISTOGRAM_H */
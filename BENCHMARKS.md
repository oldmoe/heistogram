# Benchmarks

All tests were conducted on a Ryzen 7 7840HS CPU in a VirtualBox environment.

The command line used to compile the benchmark was:

```bash
  gcc -O3 -march=native -o bench ./bench.c -lm
```

## Lognoram Distribution
In this benchmark a dataset of 10M elements was created according to lognormal distribution

The data spread denotes the difference between min (0) and max values

### Runtime of various API methods (in nanoseconds)

Data Spread	|10^1	|10^2	|10^3	|10^4	|10^5	|10^6	|10^7	|10^8	|10^9
---|---|---|---|---|---|---|---|---|---
Insert|	2.1|	2.4|	11.1|	12.7|	12.8|	12.8|	12.8|	13.7|	12.922
Percentile-query|	5.3|	22.9|	28.4|	39.2|	35.5|	33.4|	40.1|	38.9|	37.198
Percentiles-query (per p value)|	11.2|	23.1|	31.9|	33.3|	33.4|	32.8|	33.7|	34.6|	34.032
Merge|	46.4|	77.3|	93.1|	111.2|	132.3|	138.3|	172.7|	176.9|	199.271
Merge-in-place|	2.4|	7.4|	14.0|	23.6|	31.9|	38.5|	47.8|	56.0|	63.965
Serialize	|76.7	|149.5	|252.7	|403.4	|446.3	|513.7	|577.9	|582.9|	646.949
Deserialize|	62.1|	186.4|	303.3|	418.7|	503.5|	537.4|	621.5|	651.3|	707.736
Precentile-serialized|	15.5|	64.4|	75.6|	92.1|	88.3|	80.9|	94.1|	95.1|	89.651
Percentiles-serialized (per p value)|	12.8|	37.8|	51.8|	56.1|	56.9|	52.8|	59.2|	56.8|	56.006
Merge-serialized|	68.9|	202.5|	332.5|	537.4|	593.2|	646.3|	700.7|	752.4|	835.912
Merge-in-place-serialized|	32.8|	164.5|	361.5|	625.0|	819.4|	942.4|	1133.1|	1333.6|	1481.383
Merge-two-serialized|	87.1|	369.9|	693.5|	1168.3|	1377.2|	1600.6|	1891.1|	2214.0|	2381.361|

### Serialized data size (in bytes)
                  
Data Spread	|10^1	|10^2	|10^3	|10^4	|10^5	|10^6	|10^7	|10^8	|10^9
---|---|---|---|---|---|---|---|---|---
Heistogram serialized data size|	53|	278|	615|	891|	1002|	1120|	1231|	1355|	1482

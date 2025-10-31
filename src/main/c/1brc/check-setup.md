## Perf results 

- check ssd speed `sudo hdparm -Tt /dev/nvme0n1p5`

```shell
/dev/nvme0n1p5:
Timing cached reads:   35240 MB in  1.98 seconds = 17795.65 MB/sec
Timing buffered disk reads: 2566 MB in  3.00 seconds = 854.82 MB/sec

```

-- check ssd O_DIRECT metrics
```shell
sudo hdparm -tT --direct /dev/nvme0n1p5

/dev/nvme0n1p5:
 Timing O_DIRECT cached reads:   2838 MB in  2.00 seconds = 1419.15 MB/sec
 Timing O_DIRECT disk reads: 4440 MB in  3.00 seconds = 1479.96 MB/sec


- cached reads from OS buffer caches not disk
- buffered disk reads come from disk internal buffers
- direct disk reads avoid both OS cache and buffer

```
- check cpu frequency `cpupower frequency-info`
- set min freq `sudo cpupower frequency-set -d 2.75`


```shell
isaiahp@ip-xps15:~/workspace/1billionRC/src/main/c/1brc$ ./linecnt-a ~/Downloads/measurements.txt 
lines 1000000000
bytes read  13795425605
time: 5248.990 millis
isaiahp@ip-xps15:~/workspace/1billionRC/src/main/c/1brc$ ./linecnt-b ~/Downloads/measurements.txt 
lines 1000000000
bytes read  13795425605
time: 4977.168 millis
isaiahp@ip-xps15:~/workspace/1billionRC/src/main/c/1brc$ time wc -l ~/Downloads/measurements.txt 
1000000000 /home/isaiahp/Downloads/measurements.txt

real	0m13.869s
user	0m0.500s
sys	0m4.065s

```
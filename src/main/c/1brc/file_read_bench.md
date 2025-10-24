## Reading files Bench

Ensure page caches are clear, so you get a true benchmark
`sudo sysctl -w vm.drop_caches=1`
###  read 

```shell
isaiahp@ip-xps15:~/workspace/1billionRC/src/main/c/1brc$ ./read_bench /home/isaiahp/workspace/1billionRC/data/measurements.txt 
total: 13795425605
time: 17999.953 millis

```
read with hint posix_fadvise, provides an approx 3 second speed up
```c++
const int err = posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);
```

```shell
isaiahp@ip-xps15:~/workspace/1billionRC/src/main/c/1brc$ ./read_bench /home/isaiahp/workspace/1billionRC/data/measurements.txt 
total: 13795425605
time: 15408.871 millis

```
### O_DIRECT 

[O_DIRECT](https://yarchive.net/comp/linux/o_direct.html)
O_DIRECT isn't doing any read-ahead.

For O_DIRECT to be a win, you need to make it asynchronous.

current bench 
```shell
isaiahp@ip-xps15:~/workspace/1billionRC/src/main/c/1brc$ ./read_bench /home/isaiahp/workspace/1billionRC/data/measurements.txt 
total: 13795425605
time: 114356.055 millis

```

worth trying O_DIRECT with io_uring 
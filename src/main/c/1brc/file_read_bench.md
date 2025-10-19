## Reading files Bench

###  read 

```shell
isaiahp@ip-xps15:~/workspace/1billionRC/src/main/c/1brc$ ./read_bench /home/isaiahp/workspace/1billionRC/data/measurements.txt 
total: 13795425605
time: 17999.953 millis

```

### O_DIRECT 

O_DIRECT isn't doing any read-ahead.

For O_DIRECT to be a win, you need to make it asynchronous.

current bench 
```shell
isaiahp@ip-xps15:~/workspace/1billionRC/src/main/c/1brc$ ./read_bench /home/isaiahp/workspace/1billionRC/data/measurements.txt 
total: 13795425605
time: 114356.055 millis

```

worth trying O_DIRECT with io_uring 
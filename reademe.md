## Install perf
`sudo apt-get install linux-tools-$(uname -r) linux-tools-generic -y`

if perf still doesnt show up, check 
use the shell script to install it

### perf_event_paranoid
`sudo sysctl kernel.perf_event_paranoid=-1`

### Add flame graph submodule

`git add submodule https://github.com/brendangregg/FlameGraph.git`



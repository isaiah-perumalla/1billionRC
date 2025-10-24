## Build the liburing submodule
```shell
./configure --prefix=../local-build
make
make install
```
should see the libs 
```shell
ls local-build/lib/
liburing.a  liburing-ffi.a  liburing-ffi.so  liburing-ffi.so.2  liburing-ffi.so.2.13  liburing.so  liburing.so.2  liburing.so.2.13  pkgconfig

```
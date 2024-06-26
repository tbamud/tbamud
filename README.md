Files for tbaMUD.

---

## To build

1. run configure: `./configure`
2. build the code: `cd src && make`
3. run the mud: `cd ..; bin/circle 1234`
4. connect via telnet to port 1234.

Read more in the doc/ folder

## To run the tests

1. clone the munit library into src/munit. It is registered as a submodule in git 

`git submodule init && git submodule update`

2. install the cmocka-library: `sudo apt install libcmocka-dev`
3. `./config.status && cd src && make test`
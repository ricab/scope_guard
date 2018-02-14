# scope_guard
A C++11 scope guard.

#### Usage
To use simply clone this repository, copy the header file within, and include 
it.

#### Running tests
To execute the tests:

1. obtain a copy of [Catch](https://github.com/catchorg/Catch2) and install it
with `cmake ...`, `make`, `make install` (refer to their instructions for 
details)
2. clone this repo into a directory of your choice (let's call this 
`<src_path>`)
2. create a directory of your choice to build the tests (let's call this 
`<build_path>`)
3. `cd <build_path>`
4. `cmake <src_path>`
5. `make`
6. `make test`

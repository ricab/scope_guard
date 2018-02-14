# scope_guard
A C++11 scope guard. [under construction]

#### Usage
To use, simply clone this repository, copy the header file within, and include 
it.

#### Running tests
To execute the tests:

* Get and install [Catch2](https://github.com/catchorg/Catch2):

        $ git clone https://github.com/catchorg/Catch2 <catch_src_dir>
        $ mkdir <catch_bin_dir>
        $ cd <catch_bin_dir>
        $ cmake <catch_src_dir>
        $ make
        $ make install

* Clone and build this repo:

        $ git clone https://github.com/ricab/scope_guard.git <guard_src_dir>
        $ mkdir <guard_bin_dir>
        $ cd <guard_bin_dir>
        $ cmake <guard_src_dir>
        $ make
        $ make test

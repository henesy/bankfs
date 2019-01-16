# Tests

## Usage

Note: All tests are self-contained and do not rebuild the current source.

They call 6.out and as such are intended to be called from the base src dir in the format of `tests/demo.rc` for example.

If you wish to further play with the state interactively, simply comment out the `rfork n;` line at the beginning and the script should run within your current namespace. 

## Index

- demo.rc — demonstrates basic initialization of a bank
- build-tear.rc — performs the construction of a populated bank and tears it down


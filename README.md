# SimpleFS: A User-Space Filesystem Emulation in C

This project is a C-based implementation of a simple, user-space filesystem that runs over an emulated disk. It provides a basic API for file operations including creating, deleting, opening, closing, reading, writing, and seeking within files.

The filesystem is built on a block-based disk emulation, with a superblock to manage filesystem metadata, a fixed number of inodes to represent files, and a set of data blocks for file content.

## Features

The following core filesystem operations have been implemented in `src/simplefs-ops.c`:

*   `simplefs_create(char *filename)`: Creates a new file on the emulated disk.
*   `simplefs_delete(char *filename)`: Deletes a file from the disk.
*   `simplefs_open(char *filename)`: Opens an existing file and provides a file handle.
*   `simplefs_close(int file_handle)`: Closes an open file handle.
*   `simplefs_read(int file_handle, char *buf, int nbytes)`: Reads `nbytes` from a file into a buffer.
*   `simplefs_write(int file_handle, char *buf, int nbytes)`: Writes `nbytes` from a buffer to a file.
*   `simplefs_seek(int file_handle, int nseek)`: Changes the current read/write offset within a file.

## Project Structure

The project is organized into a clean and manageable directory structure:

```
.
├── Makefile            # Automates building, testing, and cleaning
├── bin/                # Compiled executables are placed here
├── expected_output/    # Provided expected output for test cases
├── include/            # Header files (*.h)
├── output/             # Actual output from test runs
├── src/                # Source code files (*.c)
└── testcases/          # Test case source files
```

## Building and Testing

A `Makefile` is included to automate the entire build and test process.

### 1. Build All Test Cases

To compile all test programs, simply run `make`:

```bash
make
```

This command will compile all test sources from the `testcases/` directory and place the resulting executables into the `bin/` directory.

### 2. Run All Tests

To execute all test cases and automatically verify their output against the expected results, run:

```bash
make test
```

This command will run each test case, save its output to the `output/` directory, and compare it with the corresponding file in `expected_output/`. The results will be printed to the console, clearly indicating which tests have `PASSED` or `FAILED`.

### 3. Clean the Project

To remove all generated files (executables in `bin/`, test results in `output/`, and the `simplefs` disk file), run:

```bash
make clean
```
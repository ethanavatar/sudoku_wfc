# Sudoku Wave Function Collapse

A visual demonstration of the wave function collapse algorithm applied to a Sudoku board.

## Demo

Keys:
- `R` - Reset the board
- `Z` - Undo the last action (only 1 level of undo)

![Demo](./demo/sudoku_wfc_2x_24fps.mp4)

## Usage

Clone the repository recursively to include the submodules.

```bash
# Pre git 2.13
$ git clone --recursive

# Post git 2.13
$ git clone --recurse-submodules
```

Use `make` to build the program and its dependencies:

```bash
$ make all
```

Run the program:

```bash
$ ./bin/sudoku
```

or:

```bash
$ make run
```


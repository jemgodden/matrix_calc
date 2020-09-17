# matrix_calc

This program has the ability to perform multiple different operations on one or more input matrices.
This program can: calculate the Frobenius Norm of a single matrix, find the Transpose of a single matrix, find the Product of two matrices, find the Determinant of a single matrix, find the Adjoint of a single matrix and find the Inverse of a single matrix.

# Downloading

The easiest way to download the code is to press the download button on the home page of the GitHub repository.

# Requirements

To run the program a C compiler is required, along with the following libraries: stdio.h, stdlib.h, memory.h, math.h.

# Use

This program will run from the command line, after being compiled. 
The command line arguments take the form:

./matrix_calc -operation -input_file1 (-input_file2) (-output_file)

The operations available are: f, t, p, d, a and i, which all respectively correlate to the functions explained at the top of this file.

The input file must take the form:
The first line states the size of the matrix, with two vlaues stating the rows and columns of the matrix, respectively.
Each line after this have the rows of each matrix printed, each value being separated by a tab.

If no output file is given, the matrix is automatically printed to stdout.

# Log

Initial version uploaded to GitHub.

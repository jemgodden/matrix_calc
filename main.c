/*
 Title:   Matrix Calculator
 Author:  Jeremy Godden
*/

/* Packages used throughout program. */
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>

/*
 This program, 'matrix_calc.c', has the ability to perform multiple different operations on one or more input matrices.
 The possible operations are:
    -Finding the frobenius norm (-f) of a matrix, requiring one input matrix file.
    -Finding the transpose (-t) of a matrix, requiring one input matrix file and an optional output file.
    -Finding the product (-m) of two matrices, requiring two input matrix files and an optional output file.
    -Finding the determinant (-d) of a matrix, requiring one input matrix file.
    -Finding the adjoint (-a) of a matrix, requiring one input matrix file and an optional output file.
    -Finding the inverse (-i) of a matrix, requiring one input matrix file and an optional output file.

 An example of the command line arguments would be:
    ./matrix_calc -i matrix_1.txt output_matrix.txt
 This will read in the file matrix_1.txt, calculate the inverse of the matrix inside and print the inverse
 to the file output_matrix.txt.

 If no output file is given, the matrix is automatically printed to stdout.
 More information on this can be found in the help() function below.

 The input file is expected to be in the same form as that given by mat_gen.c and the output file of this program.
 Matrix files will be read in a way to ignore any blank lines and anything after a #.
 If the file is not as expected in any way, an error message will be displayed.

 Checks will be run on the input matrices to make sure that the selected operations are able to run.
 If not an error message will be displayed.
*/

static const char * VERSION  = "1.0.1";
static const char * REV_DATE = "30-Oct-2019";

/* Defined variables used within the code. */
#define OPERATION_ARGUMENT 1
#define OPERATION_INPUT_LENGTH 2
#define INPUT_FILE_1 2
#define INPUT_FILE_2 3
#define NO_ARGS_f_d 3
#define MIN_ARGS_t_a_i 3
#define MAX_ARGS_t_a_i 4
#define MIN_ARGS_m 4
#define MAX_ARGS_m 5
#define MAX_LINE_LENGTH 40000 /* Maximum line length taken from file. */
#define MAX_ROWS_COLS 2000 /* Maximum size of matrix in file. */
#define TOKEN_SEPARATORS " \t\r\n" /* All string separators expected in file. */

/* Constants for giving out errors. */
typedef enum error{
    NO_ERROR = 0,
    INCORRECT_ARGUMENTS = 1,
    MEMORY_ERROR = 2,
    FILE_OPEN_ERROR = 3,
    INVALID_FILE = 4,
    INVALID_MATRIX = 5,
} Error;

/* Structure to give context to the error when program is exited. */
typedef struct context{
    FILE *file;
    char *file_name;
    char *token;
    int line_number;
} Context;

/* Structure to hold information about a matrix. */
typedef struct matrix{
    int rows;
    int cols;
    double *values;
} Matrix;

/* Function to print out help for stating appropriate command line arguments. */
void help(char *argv[]){
    fprintf(stderr, "Incorrect operation %s or incorrect command line arguments.\n\n", argv[OPERATION_ARGUMENT]);
    fprintf(stderr, "Please choose one of the following operations and enter the correct command line arguments:\n"
            "'-f': Frobenius Norm : ./matrix_calc -f input_file\n"
            "'-t': Transpose : ./matrix_calc -t input_file (output_file)\n"
            "'-m': Matrix Product : ./matrix_calc -m input_file_1 input_file_2 (output_file)\n"
            "'-d': Determinant : ./matrix_calc -d input_file\n"
            "'-a': Adjoint : ./matrix_calc -a input_file (output_file)\n"
            "'-i': Inverse : ./matrix_calc -i input_file (output_file)\n\n");
    fprintf(stderr, "The (output file) is optional. If no file is given the matrix will be written to stdout.\n\n");
}

/* Function to exit program and give an error when malloc fails. */
void exit_malloc_failed(){
    fprintf(stderr, "Memory could not be allocated.\n");
    exit(MEMORY_ERROR);
}

/* Function to exit program and give an error when a file cannot be opened. */
void exit_open_failed(const char *file_name){
    fprintf(stderr, "Error opening the file %s.\n", file_name);
    exit(FILE_OPEN_ERROR);
}

/* Function to exit program, close the file being read and give an error
 * message when the file being read is invalid. */
void exit_invalid_file(Context *context, const char *message){
    fprintf(stderr, "%s is an invalid matrix file. %s\n", context->file_name, message);
    fprintf(stderr, "The invalid string in line %d of the file is\n%s\n", context->line_number, context->token);

    fclose(context->file);
    exit(INVALID_FILE);
}

/* Function to create and allocate memory for a structure storing a matrix. */
Matrix *create_matrix(const int rows, const int cols){
    /* Allocates memory for the matrix structure. */
    Matrix *matrix = malloc(sizeof(Matrix));
    if (matrix == NULL) {
        exit_malloc_failed();
    }

    matrix->rows = rows;
    matrix->cols = cols;

    /* Allocates memory for the array of matrix elements. */
    matrix->values = malloc(sizeof(double) * rows * cols);
    if (matrix->values == NULL) {
        exit_malloc_failed();
    }

    return matrix;
}

/* Function to free the memory used to store a matrix in a Matrix structure. */
void free_matrix(Matrix *matrix){
    free(matrix->values);
    free(matrix);
}

/* Function to read a line of a file, skipping any that are blank or start with a #. */
char *read_line(char *line, Context *context){
    context->line_number++;

    /* Gets a new line from the file as a string, it returns NULL if the line is too long and exits. */
    if (fgets(line, MAX_LINE_LENGTH, context->file) == NULL) {
        exit_invalid_file(context, "");
    }

    /* Gets the first token in the line string. Separates the first string from
     * rest of the line by replacing whitespace with '\0'. */
    context->token = strtok(line, TOKEN_SEPARATORS);
    /* Will get the next line if first token is NULL or a '#'. */
    if (context->token == NULL || context->token[0] == '#'){
        read_line(line, context);
    }
    /* Also sets and returns the first token as the token used in the error context message.*/
    return context->token;
}

/* Function to get the next token in a line of separated strings. */
char *get_new_token(Context *context){
    /* Sets the new token as the error context token. */
    context->token = strtok(NULL, TOKEN_SEPARATORS);
    return context->token;
}

/* Function to turn a string into an int, usually to find the rows and cols of a matrix. */
int get_int(const char *token, Context *context){
    char *end_ptr;
    /* Use strtol to change a string to a long. */
    long value = strtol(token, &end_ptr, 10);

    /* Checks that there are no more characters after the value, using the end_ptr.
     * And that the value is valid. */
    if (*end_ptr != '\0' || value < 1){
        exit_invalid_file(context, "Stated rows or columns are invalid.");
    }
    if (value > MAX_ROWS_COLS){
        exit_invalid_file(context, "Rows or columns of the matrix are bigger than the maximum value allowed.");
    }

    /* Returns the value as an int. */
    return (int) value;
}

/* Function to turn a string into a double, usually for finding an element in a matrix array. */
double get_double(const char *token, Matrix *matrix, Context *context){
    char *end_ptr;
    /* Using strtod to change a string to a double. */
    double value = strtod(token, &end_ptr);

    /* Checks that there are no more characters after the value, using the end_ptr. */
    if (*end_ptr != '\0'){
        free_matrix(matrix);
        exit_invalid_file(context, "Matrix element is invalid.");
    }

    return value;
}

/* Function to find the rows and columns of a matrix from the file. */
void read_rows_cols(int *rows, int *cols, char *token, Context *context){
    /* Checks to make sure the first word of the first relevant line of the file is 'matrix'. */
    if (strcmp(token, "matrix") != 0) {
        exit_invalid_file(context, "");
    }

    token = get_new_token(context);
    *rows = get_int(token, context);

    token = get_new_token(context);
    *cols = get_int(token, context);

    /* Retrieves the next token and checks that its the end of the line. */
    token = get_new_token(context);
    if (token != NULL && *token != '#') {
        exit_invalid_file(context, "There are unexpected characters in the file.");
    }
}

/* Function to create the matrix array with values from a file. */
void read_array(char *line, char *token, Matrix *matrix, Context *context){
    for (int i=0; i<matrix->rows; i++) {
        token = read_line(line, context);

        /* Loops finding matrix elements for as many columns and rows stated in the file. */
        for (int j=0; j<matrix->cols; j++) {
            /* Checks that there is another matrix element when expected. */
            if (token == NULL) {
                free_matrix(matrix);
                exit_invalid_file(context, "Number of stated columns does not match file.");
            }
            /* Checks that the next token isn't the end of the file. */
            if (strcmp(token, "end") == 0){
                free_matrix(matrix);
                exit_invalid_file(context, "Number of stated rows does not match file.");
            }

            matrix->values[i*matrix->cols + j] = get_double(token, matrix, context);
            token = get_new_token(context);

        }
        /* Checks that there are no more strings when not expected, but allows comments. */
        if (token != NULL && *token != '#') {
            free_matrix(matrix);
            exit_invalid_file(context, "Unexpected characters in the file.");
        }
    }
}

/* Function to find the end of a file. */
void read_file_end(char *line, char *token, Matrix *matrix, Context *context){
    token = read_line(line, context);

    /* Checks that the last line in the file contains the word 'end'. */
    if (strcmp(token, "end") != 0) {
        free_matrix(matrix);
        exit_invalid_file(context, "Could not find the end of the file.");
    }
    /* Checks that there are no more strings when not expected, but allows comments. */
    token = get_new_token(context);
    if (token != NULL && *token != '#') {
        free_matrix(matrix);
        exit_invalid_file(context, "Unexpected characters in the file.");
    }
}

/* Function used to call all other functions used to read a matrix from a file. */
Matrix *read_matrix(char *file_name){
    FILE *f;
    char line[MAX_LINE_LENGTH];
    char *token;
    int rows, cols;

    f = fopen(file_name, "r");
    if (f == NULL){
        exit_open_failed(file_name);
    }

    printf("Processing file...\n");

    /* Creates a context structure for error in reading the file. Sets the file and file name. */
    Context file_context;
    file_context.file = f;
    file_context.file_name = file_name;

    token = read_line(line, &file_context);

    read_rows_cols(&rows, &cols, token, &file_context);

    Matrix *matrix = create_matrix(rows, cols);

    read_array(line, token, matrix, &file_context);

    read_file_end(line, token, matrix, &file_context);

    fclose(f);
    return matrix;
}

/* Function to print a matrix to a console, mainly used for testing the program. */
void print_matrix(const Matrix *matrix){
    for (int i=0; i<matrix->rows; i++){
        for (int j=0; j<matrix->cols; j++){
            printf("%.12g\t", matrix->values[i*(matrix->cols) + j]);
        }
        printf("\n");
    }
    printf("\n");
}

/* Function to calculate and return the frobenius norm of a matrix. */
double get_frob_norm(const Matrix *matrix){
    double frob_norm = 0;

    for (int i=0; i<(matrix->rows*matrix->cols); i++){
        /* Quicker way to cycle through the matrix is only using one variable.
         * Works in this case as where the value is does not matter to us. */
        frob_norm += pow(matrix->values[i], 2);
    }

    /* Returns the square root of the summed value of each matrix element squared. */
    return pow(frob_norm, 0.5);
}

/* Function to find the transpose of a matrix. */
Matrix *get_transpose(const Matrix *matrix){
    /* Creates new matrix to return from the function. */
    Matrix *new_mat = create_matrix(matrix->cols, matrix->rows);

    /* Loops to cycle through the matrix and put into correct position for transpose matrix. */
    for (int i=0; i<new_mat->cols; i++){
        for (int j=0; j<new_mat->rows; j++){
            new_mat->values[j*(new_mat->cols)+i] = matrix->values[i*(matrix->cols)+j];
        }
    }

    return new_mat;
}

/* Function to calculate the product of two matrices. */
Matrix *get_product(const Matrix *matrix1, const Matrix *matrix2) {
    double sum = 0;
    Matrix *new_mat = create_matrix(matrix1->rows, matrix2->cols);

    /* Using 3 loops to multiply column of the first matrix by row of second matrix. */
    for (int k = 0; k < new_mat->rows; k++) {
        for (int i = 0; i < new_mat->cols; i++) {
            for (int j = 0; j < matrix1->cols; j++) {
                sum += matrix1->values[k * (matrix1->cols) + j] * matrix2->values[j * (matrix2->cols) + i];
            }
            new_mat->values[k * new_mat->cols + i] = sum;
            /* Sets sum to 0 before calculating the next matrix element of product. */
            sum = 0;
        }
    }

    return new_mat;
}

/* Recursive function to find the determinant of an nxn matrix, where n>2. */
double find_det(const Matrix *matrix){
    /* Returns determinant if matrix is 2x2. */
    if (matrix->rows == 2){
        return matrix->values[0]*matrix->values[3] - matrix->values[1]*matrix->values[2];
    }

    /* Creates sub matrix if incoming matrix is bigger than 2x2. */
    Matrix *sub_mat = create_matrix(matrix->rows-1, matrix->cols-1);
    double det = 0;

    /* Finding the minor for each element in top row. */
    for (int top_row=0; top_row<matrix->cols; top_row++){
        int k=0;
        /* i starts at 1 as we will not make the sub matrix with the top row. */
        for (int i=1; i<matrix->rows; i++){
            for (int j=0; j<matrix->cols; j++){
                /* Check to make sure that any values in column of top row
                 * element being used are not put into sub matrix. */
                if (j != top_row){
                    /* Easier cycling through of a matrix just using one value. */
                    sub_mat->values[k] = matrix->values[i*matrix->cols+j];
                    /* Manually go to next value as it cannot be in a for loop. */
                    k++;
                }
            }
        }
        /* Determinant of each minor is calculated.
         * Recursively calling itself until input matrix is 2x2. */
        det += pow(-1, top_row) * matrix->values[top_row] * find_det(sub_mat);
    }

    free_matrix(sub_mat);
    return det;
}

/* Function to return the determinant of any matrix. Used in other operations too. */
double get_determinant(const Matrix *matrix){
    /* Returns only value in matrix if 1x1 matrix. */
    if (matrix->rows == 1){
        return matrix->values[0];
    }
    /* If not 1x1, uses recursive function. */
    double det = find_det(matrix);

    return det;
}

/* Function to find the cofactor matrix of a matrix. */
Matrix *find_cofactor(const Matrix *matrix){
    Matrix *new_mat = create_matrix(matrix->rows, matrix->cols);

    /* Looping through each element in original matrix, to find that elements cofactor. */
    for (int i=0; i<matrix->rows; i++){
        for (int j=0; j<matrix->cols; j++) {
            /* Creates matrix for minor matrix of each element. */
            Matrix *minor_mat = create_matrix(matrix->rows-1, matrix->cols-1);

            double cofactor = 0;
            int k=0;

            /* Loop to create minor matrix. */
            for (int m=0; m<matrix->rows; m++){
                for (int n=0; n<matrix->cols; n++){

                    if (n != j && m != i){
                        minor_mat->values[k] = matrix->values[m*matrix->cols+n];
                        k++;
                    }

                }
            }
            /* Calculates determinant of minor matrix for origianl matrix
             * element and puts value into cofactor matrix. */
            cofactor = pow(-1, i+j) * get_determinant(minor_mat);
            new_mat->values[i*new_mat->cols+j] = cofactor;

            free_matrix(minor_mat);
        }
    }

    return new_mat;
}

/* Function to find the adjiont of a matrix. */
Matrix *get_adjoint(const Matrix *matrix){
    /* If 1x1 matrix, returns the adjoint as 1x1 matrix with the value 1. */
    if (matrix->rows == 1){
        Matrix *adj_mat = create_matrix(matrix->rows, matrix->cols);
        adj_mat->values[0] = 1;

        return adj_mat;
    }

    /* If bigger than 1x1 matrix, calls functions to find the adjoint. */
    struct matrix *cofact_mat = find_cofactor(matrix);
    struct matrix *adj_mat = get_transpose(cofact_mat);

    free_matrix(cofact_mat);
    return adj_mat;
}

/* Function to find the inverse of a matrix. */
Matrix *get_inverse(const Matrix *matrix){
    /* Checks determinant first to make sure that it is not 0.
     * Inverse cannot be found if the determinant is 0. */
    double det = get_determinant(matrix);
    if (det == 0){
        fprintf(stderr, "The determinant is 0, so the inverse of the matrix could not be found.\n");
        exit(INVALID_MATRIX);
    }

    /* First step is to find adjoint of matrix. */
    struct matrix *adj_mat = get_adjoint(matrix);
    Matrix *inv_mat = create_matrix(matrix->rows, matrix->cols);

    /* Divides each value of adjoint matrix by the determinant to find the inverse matrix. */
    for (int i=0; i<matrix->rows; i++){
        for (int j=0; j<matrix->cols; j++){
            inv_mat->values[i*inv_mat->cols+j] = adj_mat->values[i*adj_mat->cols+j]/det;
        }
    }

    /* Frees matrix used in process of finding the inverse matrix. */
    free_matrix(adj_mat);
    return inv_mat;
}

/* Function to find the last value in an array, used to find the output file from argv. */
int find_output_file(char *argv[]){
    int i;
    /* Loops through each command line argument and finds the last one by looking for '\0'. */
    for (i=0; argv[i] != '\0'; i++)
        ;
    /* Returns int value of last command line argument in argv. */
    return (--i);
}

/* Function to print the matrix elements to the file. */
void file_print_matrix(FILE *f, const Matrix *matrix){
    /* States matrix and its rows and columns, as done in input files. */
    fprintf(f, "matrix %d %d\n", matrix->rows, matrix->cols);
    for (int i=0; i<matrix->rows; i++){
        for (int j=0; j<matrix->cols; j++){
            fprintf(f, "%.12g\t", matrix->values[i*(matrix->cols) + j]);
        }
        fprintf(f, "\n");
    }
}

/* Function to output the new matrix to a file in the same way as the input file is given. */
void output_matrix(const int argc, char *argv[], const char operation, Matrix *matrix){
    /* If no output file given, matrix printed to stdout. */
    FILE *f = stdout;
    char *file_name = "stdout";

    /* Finds value of output file in argv[]. If it is not equal to an input file value
     * for an operation then changes name of file and opens it. */
    int output_file = find_output_file(argv);
    if (operation == 'm' && output_file == MAX_ARGS_m - 1){
        file_name = argv[output_file];
        printf("%s", file_name);
        f = fopen(file_name, "w+");
        if (f == NULL){
            exit_open_failed(file_name);
        }
    }
    else if (operation != 'm' && output_file == MAX_ARGS_t_a_i - 1){
        file_name = argv[output_file];
        f = fopen(file_name, "w+");
        if (f == NULL){
            exit_open_failed(file_name);
        }
    }

    /* Replicating how the input file is given.
     * Prints command line arguments in first line of the file as a comment. */
    fprintf(f, "# ");
    for (int k=0; k<argc; k++) {
        fprintf(f, "%s ", argv[k]);
    }
    fprintf(f, "\n# Version = %s, Revision date = %s\n", VERSION, REV_DATE);
    file_print_matrix(f, matrix);
    fprintf(f, "end\n");

    printf("Output matrix has been printed to file %s.\n\n", file_name);

    fclose(f);
}

/* Function used to store error messages and all functions called when finding the frobenius norm of a matrix. */
void frobenius_norm(char *argv[]){
    struct matrix *a = read_matrix(argv[INPUT_FILE_1]);
    double fn = get_frob_norm(a);

    /* Prints the frobenius norm to 10 significant figures. */
    printf("The frobenius norm of the matrix is %.10g.\n\n", fn);

    free_matrix(a);
}

/* Function used to store error messages and all functions called when finding the transpose of a matrix. */
void transpose(int argc, char *argv[], char operation){
    struct matrix *a = read_matrix(argv[INPUT_FILE_1]);

    struct matrix *c = get_transpose(a);
    output_matrix(argc, argv, operation, c);

    free_matrix(a);
    free_matrix(c);
}

/* Function used to store error messages and all functions called when finding the product of two matrices. */
void product(int argc, char *argv[], char operation){
    struct matrix *a = read_matrix(argv[INPUT_FILE_1]);
    struct matrix *b = read_matrix(argv[INPUT_FILE_2]);

    /* Check that the columns of one matrix match the rows of the other, quits if not. */
    if (a->cols != b->rows && b->cols != a->rows) {
        fprintf(stderr, "It is not possible to find the matrix product of these two matrices.\n");
        free_matrix(a);
        free_matrix(b);
        exit(INVALID_MATRIX);
    }
    /* If columns and rows do match but the input files are the wrong way round,
     * will automatically swap them and fid the product. */
    if (a->cols != b->rows && b->cols == a->rows) {
        printf("\nThe input order of these two matrices was swapped in order to find their product!\n\n.");
        struct matrix *c = get_product(b, a);
        output_matrix(argc, argv, operation, c);

        free_matrix(a);
        free_matrix(b);
        free_matrix(c);
    }
    else {
        struct matrix *c = get_product(a, b);
        output_matrix(argc, argv, operation, c);

        free_matrix(a);
        free_matrix(b);
        free_matrix(c);
    }
}

/* Function used to store error messages and all functions called when finding the determinant of a matrix. */
void determinant(char *argv[]){
    struct matrix *a = read_matrix(argv[INPUT_FILE_1]);

    /* Checks if the matrix is square. */
    if (a->rows != a->cols){
        fprintf(stderr, "This matrix is not square, thus the determinant cannot be found.\n");
        free_matrix(a);
        exit(INVALID_MATRIX);
    }

    double det = get_determinant(a);
    /* Prints the determinant to 10 significant figures. */
    printf("The determinant of the matrix is %.10g.\n\n", det);

    free_matrix(a);
}

/* Function used to store error messages and all functions called when finding the adjoint of a matrix. */
void adjoint(int argc, char *argv[], char operation){
    struct matrix *a = read_matrix(argv[INPUT_FILE_1]);

    /* Checks that the matrix is square. */
    if (a->rows != a->cols){
        fprintf(stderr, "This matrix is not square, thus the adjoint cannot be found.\n");
        free_matrix(a);
        exit(INVALID_MATRIX);
    }

    struct matrix *c = get_adjoint(a);
    output_matrix(argc, argv, operation, c);

    free_matrix(a);
    free_matrix(c);
}

/* Function used to store error messages and all functions called when finding the inverse of a matrix. */
void inverse(int argc, char *argv[], char operation){
    struct matrix *a = read_matrix(argv[INPUT_FILE_1]);

    /* Checks that the matrix is square. */
    if (a->rows != a->cols){
        fprintf(stderr, "This matrix is not square, thus the inverse of the matrix could not be found.\n");
        free_matrix(a);
        exit(INVALID_MATRIX);
    }

    struct matrix *c = get_inverse(a);

    output_matrix(argc, argv, operation, c);

    free_matrix(a);
    free_matrix(c);
}

int main(int argc, char *argv[]) {

    /* Checks on command line arguments to make sure an operation is given and in the right form.
     * Any errors and help function is called in order to help user input arguments correctly. */
    if (argc == 1 || argv[OPERATION_ARGUMENT][0] != '-' || strlen(argv[OPERATION_ARGUMENT]) != OPERATION_INPUT_LENGTH){
        help(argv);
        return INCORRECT_ARGUMENTS;
    }

    char operation = argv[OPERATION_ARGUMENT][1];

    /* Switch statement with operation to call correct function.
     * If incorrect command line arguments for operation, help() will be called.*/
    switch (operation){
        case 'f':
            if (argc != NO_ARGS_f_d){
                help(argv);
                return INCORRECT_ARGUMENTS;
            }
            frobenius_norm(argv);
            break;
        case 't':
            if (argc < MIN_ARGS_t_a_i || argc > MAX_ARGS_t_a_i){
                help(argv);
                return INCORRECT_ARGUMENTS;
            }
            transpose(argc, argv, operation);
            break;
        case 'm':
            if (argc < MIN_ARGS_m || argc > MAX_ARGS_m){
                help(argv);
                return INCORRECT_ARGUMENTS;
            }
            product(argc, argv, operation);
            break;
        case 'd':
            if (argc != NO_ARGS_f_d){
                help(argv);
                return INCORRECT_ARGUMENTS;
            }
            determinant(argv);
            break;
        case 'a':
            if (argc < MIN_ARGS_t_a_i || argc > MAX_ARGS_t_a_i){
                help(argv);
                return INCORRECT_ARGUMENTS;
            }
            adjoint(argc, argv, operation);
            break;
        case 'i':
            if (argc < MIN_ARGS_t_a_i || argc > MAX_ARGS_t_a_i){
                help(argv);
                return INCORRECT_ARGUMENTS;
            }
            inverse(argc, argv, operation);
            break;
        default:
            /* If operation not recognised, help is called for user. */
            help(argv);
            return INCORRECT_ARGUMENTS;
    }

    return NO_ERROR;
}

/*

 Here is an example output from the terminal when using mat_calc.c.

 Firstly when printing to files:


Jems-MacBook-Air:matrix_calc jemgodden$ ./matrix_calc -f matrix5.txt
Processing file...
The frobenius norm of the matrix is 28.24531159.

Jems-MacBook-Air:matrix_calc jemgodden$ ./matrix_calc -t matrix5.txt transpose5.txt
Processing file...
Output matrix has been printed to file transpose5.txt.

Jems-MacBook-Air:matrix_calc jemgodden$ ./matrix_calc -d matrix5.txt
Processing file...
The determinant of the matrix is -6815.551925.

Jems-MacBook-Air:matrix_calc jemgodden$ ./matrix_calc -a matrix5.txt adjoint5.txt
Processing file...
Output matrix has been printed to file adjoint5.txt.

Jems-MacBook-Air:matrix_calc jemgodden$ ./matrix_calc -i matrix5.txt inverse5.txt
Processing file...
Output matrix has been printed to file inverse5.txt.

Jems-MacBook-Air:matrix_calc jemgodden$ ./matrix_calc -m matrix5.txt inverse5.txt identity5.txt
Processing file...
Processing file...
Output matrix has been printed to file identity5.txt.




 And also when printing to stdout:


Jems-MacBook-Air:matrix_calc jemgodden$ ./matrix_calc -f matrix5.txt
Processing file...
The frobenius norm of the matrix is 28.24531159.

Jems-MacBook-Air:matrix_calc jemgodden$ ./matrix_calc -t matrix5.txt
Processing file...
# ./matrix_calc -t matrix5.txt
# Version = 1.0.1, Revision date = 30-Oct-2019
matrix 5 5
2.83370440492	7.28480886542	3.45785923929	7.32063978786	5.91152850814
8.88917318028	1.52160318639	0.74100860429	2.38237182721	5.86496787885
9.4905927449	3.90610974464	2.18731795539	6.42597589941	3.83524731446
6.78923309631	5.39168804669	4.01333075669	0.218148487722	8.14470262646
5.70598280323	9.67236023847	0.839364077355	7.28209677491	3.26078773162
end
Output matrix has been printed to file stdout.

Jems-MacBook-Air:matrix_calc jemgodden$ ./matrix_calc -d matrix5.txt
Processing file...
The determinant of the matrix is -6815.551925.

Jems-MacBook-Air:matrix_calc jemgodden$ ./matrix_calc -a matrix5.txt
Processing file...
# ./matrix_calc -a matrix5.txt
# Version = 1.0.1, Revision date = 30-Oct-2019
matrix 5 5
853.402141095	586.051710299	-62.9771902127	-973.157318575	-1042.23731172
167.63255659	569.973653663	2438.21898098	-404.797263074	-1707.64858972
-819.547379652	331.822600441	-2132.00876013	-233.592517558	1520.30454621
-281.691471906	-571.145698543	-1179.45649552	966.904633518	331.378190844
-181.125095064	-1051.32523536	1182.32741783	351.971559938	254.922154423
end
Output matrix has been printed to file stdout.

Jems-MacBook-Air:matrix_calc jemgodden$ ./matrix_calc -i matrix5.txt
Processing file...
# ./matrix_calc -i matrix5.txt
# Version = 1.0.1, Revision date = 30-Oct-2019
matrix 5 5
-0.1252139446	-0.0859874177159	0.00924021868075	0.142784814693	0.152920456517
-0.0245955952571	-0.0836283928249	-0.357743438533	0.0593931742514	0.250551768753
0.12024666362	-0.0486860938212	0.312815276552	0.0342734557882	-0.223064039867
0.0413306911935	0.083800359071	0.173053702559	-0.141867400355	-0.0486208885962
0.0265752644935	0.154253866305	-0.173474933641	-0.0516424148529	-0.0374030096528
end
Output matrix has been printed to file stdout.

Jems-MacBook-Air:matrix_calc jemgodden$ ./matrix_calc -m matrix5.txt inverse5.txt
Processing file...
Processing file...
# ./matrix_calc -m matrix5.txt inverse5.txt
# Version = 1.0.1, Revision date = 30-Oct-2019
matrix 5 5
1	2.99982261254e-13	-6.78923584019e-12	-9.20541420868e-13	-1.59514068621e-12
-9.38027433506e-13	1	-3.58046925442e-12	-3.59601237676e-13	-2.35350627875e-12
-2.52235732301e-13	1.26648691534e-13	0.999999999999	-8.54455395327e-14	-1.36555350361e-12
-1.87988513645e-13	2.44693154627e-13	-3.91375820641e-12	1	-3.06937808503e-12
-2.0143609003e-13	8.18234369149e-14	-3.82449627523e-12	-4.71761518739e-13	0.999999999999
end
Output matrix has been printed to file stdout.

*/
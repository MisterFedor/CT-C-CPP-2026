#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EPS 1e-6f
#define INIT_CAPACITY 16
#define MARKETMATRIX_SIZE 15
#define OBJECT_SIZE 7
#define GENERAL_SIZE 8
#define FORMAT_SIZE 11
#define FIELD_SIZE 8
#define CAPACITY_EXTEND 2
#define IS_ZERO_FLOAT(x) (fabsf(x) < EPS)
#define SWAP(type, first, second)                                                                                      \
	{                                                                                                                  \
		type temp = (first);                                                                                           \
		(first) = (second);                                                                                            \
		(second) = temp;                                                                                               \
	}

typedef struct
{
	size_t rows_amount, col_amount, nnz, capacity;
	size_t *rows, *col;
	float *values;
} SparseMatrix;

enum operations
{
	ADD,
	SUBTRACT,
	MULTIPLY,
	INVOLUTION,
	DETERMINANT,
	ABS
};

enum elements_position
{
	ELEMS_COORDINATES_EQUALS,
	FIRST_ELEM_BEFORE,
	SECOND_ELEM_BEFORE
};

void SparseMatrix_init(SparseMatrix *mtx)
{
	mtx->rows_amount = 0, mtx->col_amount = 0, mtx->nnz = 0, mtx->capacity = 0;
	mtx->rows = NULL, mtx->col = NULL, mtx->values = NULL;
}

void free_matrix_coordinates(size_t **rows, size_t **col, float **values)
{
	free(*rows);
	free(*col);
	free(*values);
	*rows = NULL, *col = NULL, *values = NULL;
}

void free_SparseMatrix_coordinates(SparseMatrix *mtx)
{
	free(mtx->rows);
	free(mtx->col);
	free(mtx->values);
	mtx->rows_amount = 0, mtx->col_amount = 0, mtx->nnz = 0, mtx->capacity = 0;
	mtx->rows = NULL, mtx->col = NULL, mtx->values = NULL;
}

bool ensure_capacity(SparseMatrix *mtx, size_t needed)
{
	if (needed <= mtx->capacity)
	{
		return true;
	}

	size_t ext_capacity = mtx->capacity;
	if (ext_capacity == 0)
	{
		ext_capacity = INIT_CAPACITY;
	}
	while (ext_capacity < needed)
	{
		ext_capacity *= CAPACITY_EXTEND;
	}
	size_t *new_rows = malloc(ext_capacity * sizeof(size_t));
	size_t *new_col = malloc(ext_capacity * sizeof(size_t));
	float *new_values = malloc(ext_capacity * sizeof(float));
	if (new_rows == NULL || new_col == NULL || new_values == NULL)
	{
		free_matrix_coordinates(&new_rows, &new_col, &new_values);
		fprintf(stderr, "Problems with memory.\n");
		return false;
	}
	if (mtx->nnz > 0)
	{
		memcpy(new_rows, mtx->rows, mtx->nnz * sizeof(size_t));
		memcpy(new_col, mtx->col, mtx->nnz * sizeof(size_t));
		memcpy(new_values, mtx->values, mtx->nnz * sizeof(float));
		free_matrix_coordinates(&mtx->rows, &mtx->col, &mtx->values);
	}
	mtx->rows = new_rows;
	mtx->col = new_col;
	mtx->values = new_values;
	mtx->capacity = ext_capacity;
	return true;
}

bool copy_matrix(SparseMatrix *mtx1, SparseMatrix *mtx2)
{
	SparseMatrix_init(mtx2);
	mtx2->nnz = mtx1->nnz;
	mtx2->capacity = mtx1->nnz;
	mtx2->col_amount = mtx1->col_amount;
	mtx2->rows_amount = mtx1->rows_amount;
	if (mtx1->nnz == 0)
	{
		return true;
	}
	mtx2->rows = malloc(mtx1->nnz * sizeof(size_t));
	mtx2->col = malloc(mtx1->nnz * sizeof(size_t));
	mtx2->values = malloc(mtx1->nnz * sizeof(float));
	if (mtx2->rows == NULL || mtx2->col == NULL || mtx2->values == NULL)
	{
		free_SparseMatrix_coordinates(mtx2);
		fprintf(stderr, "Problems with memory.\n");
		return false;
	}
	memcpy(mtx2->rows, mtx1->rows, mtx1->nnz * sizeof(size_t));
	memcpy(mtx2->col, mtx1->col, mtx1->nnz * sizeof(size_t));
	memcpy(mtx2->values, mtx1->values, mtx1->nnz * sizeof(float));
	return true;
}

bool make_matrix_E(size_t dim, SparseMatrix *mtx)
{
	SparseMatrix_init(mtx);
	mtx->nnz = dim, mtx->capacity = dim, mtx->col_amount = dim, mtx->rows_amount = dim;
	if (dim == 0)
	{
		return true;
	}
	mtx->rows = malloc(dim * sizeof(size_t));
	mtx->col = malloc(dim * sizeof(size_t));
	mtx->values = malloc(dim * sizeof(float));
	if (mtx->rows == NULL || mtx->col == NULL || mtx->values == NULL)
	{
		free_SparseMatrix_coordinates(mtx);
		fprintf(stderr, "Problems with memory.\n");
		return false;
	}
	for (size_t i = 0; i < dim; i++)
	{
		mtx->rows[i] = i;
		mtx->col[i] = i;
		mtx->values[i] = 1.0f;
	}
	return true;
}

void to_lowercase_str(char *string)
{
	for (size_t i = 0; string[i] != '\0'; i++)
	{
		unsigned char symbol = string[i];
		string[i] = tolower(symbol);
	}
}

bool checking_all_formats(char MatrixMarket[], char object[], char format[], char field[], char general[])
{
	to_lowercase_str(object), to_lowercase_str(format), to_lowercase_str(field), to_lowercase_str(general);
	if (strcmp(MatrixMarket, "%%MatrixMarket") == 0 && strcmp(object, "matrix") == 0 &&
		(strcmp(format, "array") == 0 || strcmp(format, "coordinate") == 0) &&
		(strcmp(field, "real") == 0 || strcmp(field, "integer") == 0) && strcmp(general, "general") == 0)
	{
		return true;
	}
	return false;
}

bool read_header(FILE *input_file, char format[], char field[])
{
	char MatrixMarket[MARKETMATRIX_SIZE], object[OBJECT_SIZE], general[GENERAL_SIZE];
	if (fscanf(input_file, "%14s %6s %10s %7s %7s", MatrixMarket, object, format, field, general) != 5)
	{
		fprintf(stderr, "Incorrect input. Need: %%%%MatrixMarket matrix <array | coordinate> <real | integer> general \n");
		return false;
	}
	if (!checking_all_formats(MatrixMarket, object, format, field, general))
	{
		fprintf(stderr, "Incorrect Matrix Market exchange format header.\n");
		fprintf(stderr, "Need: %%%%MatrixMarket matrix <array | coordinate> <real | integer> general \n");
		return false;
	}
	int symbol;
	while (true)
	{
		symbol = fgetc(input_file);
		if (symbol == '\n' || symbol == EOF)
		{
			break;
		}
		if (!isspace(symbol))
		{
			fprintf(stderr, "Incorrect Matrix Market exchange format header.\n");
			fprintf(stderr, "Need only: %%%%MatrixMarket matrix <array | coordinate> <real | integer> general \n");
			return false;
		}
	}
	return true;
}

void skip_comments(FILE *input_file)
{
	int comment_symbol = fgetc(input_file);
	while (comment_symbol != EOF)
	{
		if (comment_symbol == '\n')
		{
			comment_symbol = fgetc(input_file);
		}
		else if (comment_symbol == '%')
		{
			while (comment_symbol != EOF && comment_symbol != '\n')
			{
				comment_symbol = fgetc(input_file);
			}
			comment_symbol = fgetc(input_file);
		}
		else
		{
			ungetc(comment_symbol, input_file);
			return;
		}
	}
}

bool parse_matrix(FILE *input_file, SparseMatrix *mtx)
{
	SparseMatrix_init(mtx);
	char format[FORMAT_SIZE], field[FIELD_SIZE];

	if (!read_header(input_file, format, field))
	{
		return false;
	}
	skip_comments(input_file);
	if (strcmp(format, "coordinate") == 0)
	{
		if (fscanf(input_file, "%zu %zu %zu", &mtx->rows_amount, &mtx->col_amount, &mtx->nnz) != 3)
		{
			fprintf(stderr, "First line must be: [row amount] [col amount] [nnz]\n");
			goto lCleanup_parsing_error;
		}
		mtx->capacity = mtx->nnz;
		if (mtx->rows_amount == 0 || mtx->col_amount == 0)
		{
			fprintf(stderr, "Arguments must be > 0.\n");
			goto lCleanup_parsing_error;
		}
		if (mtx->nnz == 0)
		{
			return true;
		}
		mtx->rows = malloc(mtx->nnz * sizeof(size_t));
		mtx->col = malloc(mtx->nnz * sizeof(size_t));
		mtx->values = malloc(mtx->nnz * sizeof(float));
		if (mtx->rows == NULL || mtx->col == NULL || mtx->values == NULL)
		{
			fprintf(stderr, "Problems with memory allocation.\n");
			goto lCleanup_parsing_error;
		}
		for (size_t i = 0; i < mtx->nnz; i++)
		{
			if (fscanf(input_file, "%zu %zu %f", &mtx->rows[i], &mtx->col[i], &mtx->values[i]) != 3)
			{
				fprintf(stderr, "Every line with coordinates must be: [row] [col] [value]\n");
				goto lCleanup_parsing_error;
			}
			if (mtx->rows[i] == 0 || mtx->col[i] == 0 || mtx->rows[i] > mtx->rows_amount || mtx->col[i] > mtx->col_amount)
			{
				fprintf(stderr, "Arguments must be > 0 and <= rows amount[columns amount].\n");
				goto lCleanup_parsing_error;
			}
			mtx->rows[i]--;
			mtx->col[i]--;
		}
	}
	if (strcmp(format, "array") == 0)
	{
		if (fscanf(input_file, "%zu %zu", &mtx->rows_amount, &mtx->col_amount) != 2)
		{
			fprintf(stderr, "First line must be: [row amount] [col amount]\n");
			goto lCleanup_parsing_error;
		}
		if (mtx->rows_amount == 0 || mtx->col_amount == 0)
		{
			fprintf(stderr, "Arguments must be > 0.\n");
			goto lCleanup_parsing_error;
		}

		for (size_t c = 0; c < mtx->col_amount; c++)
		{
			for (size_t r = 0; r < mtx->rows_amount; r++)
			{
				float x;
				if (fscanf(input_file, "%f", &x) != 1)
				{
					fprintf(stderr, "Every line must be: [value]\n");
					goto lCleanup_parsing_error;
				}

				if (IS_ZERO_FLOAT(x))
				{
					continue;
				}

				if (!ensure_capacity(mtx, mtx->nnz + 1))
				{
					goto lCleanup_parsing_error;
				}
				mtx->rows[mtx->nnz] = r;
				mtx->col[mtx->nnz] = c;
				mtx->values[mtx->nnz] = x;
				mtx->nnz++;
			}
		}
	}
	return true;

lCleanup_parsing_error:
	free_SparseMatrix_coordinates(mtx);
	fprintf(stderr, "Some problems with parsing matrices.\n");
	return false;
}

bool read_input_file(const char input_filename[], SparseMatrix *mtx)
{
	SparseMatrix_init(mtx);
	FILE *input_file = fopen(input_filename, "r");
	if (input_file == NULL)
	{
		fprintf(stderr, "Can`t read input file: %s. \n", input_filename);
		return false;
	}
	if (!parse_matrix(input_file, mtx))
	{
		fclose(input_file);
		fprintf(stderr, "Check your input file: %s.\n", input_filename);
		return false;
	}
	fclose(input_file);
	return true;
}

bool read_output_file(const char output_filename[], FILE **output_file)
{
	*output_file = fopen(output_filename, "w");
	if (*output_file == NULL)
	{
		fprintf(stderr, "Can`t open output file: %s. \n", output_filename);
		return false;
	}
	return true;
}

bool read_degree(const char input_filename[], size_t *deg)
{
	FILE *input_file = fopen(input_filename, "r");
	if (input_file == NULL)
	{
		fprintf(stderr, "Can`t read input file: %s. \n", input_filename);
		return false;
	}
	if (fscanf(input_file, "%zu", deg) != 1)
	{
		fprintf(stderr, "Input2 file: %s must include 1 number\n", input_filename);
		fclose(input_file);
		return false;
	}
	fclose(input_file);
	return true;
}

bool write_answer_to_file(FILE *output_file, SparseMatrix *mtx)
{
	if (fprintf(output_file, "%%%%MatrixMarket matrix coordinate real general\n") < 0 ||
		fprintf(output_file, "%zu %zu %zu\n", mtx->rows_amount, mtx->col_amount, mtx->nnz) < 0)
	{
		return false;
	}
	for (size_t i = 0; i < mtx->nnz; i++)
	{
		fprintf(output_file, "%zu %zu %g\n", mtx->rows[i] + 1, mtx->col[i] + 1, mtx->values[i]);
	}
	return true;
}

void sort_matrix(size_t *rows, size_t *col, float *values, size_t nnz)
{
	if (nnz <= 1)
	{
		return;
	}
	for (size_t i = 0; i < nnz - 1; i++)
	{
		for (size_t j = i + 1; j < nnz; j++)
		{
			if ((rows[i] > rows[j]) || (rows[i] == rows[j] && col[i] > col[j]))
			{
				SWAP(size_t, rows[i], rows[j]);
				SWAP(size_t, col[i], col[j]);
				SWAP(float, values[i], values[j]);
			}
		}
	}
}

enum elements_position element_pos(size_t row1, size_t row2, size_t col1, size_t col2)
{
	if (row1 == row2 && col1 == col2)
	{
		return ELEMS_COORDINATES_EQUALS;
	}
	if (row1 < row2 || (row1 == row2 && col1 < col2))
	{
		return FIRST_ELEM_BEFORE;
	}
	return SECOND_ELEM_BEFORE;
}

bool elements_add(SparseMatrix *result_mtx, size_t row, size_t col, float value)
{
	if (IS_ZERO_FLOAT(value))
	{
		return true;
	}
	if (!ensure_capacity(result_mtx, result_mtx->nnz + 1))
	{
		return false;
	}
	result_mtx->rows[result_mtx->nnz] = row;
	result_mtx->col[result_mtx->nnz] = col;
	result_mtx->values[result_mtx->nnz] = value;
	result_mtx->nnz++;
	return true;
}

bool add_rest_elements(int sign, SparseMatrix *mtx1, size_t *pos, SparseMatrix *mtx2)
{
	while (*pos < mtx1->nnz)
	{
		if (!elements_add(mtx2, mtx1->rows[*pos], mtx1->col[*pos], sign * mtx1->values[*pos]))
		{
			return false;
		}
		(*pos)++;
	}
	return true;
}

bool elements_add_multiply(SparseMatrix *res_mtx, size_t row, size_t col, float value)
{
	if (IS_ZERO_FLOAT(value))
	{
		return true;
	}
	for (size_t i = 0; i < res_mtx->nnz; i++)
	{
		if (res_mtx->rows[i] == row && res_mtx->col[i] == col)
		{
			res_mtx->values[i] += value;
			if (IS_ZERO_FLOAT(res_mtx->values[i]))
			{
				for (size_t j = i + 1; j < res_mtx->nnz; j++)
				{
					res_mtx->rows[j - 1] = res_mtx->rows[j];
					res_mtx->col[j - 1] = res_mtx->col[j];
					res_mtx->values[j - 1] = res_mtx->values[j];
				}
				res_mtx->nnz--;
			}
			return true;
		}
	}
	return elements_add(res_mtx, row, col, value);
}

float get_value_matrix(SparseMatrix *mtx, size_t need_row, size_t need_col)
{
	float matrix_value = 0.0f;
	for (size_t i = 0; i < mtx->nnz; i++)
	{
		if (mtx->rows[i] == need_row && mtx->col[i] == need_col)
		{
			matrix_value += mtx->values[i];
		}
	}
	return matrix_value;
}

bool matrix_minor(SparseMatrix *mtx, size_t pass_row, size_t pass_col, SparseMatrix *minor)
{
	SparseMatrix_init(minor);
	minor->rows_amount = mtx->rows_amount - 1;
	minor->col_amount = mtx->col_amount - 1;
	minor->nnz = 0;
	for (size_t i = 0; i < mtx->nnz; i++)
	{
		if (mtx->rows[i] == pass_row || mtx->col[i] == pass_col)
		{
			continue;
		}
		if (IS_ZERO_FLOAT(mtx->values[i]))
		{
			continue;
		}

		size_t new_row = mtx->rows[i];
		size_t new_col = mtx->col[i];
		if (new_row > pass_row)
		{
			new_row--;
		}
		if (new_col > pass_col)
		{
			new_col--;
		}
		if (!elements_add(minor, new_row, new_col, mtx->values[i]))
		{
			free_SparseMatrix_coordinates(minor);
			return false;
		}
	}
	return true;
}

bool zero_row_col_check(SparseMatrix *mtx)
{
	for (size_t row = 0; row < mtx->rows_amount; row++)
	{
		bool non_zero_element = false;
		for (size_t i = 0; i < mtx->nnz; i++)
		{
			if (mtx->rows[i] == row && !IS_ZERO_FLOAT(mtx->values[i]))
			{
				non_zero_element = true;
				break;
			}
		}
		if (!non_zero_element)
		{
			return true;
		}
	}

	for (size_t col = 0; col < mtx->col_amount; col++)
	{
		bool non_zero_element = false;
		for (size_t i = 0; i < mtx->nnz; i++)
		{
			if (mtx->col[i] == col && !IS_ZERO_FLOAT(mtx->values[i]))
			{
				non_zero_element = true;
				break;
			}
		}
		if (!non_zero_element)
		{
			return true;
		}
	}
	return false;
}

bool matrix_add_subtract(SparseMatrix *mtx1, SparseMatrix *mtx2, int operation_sign, SparseMatrix *res_mtx)
{
	SparseMatrix_init(res_mtx);
	sort_matrix(mtx1->rows, mtx1->col, mtx1->values, mtx1->nnz);
	sort_matrix(mtx2->rows, mtx2->col, mtx2->values, mtx2->nnz);
	res_mtx->rows_amount = mtx1->rows_amount;
	res_mtx->col_amount = mtx1->col_amount;
	size_t first = 0, second = 0;
	while (first < mtx1->nnz && second < mtx2->nnz)
	{
		switch (element_pos(mtx1->rows[first], mtx2->rows[second], mtx1->col[first], mtx2->col[second]))
		{
		case ELEMS_COORDINATES_EQUALS:
		{
			float sum = mtx1->values[first] + operation_sign * mtx2->values[second];
			if (!IS_ZERO_FLOAT(sum) && !elements_add(res_mtx, mtx1->rows[first], mtx1->col[first], sum))
			{
				goto lCleanup_error;
			}
			first++, second++;
			break;
		}
		case FIRST_ELEM_BEFORE:
		{
			if (!elements_add(res_mtx, mtx1->rows[first], mtx1->col[first], mtx1->values[first]))
			{
				goto lCleanup_error;
			}
			first++;
			break;
		}
		case SECOND_ELEM_BEFORE:
		{
			if (!elements_add(res_mtx, mtx2->rows[second], mtx2->col[second], operation_sign * mtx2->values[second]))
			{
				goto lCleanup_error;
			}
			second++;
			break;
		}
		}
	}
	if (!add_rest_elements(1, mtx1, &first, res_mtx))
	{
		goto lCleanup_error;
	}
	if (!add_rest_elements(operation_sign, mtx2, &second, res_mtx))
	{
		goto lCleanup_error;
	}
	return true;
lCleanup_error:
	free_SparseMatrix_coordinates(res_mtx);
	return false;
}

bool matrix_multiply(SparseMatrix *mtx1, SparseMatrix *mtx2, SparseMatrix *res_mtx)
{
	SparseMatrix_init(res_mtx);
	res_mtx->rows_amount = mtx1->rows_amount;
	res_mtx->col_amount = mtx2->col_amount;
	for (size_t first = 0; first < mtx1->nnz; first++)
	{
		for (size_t second = 0; second < mtx2->nnz; second++)
		{
			if (mtx1->col[first] != mtx2->rows[second])
			{
				continue;
			}
			float product = mtx1->values[first] * mtx2->values[second];
			if (!elements_add_multiply(res_mtx, mtx1->rows[first], mtx2->col[second], product))
			{
				goto lCleanup_multiply_error;
			}
		}
	}

	return true;
lCleanup_multiply_error:
	free_SparseMatrix_coordinates(res_mtx);
	return false;
}

bool matrix_involution(SparseMatrix *mtx, size_t deg, SparseMatrix *res_mtx)
{
	SparseMatrix_init(res_mtx);
	if (deg == 0)
	{
		return make_matrix_E(mtx->rows_amount, res_mtx);
	}
	if (deg == 1)
	{
		return copy_matrix(mtx, res_mtx);
	}
	SparseMatrix half_mtx, temp_mtx;
	SparseMatrix_init(&half_mtx), SparseMatrix_init(&temp_mtx);
	if (!matrix_involution(mtx, deg / 2, &half_mtx))
	{
		goto lCleanup_involution_error;
	}
	if (!matrix_multiply(&half_mtx, &half_mtx, &temp_mtx))
	{
		goto lCleanup_involution_error;
	}
	free_SparseMatrix_coordinates(&half_mtx);

	if (deg % 2 == 0)
	{
		*res_mtx = temp_mtx;
		SparseMatrix_init(&temp_mtx);
		return true;
	}
	else if (!matrix_multiply(mtx, &temp_mtx, res_mtx))
	{
		goto lCleanup_involution_error;
	}
	free_SparseMatrix_coordinates(&temp_mtx);
	return true;
lCleanup_involution_error:
	free_SparseMatrix_coordinates(&temp_mtx);
	free_SparseMatrix_coordinates(&half_mtx);
	free_SparseMatrix_coordinates(res_mtx);
	return false;
}

bool matrix_determinant(SparseMatrix *mtx, float *det)
{
	if (mtx->col_amount != mtx->rows_amount)
	{
		return false;
	}
	size_t dim = mtx->col_amount;
	if (dim == 1)
	{
		*det = get_value_matrix(mtx, 0, 0);
		return true;
	}
	if (dim == 2)
	{
		float first = get_value_matrix(mtx, 0, 0);
		float second = get_value_matrix(mtx, 0, 1);
		float third = get_value_matrix(mtx, 1, 0);
		float fourth = get_value_matrix(mtx, 1, 1);
		*det = first * fourth - second * third;
		return true;
	}
	if (zero_row_col_check(mtx))
	{
		*det = 0.0f;
		return true;
	}
	float determinant_res = 0.0f;
	for (size_t cur_col = 0; cur_col < dim; cur_col++)
	{
		float matrix_elem = get_value_matrix(mtx, 0, cur_col);
		if (IS_ZERO_FLOAT(matrix_elem))
		{
			continue;
		}
		SparseMatrix minor;
		SparseMatrix_init(&minor);

		if (!matrix_minor(mtx, 0, cur_col, &minor))
		{
			free_SparseMatrix_coordinates(&minor);
			return false;
		}
		float minor_det = 0.0f;
		if (!matrix_determinant(&minor, &minor_det))
		{
			free_SparseMatrix_coordinates(&minor);
			return false;
		}
		free_SparseMatrix_coordinates(&minor);
		if (cur_col % 2 == 0)
		{
			determinant_res = determinant_res + (matrix_elem * minor_det);
		}
		else
		{
			determinant_res = determinant_res - (matrix_elem * minor_det);
		}
	}
	*det = determinant_res;
	return true;
}

bool before_binary_operation(int argc, char *argv[], const char output_filename[], SparseMatrix *mtx, FILE **output_file)
{
	if (argc != 5)
	{
		fprintf(stderr, "For binary operations you need second input file.\n");
		fprintf(stderr, "Usage: <operation> <output_filename> <input1_filename> [<input2_filename>]\n");
		return false;
	}
	const char *input2_filename = argv[4];
	if (!read_input_file(input2_filename, mtx))
	{
		return false;
	}
	if (!read_output_file(output_filename, output_file))
	{
		free_SparseMatrix_coordinates(mtx);
		return false;
	}
	return true;
}
bool add_subtract_operation(int sign, int argc, char *argv[], const char output_filename[], SparseMatrix *mtx1, SparseMatrix *mtx2, SparseMatrix *res_mtx, FILE **output_file)
{
	if (!before_binary_operation(argc, argv, output_filename, mtx2, output_file))
	{
		fprintf(stderr, "Problems with add[subtract].\n");
		return false;
	}
	if ((mtx1->rows_amount != mtx2->rows_amount) || (mtx1->col_amount != mtx2->col_amount))
	{
		fprintf(*output_file, "no solution\n");
		return true;
	}
	if (!matrix_add_subtract(mtx1, mtx2, sign, res_mtx))
	{
		fprintf(stderr, "Problems with add[subtract].\n");
		return false;
	}
	if (!write_answer_to_file(*output_file, res_mtx))
	{
		fprintf(stderr, "Can`t write output file: %s. \n", output_filename);
		return false;
	}
	return true;
}
bool multiply_operation(int argc, char *argv[], const char output_filename[], SparseMatrix *mtx1, SparseMatrix *mtx2, SparseMatrix *res_mtx, FILE **output_file)
{
	if (!before_binary_operation(argc, argv, output_filename, mtx2, output_file))
	{
		fprintf(stderr, "Problems with multiply.\n");
		return false;
	}
	if (mtx1->col_amount != mtx2->rows_amount)
	{
		fprintf(*output_file, "no solution\n");
		return true;
	}
	if (!matrix_multiply(mtx1, mtx2, res_mtx))
	{
		fprintf(stderr, "Problems with multiply.\n");
		return false;
	}
	if (!write_answer_to_file(*output_file, res_mtx))
	{
		fprintf(stderr, "Can`t write output file: %s. \n", output_filename);
		return false;
	}
	return true;
}
bool matrix_abs(SparseMatrix *mtx, SparseMatrix *res_mtx)
{
	if (!copy_matrix(mtx, res_mtx))
	{
		return false;
	}
	for (size_t i = 0; i < res_mtx->nnz; i++)
	{
		res_mtx->values[i] = fabsf(res_mtx->values[i]);
	}
	return true;
}

bool abs_operation(const char output_filename[], SparseMatrix *mtx, SparseMatrix *res_mtx, FILE **output_file)
{
	if (!read_output_file(output_filename, output_file))
	{
		fprintf(stderr, "Problems with output file.\n");
		return false;
	}
	if (!matrix_abs(mtx, res_mtx))
	{
		fprintf(stderr, "Problems in abs. \n");
		return false;
	}
	if (!write_answer_to_file(*output_file, res_mtx))
	{
		fprintf(stderr, "Can`t write output file: %s. \n", output_filename);
		return false;
	}
	return true;
}
bool involution_operation(int argc, char *argv[], const char output_filename[], SparseMatrix *mtx, SparseMatrix *res_mtx, FILE **output_file)
{
	if (argc != 5)
	{
		fprintf(stderr, "For involution you need file with degre.\n");
		fprintf(stderr, "Usage: <operation> <output_filename> <input1_filename> [<input2_filename>]\n");
		return false;
	}
	const char *input2_filename = argv[4];
	size_t deg;
	if (!read_degree(input2_filename, &deg))
	{
		fprintf(stderr, "Problems with input file for involution.\n");
		return false;
	}
	if (!read_output_file(output_filename, output_file))
	{
		fprintf(stderr, "Problems with output file.\n");
		return false;
	}
	if (deg != 1 && mtx->rows_amount != mtx->col_amount)
	{
		fprintf(*output_file, "no solution\n");
		return true;
	}
	if (!matrix_involution(mtx, deg, res_mtx))
	{
		fprintf(stderr, "Problems with involution.\n");
		return false;
	}
	if (!write_answer_to_file(*output_file, res_mtx))
	{
		fprintf(stderr, "Can`t write output file: %s. \n", output_filename);
		return false;
	}
	return true;
}
bool determinant_operation(const char output_filename[], SparseMatrix *mtx, FILE **output_file)
{
	if (!read_output_file(output_filename, output_file))
	{
		fprintf(stderr, "Problems with output file.\n");
		return false;
	}
	if (mtx->rows_amount != mtx->col_amount)
	{
		fprintf(*output_file, "no solution\n");
		return true;
	}
	float det = 0.0f;
	if (!matrix_determinant(mtx, &det))
	{
		fprintf(stderr, "Problems with determinant.\n");
		return false;
	}
	if (fprintf(*output_file, "%g\n", det) < 0)
	{
		fprintf(stderr, "Can`t write output file: %s. \n", output_filename);
		return false;
	}
	return true;
}

int main(int argc, char *argv[])
{
	if (argc != 4 && argc != 5)
	{
		fprintf(stderr, "Incorrect input. Amount of arguments != 4 and != 5.\n");
		fprintf(stderr, "Usage: <operation> <output_filename> <input1_filename> [<input2_filename>]\n");
		return 1;
	}
	enum operations operation;
	if (argv[1][1] != '\0')
	{
		fprintf(stderr, "Incorrect operation input.\n");
		return 1;
	}
	switch (argv[1][0])
	{
	case '+':
		operation = ADD;
		break;
	case '-':
		operation = SUBTRACT;
		break;
	case '*':
		operation = MULTIPLY;
		break;
	case '^':
		operation = INVOLUTION;
		break;
	case '|':
		operation = DETERMINANT;
		break;
	case '$':
		operation = ABS;
		break;
	default:
		fprintf(stderr, "Incorrect operation. First argument (operation) must be +, -, *, ^ or |.\n");
		return 1;
	}

	const char *output_filename = argv[2];
	const char *input1_filename = argv[3];

	SparseMatrix mtx1, mtx2, res_mtx;
	SparseMatrix_init(&mtx1), SparseMatrix_init(&mtx2), SparseMatrix_init(&res_mtx);

	FILE *output_file = NULL;
	bool operation_result = false;
	if (!read_input_file(input1_filename, &mtx1))
	{
		goto lCleanup;
	}
	switch (operation)
	{
	case ADD:
	{
		operation_result = add_subtract_operation(1, argc, argv, output_filename, &mtx1, &mtx2, &res_mtx, &output_file);
		break;
	}
	case SUBTRACT:
	{
		operation_result = add_subtract_operation(-1, argc, argv, output_filename, &mtx1, &mtx2, &res_mtx, &output_file);
		break;
	}
	case MULTIPLY:
	{
		operation_result = multiply_operation(argc, argv, output_filename, &mtx1, &mtx2, &res_mtx, &output_file);
		break;
	}
	case INVOLUTION:
	{
		operation_result = involution_operation(argc, argv, output_filename, &mtx1, &res_mtx, &output_file);
		break;
	}
	case DETERMINANT:
	{
		operation_result = determinant_operation(output_filename, &mtx1, &output_file);
		break;
	}
	case ABS:
	{
		operation_result = abs_operation(output_filename, &mtx1, &res_mtx, &output_file);
		break;
	}
	}
lCleanup:
	if (output_file != NULL)
	{
		fclose(output_file);
	}
	free_SparseMatrix_coordinates(&mtx1);
	free_SparseMatrix_coordinates(&mtx2);
	free_SparseMatrix_coordinates(&res_mtx);
	return operation_result ? 0 : 1;
}

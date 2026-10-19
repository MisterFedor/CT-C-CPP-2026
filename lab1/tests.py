import os
import random
import subprocess


BASE_DIR = os.path.dirname(os.path.abspath(__file__))

EXECUTABLE = "a.exe"
TESTS_DIR_NAME = "tests"
OUT_FILE_NAME = "out.txt"

TESTS_DIR = os.path.join(BASE_DIR, TESTS_DIR_NAME)
OUT_FILE = os.path.join(BASE_DIR, OUT_FILE_NAME)

TEST_COUNT_PER_OPERATOR = 50

MIN_SIZE = 5
MAX_SIZE = 8
MIN_VALUE = 0
MAX_VALUE = 5

OPERATORS = ["|", "+", "-", "*", "^"]
# OPERATORS = ["^"]


def normalize_coordinate(text):
    lines = []

    for line in text.splitlines():
        stripped = line.strip()

        if not stripped:
            continue

        if stripped.startswith("%") and not stripped.startswith("%%MatrixMarket"):
            continue

        lines.append(stripped)

    if len(lines) < 2:
        raise ValueError("Некорректный MatrixMarket coordinate")

    header = lines[0]
    size = lines[1]

    entries = []

    for line in lines[2:]:
        i, j, v = line.split()
        entries.append((int(i), int(j), int(v)))

    entries.sort()

    return header, size, entries


def ensure_tests_dir():
    if not os.path.exists(TESTS_DIR):
        os.mkdir(TESTS_DIR)


def generate_matrix(rows, cols):
    matrix = []

    for _ in range(rows):
        row = []

        for _ in range(cols):
            row.append(random.randint(MIN_VALUE, MAX_VALUE))

        matrix.append(row)

    return matrix


def identity_matrix(n):
    result = []

    for i in range(n):
        row = []

        for j in range(n):
            row.append(1 if i == j else 0)

        result.append(row)

    return result


def determinant(matrix):
    n = len(matrix)

    if n == 1:
        return matrix[0][0]

    if n == 2:
        return matrix[0][0] * matrix[1][1] - matrix[0][1] * matrix[1][0]

    det = 0

    for col in range(n):
        minor = []

        for i in range(1, n):
            row = []

            for j in range(n):
                if j != col:
                    row.append(matrix[i][j])

            minor.append(row)

        sign = -1 if col % 2 else 1
        det += sign * matrix[0][col] * determinant(minor)

    return det


def add_matrices(a, b):
    rows = len(a)
    cols = len(a[0])
    result = []

    for i in range(rows):
        row = []

        for j in range(cols):
            row.append(a[i][j] + b[i][j])

        result.append(row)

    return result


def sub_matrices(a, b):
    rows = len(a)
    cols = len(a[0])
    result = []

    for i in range(rows):
        row = []

        for j in range(cols):
            row.append(a[i][j] - b[i][j])

        result.append(row)

    return result


def mul_matrices(a, b):
    rows_a = len(a)
    cols_a = len(a[0])
    cols_b = len(b[0])

    result = []

    for i in range(rows_a):
        row = []

        for j in range(cols_b):
            value = 0

            for k in range(cols_a):
                value += a[i][k] * b[k][j]

            row.append(value)

        result.append(row)

    return result


def pow_matrix(matrix, power):
    n = len(matrix)
    result = identity_matrix(n)

    for _ in range(power):
        result = mul_matrices(result, matrix)

    return result


def matrix_to_coordinate_text(matrix):
    rows = len(matrix)
    cols = len(matrix[0])

    entries = []

    for i in range(rows):
        for j in range(cols):
            if matrix[i][j] != 0:
                entries.append((i + 1, j + 1, matrix[i][j]))

    lines = []
    lines.append("%%MatrixMarket matrix coordinate real general")
    lines.append(f"{rows} {cols} {len(entries)}")

    for i, j, value in entries:
        lines.append(f"{i} {j} {value}")

    return "\n".join(lines).strip()


def print_matrix(matrix):
    for row in matrix:
        print(" ".join(f"{x:4d}" for x in row))


def write_comments(f, comments=None):
    if comments is None:
        comments = [
            "% generated test",
            "% random matrix",
        ]

    for comment in comments:
        f.write(comment)
        f.write("\n")


def write_matrix_market_coordinate(filepath, matrix):
    rows = len(matrix)
    cols = len(matrix[0])

    entries = []

    for i in range(rows):
        for j in range(cols):
            if matrix[i][j] != 0:
                entries.append((i + 1, j + 1, matrix[i][j]))

    with open(filepath, "w", encoding="utf-8") as f:
        f.write("%%MatrixMarket matrix coordinate real general\n")
        write_comments(f)
        f.write(f"{rows} {cols} {len(entries)}\n")

        for i, j, value in entries:
            f.write(f"{i} {j} {value}\n")


def write_matrix_market_array(filepath, matrix):
    rows = len(matrix)
    cols = len(matrix[0])

    with open(filepath, "w", encoding="utf-8") as f:
        f.write("%%MatrixMarket matrix array real general\n")
        write_comments(f)
        f.write(f"{rows} {cols}\n")

        # Matrix Market array = column-major
        for j in range(cols):
            for i in range(rows):
                f.write(f"{matrix[i][j]}\n")


def write_number(filepath, value):
    with open(filepath, "w", encoding="utf-8") as f:
        f.write(str(value))
        f.write("\n")


def read_out_file():
    if not os.path.exists(OUT_FILE):
        return None

    with open(OUT_FILE, "r", encoding="utf-8") as f:
        return f.read().strip()


def write_matrix_file(filepath, matrix, fmt):
    if fmt == "array":
        write_matrix_market_array(filepath, matrix)
    else:
        write_matrix_market_coordinate(filepath, matrix)


def build_test_data(operator):
    if operator == "|":
        n = random.randint(MIN_SIZE, MAX_SIZE)
        a = generate_matrix(n, n)
        expected = str(determinant(a))

        return {
            "input1_matrix": a,
            "input2_matrix": None,
            "input2_number": None,
            "expected": expected,
        }

    if operator == "+":
        rows = random.randint(MIN_SIZE, MAX_SIZE)
        cols = random.randint(MIN_SIZE, MAX_SIZE)

        a = generate_matrix(rows, cols)
        b = generate_matrix(rows, cols)
        result = add_matrices(a, b)

        return {
            "input1_matrix": a,
            "input2_matrix": b,
            "input2_number": None,
            "expected": matrix_to_coordinate_text(result),
        }

    if operator == "-":
        rows = random.randint(MIN_SIZE, MAX_SIZE)
        cols = random.randint(MIN_SIZE, MAX_SIZE)

        a = generate_matrix(rows, cols)
        b = generate_matrix(rows, cols)
        result = sub_matrices(a, b)

        return {
            "input1_matrix": a,
            "input2_matrix": b,
            "input2_number": None,
            "expected": matrix_to_coordinate_text(result),
        }

    if operator == "*":
        rows_a = random.randint(MIN_SIZE, MAX_SIZE)
        cols_a_rows_b = random.randint(MIN_SIZE, MAX_SIZE)
        cols_b = random.randint(MIN_SIZE, MAX_SIZE)

        a = generate_matrix(rows_a, cols_a_rows_b)
        b = generate_matrix(cols_a_rows_b, cols_b)
        result = mul_matrices(a, b)

        return {
            "input1_matrix": a,
            "input2_matrix": b,
            "input2_number": None,
            "expected": matrix_to_coordinate_text(result),
        }

    if operator == "^":
        n = random.randint(MIN_SIZE, MAX_SIZE)
        a = generate_matrix(n, n)
        power = random.randint(0, 5)
        result = pow_matrix(a, power)

        return {
            "input1_matrix": a,
            "input2_matrix": None,
            "input2_number": power,
            "expected": matrix_to_coordinate_text(result),
        }

    raise ValueError(f"Unknown operator: {operator}")


def operator_to_name(operator):
    mapping = {
        "|": "det",
        "+": "add",
        "-": "sub",
        "*": "mul",
        "^": "pow",
    }

    return mapping[operator]


def run_single_test(operator, test_number, fmt, test_data):
    input1_filename = f"input1_{operator_to_name(operator)}_{test_number}_{fmt}.txt"
    input2_filename = f"input2_{operator_to_name(operator)}_{test_number}_{fmt}.txt"

    input1_filepath = os.path.join(TESTS_DIR, input1_filename)
    input2_filepath = os.path.join(TESTS_DIR, input2_filename)

    input1_arg = os.path.join(TESTS_DIR_NAME, input1_filename)
    input2_arg = os.path.join(TESTS_DIR_NAME, input2_filename)
    out_arg = OUT_FILE_NAME

    write_matrix_file(input1_filepath, test_data["input1_matrix"], fmt)

    if operator == "^":
        write_number(input2_filepath, test_data["input2_number"])
    elif operator in ["+", "-", "*"]:
        write_matrix_file(input2_filepath, test_data["input2_matrix"], fmt)

    if os.path.exists(OUT_FILE):
        os.remove(OUT_FILE)

    command = [EXECUTABLE, operator, out_arg, input1_arg]

    if operator in ["+", "-", "*", "^"]:
        command.append(input2_arg)

    result = subprocess.run(
        command,
        capture_output=True,
        text=True,
        cwd=BASE_DIR
    )

    actual = read_out_file()
    expected = test_data["expected"]

    success = False

    if result.returncode == 0 and actual is not None:
        if operator == "|":
            success = actual == expected
        else:
            try:
                h1, s1, e1 = normalize_coordinate(expected)
                h2, s2, e2 = normalize_coordinate(actual)
                success = (h1 == h2 and s1 == s2 and e1 == e2)
            except Exception:
                success = False

    if success:
        return True

    print("=" * 70)
    print(f"FAIL TEST : {test_number}")
    print(f"OPERATOR  : {operator}")
    print(f"FORMAT    : {fmt}")
    print(f"COMMAND   : {command}")

    print("MATRIX 1:")
    print_matrix(test_data["input1_matrix"])

    if operator in ["+", "-", "*"]:
        print("MATRIX 2:")
        print_matrix(test_data["input2_matrix"])

    if operator == "^":
        print(f"POWER : {test_data['input2_number']}")

    print("\nEXPECTED:")
    print(expected)

    print("\nOUT.TXT:")
    print(actual)

    print(f"\nRETURN CODE: {result.returncode}")

    if result.stdout:
        print("\nSTDOUT:")
        print(result.stdout)

    if result.stderr:
        print("\nSTDERR:")
        print(result.stderr)

    return False


def run_tests():
    print("Запуск тестов...")
    print(f"Рабочая папка : {BASE_DIR}")
    print(f"Исполняемый   : {EXECUTABLE}")
    print(f"Папка tests   : {TESTS_DIR_NAME}")
    print(f"Файл out.txt  : {OUT_FILE_NAME}")
    print()

    exe_path = os.path.join(BASE_DIR, EXECUTABLE)

    if not os.path.exists(exe_path):
        print("Ошибка: a.exe не найден рядом со скриптом.")
        print("Скомпилируй программу:")
        print("gcc main.c -o a.exe")
        return

    ensure_tests_dir()

    total_passed = 0
    total_failed = 0

    for operator in OPERATORS:
        print("#" * 70)
        print(f"ТЕСТЫ ДЛЯ ОПЕРАТОРА {operator}")
        print("#" * 70)

        passed = 0
        failed = 0

        for test_number in range(1, TEST_COUNT_PER_OPERATOR + 1):
            test_data = build_test_data(operator)

            for fmt in ["array", "coordinate"]:
                ok = run_single_test(operator, test_number, fmt, test_data)

                if ok:
                    passed += 1
                    total_passed += 1
                else:
                    failed += 1
                    total_failed += 1

        print("-" * 70)
        print(f"ИТОГ ДЛЯ {operator}: passed={passed}, failed={failed}")
        print("-" * 70)

    print("=" * 70)
    print("FINAL SUMMARY")
    print(f"TOTAL PASSED : {total_passed}")
    print(f"TOTAL FAILED : {total_failed}")
    print("=" * 70)


if __name__ == "__main__":
    run_tests()
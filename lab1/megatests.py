import math
import os
import subprocess
import sys
import tempfile
import textwrap

PROGRAM = "a.exe"


def write_text(path, text):
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(textwrap.dedent(text).lstrip())


def run_program(args, cwd):
    return subprocess.run(
        [PROGRAM] + args,
        cwd=cwd,
        text=True,
        capture_output=True
    )


def normalize_zero(x, eps=1e-7):
    return 0.0 if abs(x) < eps else x


def parse_matrix_market_coordinate(text):
    lines = [line.strip() for line in text.splitlines() if line.strip()]
    if len(lines) < 2:
        raise AssertionError(f"Too short output:\n{text}")

    if lines[0].lower() != "%%matrixmarket matrix coordinate real general":
        raise AssertionError(f"Bad header:\n{text}")

    parts = lines[1].split()
    if len(parts) != 3:
        raise AssertionError(f"Bad size line:\n{text}")

    rows_count = int(parts[0])
    cols_count = int(parts[1])
    nnz = int(parts[2])

    entries = []
    for line in lines[2:]:
        p = line.split()
        if len(p) != 3:
            raise AssertionError(f"Bad coordinate line:\n{text}")
        r = int(p[0])
        c = int(p[1])
        v = normalize_zero(float(p[2]))
        entries.append((r, c, v))

    entries.sort(key=lambda x: (x[0], x[1], x[2]))

    if len(entries) != nnz:
        raise AssertionError(
            f"nnz mismatch: header says {nnz}, but got {len(entries)} entries.\n{text}"
        )

    return rows_count, cols_count, entries


def assert_close(a, b, eps=1e-6):
    return abs(a - b) <= eps


passed = 0
failed = 0


def ok(name):
    global passed
    passed += 1
    print(f"[PASS] {name}")


def fail(name, msg):
    global failed
    failed += 1
    print(f"[FAIL] {name}")
    print(msg)
    print("-" * 80)


def expect_matrix(name, op, in1_text, expected_rows, expected_cols, expected_entries, in2_text=None):
    with tempfile.TemporaryDirectory() as tmp:
        in1 = os.path.join(tmp, "in1.mtx")
        out = os.path.join(tmp, "out.txt")
        write_text(in1, in1_text)

        args = [op, out, in1]

        if in2_text is not None:
            in2 = os.path.join(tmp, "in2.mtx")
            write_text(in2, in2_text)
            args.append(in2)

        result = run_program(args, tmp)

        if result.returncode != 0:
            fail(name, f"Return code = {result.returncode}\nSTDERR:\n{result.stderr}\nSTDOUT:\n{result.stdout}")
            return

        if not os.path.exists(out):
            fail(name, "Output file was not created")
            return

        with open(out, "r", encoding="utf-8") as f:
            out_text = f.read()

        try:
            rows_count, cols_count, entries = parse_matrix_market_coordinate(out_text)
        except Exception as e:
            fail(name, f"Cannot parse output:\n{e}\nOutput was:\n{out_text}")
            return

        exp_entries = sorted(
            [(r, c, normalize_zero(v)) for (r, c, v) in expected_entries],
            key=lambda x: (x[0], x[1], x[2])
        )

        if rows_count != expected_rows or cols_count != expected_cols:
            fail(name, f"Wrong shape: got {rows_count}x{cols_count}, expected {expected_rows}x{expected_cols}\nOutput:\n{out_text}")
            return

        if len(entries) != len(exp_entries):
            fail(name, f"Wrong nnz: got {len(entries)}, expected {len(exp_entries)}\nOutput:\n{out_text}")
            return

        for got, exp in zip(entries, exp_entries):
            if got[0] != exp[0] or got[1] != exp[1] or not assert_close(got[2], exp[2]):
                fail(name, f"Entry mismatch.\nGot: {entries}\nExp: {exp_entries}\nOutput:\n{out_text}")
                return

        ok(name)


def expect_det(name, in1_text, expected_det):
    with tempfile.TemporaryDirectory() as tmp:
        in1 = os.path.join(tmp, "in1.mtx")
        out = os.path.join(tmp, "out.txt")
        write_text(in1, in1_text)

        result = run_program(["|", out, in1], tmp)

        if result.returncode != 0:
            fail(name, f"Return code = {result.returncode}\nSTDERR:\n{result.stderr}\nSTDOUT:\n{result.stdout}")
            return

        if not os.path.exists(out):
            fail(name, "Output file was not created")
            return

        with open(out, "r", encoding="utf-8") as f:
            out_text = f.read().strip()

        try:
            got = float(out_text)
        except ValueError:
            fail(name, f"Output is not a number:\n{out_text}")
            return

        if not assert_close(got, expected_det):
            fail(name, f"Wrong determinant: got {got}, expected {expected_det}")
            return

        ok(name)


def expect_no_solution(name, op, in1_text, in2_text=None):
    with tempfile.TemporaryDirectory() as tmp:
        in1 = os.path.join(tmp, "in1.mtx")
        out = os.path.join(tmp, "out.txt")
        write_text(in1, in1_text)

        args = [op, out, in1]

        if in2_text is not None:
            in2 = os.path.join(tmp, "in2.txt" if op == "^" else "in2.mtx")
            write_text(in2, in2_text)
            args.append(in2)

        result = run_program(args, tmp)

        if result.returncode != 0:
            fail(name, f"Return code = {result.returncode}\nSTDERR:\n{result.stderr}\nSTDOUT:\n{result.stdout}")
            return

        if not os.path.exists(out):
            fail(name, "Output file was not created")
            return

        with open(out, "r", encoding="utf-8") as f:
            out_text = f.read().strip()

        if out_text != "no solution":
            fail(name, f'Expected "no solution", got:\n{out_text}')
            return

        ok(name)


def expect_error(name, op, in1_text, in2_text=None):
    with tempfile.TemporaryDirectory() as tmp:
        in1 = os.path.join(tmp, "in1.mtx")
        out = os.path.join(tmp, "out.txt")
        write_text(in1, in1_text)

        args = [op, out, in1]

        if in2_text is not None:
            in2 = os.path.join(tmp, "in2.txt" if op == "^" else "in2.mtx")
            write_text(in2, in2_text)
            args.append(in2)

        result = run_program(args, tmp)

        if result.returncode == 0:
            fail(name, f"Expected non-zero exit code.\nSTDERR:\n{result.stderr}\nSTDOUT:\n{result.stdout}")
            return

        ok(name)


# -----------------------------
# MATRICES
# -----------------------------

A_ADD = """
%%MatrixMarket matrix coordinate real general
2 2 2
1 1 1
2 2 2
"""

B_ADD = """
%%MatrixMarket matrix coordinate real general
2 2 2
1 2 3
2 2 4
"""

A_CANCEL = """
%%MatrixMarket matrix coordinate real general
1 1 1
1 1 5
"""

B_CANCEL = """
%%MatrixMarket matrix coordinate real general
1 1 1
1 1 -5
"""

A_ARRAY = """
%%MatrixMarket matrix array real general
2 2
1
0
2
3
"""

B_COORD = """
%%MatrixMarket matrix coordinate real general
2 2 1
2 1 4
"""

A_SUB = """
%%MatrixMarket matrix coordinate real general
2 2 3
1 1 5
1 2 1
2 2 4
"""

B_SUB = """
%%MatrixMarket matrix coordinate real general
2 2 2
1 1 2
2 1 7
"""

A_MUL = """
%%MatrixMarket matrix coordinate real general
2 3 4
1 1 1
1 3 2
2 2 3
2 3 4
"""

B_MUL = """
%%MatrixMarket matrix coordinate real general
3 2 4
1 1 5
2 2 6
3 1 7
3 2 8
"""

ZERO_3x2 = """
%%MatrixMarket matrix coordinate real general
3 2 0
"""

FIB = """
%%MatrixMarket matrix coordinate real general
2 2 3
1 1 1
1 2 1
2 1 1
"""

NON_SQUARE = """
%%MatrixMarket matrix coordinate real general
2 3 2
1 1 1
2 3 4
"""

POW0 = "0\n"
POW1 = "1\n"
POW2 = "2\n"
POW5 = "5\n"

DET_1x1 = """
%%MatrixMarket matrix coordinate real general
1 1 1
1 1 7
"""

DET_2x2 = """
%%MatrixMarket matrix coordinate real general
2 2 4
1 1 4
1 2 2
2 1 3
2 2 8
"""

DET_3x3 = """
%%MatrixMarket matrix coordinate real general
3 3 7
1 1 1
1 2 2
1 3 3
2 2 4
2 3 5
3 1 1
3 3 6
"""

DET_SINGULAR = """
%%MatrixMarket matrix coordinate real general
3 3 6
1 1 1
1 2 2
1 3 3
2 1 2
2 2 4
2 3 6
"""

BAD_HEADER = """
%%MatrixMarket matrix wrongformat real general
2 2 0
"""

BAD_COORD_INDEX = """
%%MatrixMarket matrix coordinate real general
2 2 1
3 1 5
"""

# -----------------------------
# TESTS
# -----------------------------

expect_matrix(
    "add_basic",
    "+",
    A_ADD,
    2, 2,
    [
        (1, 1, 1.0),
        (1, 2, 3.0),
        (2, 2, 6.0),
    ],
    B_ADD
)

expect_matrix(
    "add_cancel_to_zero",
    "+",
    A_CANCEL,
    1, 1,
    [],
    B_CANCEL
)

expect_matrix(
    "add_array_and_coordinate",
    "+",
    A_ARRAY,
    2, 2,
    [
        (1, 1, 1.0),
        (1, 2, 2.0),
        (2, 1, 4.0),
        (2, 2, 3.0),
    ],
    B_COORD
)

expect_matrix(
    "subtract_basic",
    "-",
    A_SUB,
    2, 2,
    [
        (1, 1, 3.0),
        (1, 2, 1.0),
        (2, 1, -7.0),
        (2, 2, 4.0),
    ],
    B_SUB
)

expect_matrix(
    "subtract_to_zero",
    "-",
    A_CANCEL,
    1, 1,
    [],
    A_CANCEL
)

expect_matrix(
    "multiply_basic",
    "*",
    A_MUL,
    2, 2,
    [
        (1, 1, 19.0),  # 1*5 + 2*7
        (1, 2, 16.0),  # 2*8
        (2, 1, 28.0),  # 4*7
        (2, 2, 50.0),  # 3*6 + 4*8
    ],
    B_MUL
)

expect_matrix(
    "multiply_by_zero",
    "*",
    A_MUL,
    2, 2,
    [],
    ZERO_3x2
)

expect_no_solution(
    "multiply_no_solution",
    "*",
    A_ADD,
    A_ADD
)

expect_matrix(
    "power_0_identity",
    "^",
    FIB,
    2, 2,
    [
        (1, 1, 1.0),
        (2, 2, 1.0),
    ],
    POW0
)

expect_matrix(
    "power_1_same_matrix",
    "^",
    FIB,
    2, 2,
    [
        (1, 1, 1.0),
        (1, 2, 1.0),
        (2, 1, 1.0),
    ],
    POW1
)

expect_matrix(
    "power_2",
    "^",
    FIB,
    2, 2,
    [
        (1, 1, 2.0),
        (1, 2, 1.0),
        (2, 1, 1.0),
        (2, 2, 1.0),
    ],
    POW2
)

expect_matrix(
    "power_5",
    "^",
    FIB,
    2, 2,
    [
        (1, 1, 8.0),
        (1, 2, 5.0),
        (2, 1, 5.0),
        (2, 2, 3.0),
    ],
    POW5
)

expect_no_solution(
    "power_no_solution_non_square",
    "^",
    NON_SQUARE,
    "2\n"
)

expect_det(
    "det_1x1",
    DET_1x1,
    7.0
)

expect_det(
    "det_2x2",
    DET_2x2,
    26.0
)

expect_det(
    "det_3x3",
    DET_3x3,
    22.0
)

expect_det(
    "det_singular",
    DET_SINGULAR,
    0.0
)

expect_no_solution(
    "det_no_solution_non_square",
    "|",
    NON_SQUARE
)

expect_error(
    "bad_header_error",
    "|",
    BAD_HEADER
)

expect_error(
    "bad_coordinate_index_error",
    "|",
    BAD_COORD_INDEX
)

print("=" * 80)
print(f"PASSED: {passed}")
print(f"FAILED: {failed}")

sys.exit(1 if failed else 0)
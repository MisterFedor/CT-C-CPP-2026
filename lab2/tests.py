import sys
import struct
import zlib
import subprocess
from pathlib import Path
from tempfile import TemporaryDirectory


BYTE = 1
SHORT = 3
LONG = 4

NO_COMPRESSION = 1
DEFLATE_8 = 8
DEFLATE_32946 = 32946
PACKBITS = 32773

PHOTOMETRIC_WHITE_IS_ZERO = 0
PHOTOMETRIC_BLACK_IS_ZERO = 1
PHOTOMETRIC_RGB = 2

NO_PREDICTOR = 1
HORIZONTAL_DIFFERENCING = 2


def pack_values(field_type, values, endian):
    fmt_prefix = "<" if endian == "LE" else ">"

    if field_type == SHORT:
        return b"".join(struct.pack(fmt_prefix + "H", v) for v in values)

    if field_type == LONG:
        return b"".join(struct.pack(fmt_prefix + "I", v) for v in values)

    if field_type == BYTE:
        return bytes(values)

    raise ValueError("unsupported field type")


def make_ifd_entry(tag, field_type, values, endian, value_offset=None):
    fmt_prefix = "<" if endian == "LE" else ">"
    count = len(values)

    raw_values = pack_values(field_type, values, endian)

    if len(raw_values) <= 4:
        value_field = raw_values + b"\x00" * (4 - len(raw_values))
    else:
        value_field = struct.pack(fmt_prefix + "I", value_offset)

    return struct.pack(fmt_prefix + "HHI", tag, field_type, count) + value_field


def packbits_literal(data):
    result = bytearray()
    pos = 0

    while pos < len(data):
        chunk = data[pos:pos + 128]
        result.append(len(chunk) - 1)
        result.extend(chunk)
        pos += len(chunk)

    return bytes(result)


def encode_horizontal_predictor(raw, width, height, channels):
    result = bytearray(raw)
    row_size = width * channels

    for row in range(height):
        row_start = row * row_size

        for i in range(channels, row_size):
            cur = raw[row_start + i]
            left = raw[row_start + i - channels]
            result[row_start + i] = (cur - left) & 0xFF

    return bytes(result)


def make_tiff(
    width,
    height,
    channels,
    photometric,
    raw_pixels,
    *,
    compression=NO_COMPRESSION,
    predictor=None,
    endian="LE",
):
    if predictor == HORIZONTAL_DIFFERENCING:
        stored_pixels = encode_horizontal_predictor(raw_pixels, width, height, channels)
    else:
        stored_pixels = raw_pixels

    if compression == NO_COMPRESSION:
        strip_data = stored_pixels
    elif compression in (DEFLATE_8, DEFLATE_32946):
        strip_data = zlib.compress(stored_pixels)
    elif compression == PACKBITS:
        strip_data = packbits_literal(stored_pixels)
    else:
        raise ValueError("unsupported compression in test generator")

    fmt_prefix = "<" if endian == "LE" else ">"
    byte_order = b"II" if endian == "LE" else b"MM"

    tags = [
        (256, LONG, [width]),                 # ImageWidth
        (257, LONG, [height]),                # ImageLength
        (258, SHORT, [8] * channels),         # BitsPerSample
        (259, SHORT, [compression]),          # Compression
        (262, SHORT, [photometric]),          # PhotometricInterpretation
        (273, LONG, [0]),                     # StripOffsets, заполним позже
        (277, SHORT, [channels]),             # SamplesPerPixel
        (278, LONG, [height]),                # RowsPerStrip
        (279, LONG, [len(strip_data)]),       # StripByteCounts
    ]

    if predictor is not None:
        tags.append((317, SHORT, [predictor]))  # Predictor

    tags.sort(key=lambda x: x[0])

    ifd_offset = 8
    ifd_size = 2 + len(tags) * 12 + 4
    extra_offset = ifd_offset + ifd_size

    extra_data = bytearray()
    prepared = []

    for tag, field_type, values in tags:
        raw_values = pack_values(field_type, values, endian)

        if len(raw_values) > 4:
            value_offset = extra_offset + len(extra_data)
            extra_data.extend(raw_values)
        else:
            value_offset = None

        prepared.append([tag, field_type, values, value_offset])

    strip_offset = extra_offset + len(extra_data)

    for item in prepared:
        if item[0] == 273:
            item[2] = [strip_offset]

    header = byte_order
    header += struct.pack(fmt_prefix + "H", 42)
    header += struct.pack(fmt_prefix + "I", ifd_offset)

    ifd = bytearray()
    ifd.extend(struct.pack(fmt_prefix + "H", len(prepared)))

    for tag, field_type, values, value_offset in prepared:
        ifd.extend(make_ifd_entry(tag, field_type, values, endian, value_offset))

    ifd.extend(struct.pack(fmt_prefix + "I", 0))  # next IFD offset

    return header + bytes(ifd) + bytes(extra_data) + strip_data


def expected_pnm(width, height, channels, photometric, raw_pixels):
    if channels == 1:
        magic = b"P5"

        if photometric == PHOTOMETRIC_WHITE_IS_ZERO:
            data = bytes((~b) & 0xFF for b in raw_pixels)
        else:
            data = raw_pixels

    elif channels == 3:
        magic = b"P6"
        data = raw_pixels
    else:
        raise ValueError("unsupported channels")

    header = magic + f"\n{width} {height}\n255\n".encode("ascii")
    return header + data


def first_diff(a, b):
    n = min(len(a), len(b))

    for i in range(n):
        if a[i] != b[i]:
            return f"first diff at byte {i}: got {a[i]:02X}, expected {b[i]:02X}"

    if len(a) != len(b):
        return f"different size: got {len(a)}, expected {len(b)}"

    return "no diff"


def run_converter(exe_path, input_path, output_path):
    # Если твой main принимает аргументы иначе, поменяй только эту строку.
    return subprocess.run(
        [str(exe_path), str(input_path), str(output_path)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )


def run_one_test(exe_path, tmpdir, name, tiff_bytes, expected_bytes):
    input_path = tmpdir / f"{name}.tiff"
    output_path = tmpdir / f"{name}.pnm"

    input_path.write_bytes(tiff_bytes)

    result = run_converter(exe_path, input_path, output_path)

    if result.returncode != 0:
        stdout = result.stdout.decode(errors="replace")
        stderr = result.stderr.decode(errors="replace")
        raise AssertionError(
            f"{name}: program returned {result.returncode}\n"
            f"stdout:\n{stdout}\n"
            f"stderr:\n{stderr}"
        )

    if not output_path.exists():
        raise AssertionError(f"{name}: output file was not created")

    actual = output_path.read_bytes()

    if actual != expected_bytes:
        raise AssertionError(
            f"{name}: bad output\n{first_diff(actual, expected_bytes)}"
        )


def main():
    if len(sys.argv) >= 2:
        exe_path = Path(sys.argv[1]).resolve()
    else:
        exe_path = Path("a.exe").resolve()

    if not exe_path.exists():
        print(f"ERROR: exe not found: {exe_path}")
        return 2

    tests = []

    # 1. Gray, little-endian, no compression
    raw = bytes([0, 127, 200, 255])
    tests.append((
        "gray_le_no_compression",
        make_tiff(2, 2, 1, PHOTOMETRIC_BLACK_IS_ZERO, raw, endian="LE"),
        expected_pnm(2, 2, 1, PHOTOMETRIC_BLACK_IS_ZERO, raw),
    ))

    # 2. Gray, big-endian, no compression
    raw = bytes([10, 20, 30, 40])
    tests.append((
        "gray_be_no_compression",
        make_tiff(2, 2, 1, PHOTOMETRIC_BLACK_IS_ZERO, raw, endian="BE"),
        expected_pnm(2, 2, 1, PHOTOMETRIC_BLACK_IS_ZERO, raw),
    ))

    # 3. Gray, white is zero: output должен инвертироваться
    raw = bytes([0, 64, 128, 255])
    tests.append((
        "gray_white_is_zero",
        make_tiff(2, 2, 1, PHOTOMETRIC_WHITE_IS_ZERO, raw, endian="LE"),
        expected_pnm(2, 2, 1, PHOTOMETRIC_WHITE_IS_ZERO, raw),
    ))

    # 4. RGB, no compression
    raw = bytes([
        255, 0, 0,
        0, 255, 0,
    ])
    tests.append((
        "rgb_le_no_compression",
        make_tiff(2, 1, 3, PHOTOMETRIC_RGB, raw, endian="LE"),
        expected_pnm(2, 1, 3, PHOTOMETRIC_RGB, raw),
    ))

    # 5. Deflate compression = 8
    raw = bytes([1, 2, 3, 4, 5, 6])
    tests.append((
        "gray_deflate_8",
        make_tiff(3, 2, 1, PHOTOMETRIC_BLACK_IS_ZERO, raw, compression=DEFLATE_8, endian="LE"),
        expected_pnm(3, 2, 1, PHOTOMETRIC_BLACK_IS_ZERO, raw),
    ))

    # 6. Deflate compression = 32946
    raw = bytes([11, 22, 33, 44, 55, 66])
    tests.append((
        "gray_deflate_32946",
        make_tiff(3, 2, 1, PHOTOMETRIC_BLACK_IS_ZERO, raw, compression=DEFLATE_32946, endian="LE"),
        expected_pnm(3, 2, 1, PHOTOMETRIC_BLACK_IS_ZERO, raw),
    ))

    # 7. PackBits, literal chunks
    raw = bytes([7, 8, 9, 10, 11, 12])
    tests.append((
        "gray_packbits",
        make_tiff(3, 2, 1, PHOTOMETRIC_BLACK_IS_ZERO, raw, compression=PACKBITS, endian="LE"),
        expected_pnm(3, 2, 1, PHOTOMETRIC_BLACK_IS_ZERO, raw),
    ))

    # 8. RGB + Predictor 2, no compression
    raw = bytes([
        10, 20, 30,
        15, 25, 35,
        25, 40, 50,

        100, 110, 120,
        101, 115, 130,
        130, 140, 150,
    ])
    tests.append((
        "rgb_predictor_no_compression",
        make_tiff(
            3,
            2,
            3,
            PHOTOMETRIC_RGB,
            raw,
            compression=NO_COMPRESSION,
            predictor=HORIZONTAL_DIFFERENCING,
            endian="LE",
        ),
        expected_pnm(3, 2, 3, PHOTOMETRIC_RGB, raw),
    ))

    # 9. Gray + Predictor 2 + Deflate
    raw = bytes([
        10, 20, 30,
        100, 105, 130,
    ])
    tests.append((
        "gray_predictor_deflate",
        make_tiff(
            3,
            2,
            1,
            PHOTOMETRIC_BLACK_IS_ZERO,
            raw,
            compression=DEFLATE_8,
            predictor=HORIZONTAL_DIFFERENCING,
            endian="LE",
        ),
        expected_pnm(3, 2, 1, PHOTOMETRIC_BLACK_IS_ZERO, raw),
    ))

    passed = 0

    with TemporaryDirectory() as tmp:
        tmpdir = Path(tmp)

        for name, tiff_bytes, expected_bytes in tests:
            try:
                run_one_test(exe_path, tmpdir, name, tiff_bytes, expected_bytes)
                print(f"[PASS] {name}")
                passed += 1
            except AssertionError as e:
                print(f"[FAIL] {e}")

    print()
    print(f"Passed: {passed}/{len(tests)}")

    if passed != len(tests):
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
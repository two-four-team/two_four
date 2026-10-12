#!/usr/bin/env python3
"""Run unit test cases for one module function.

Usage: python3 test/test_unit.py FUNCTION

Reads test/testcases/FUNCTION.csv. Columns are the function's parameter
names plus a column named FUNCTION holding the expected return value.
A blank input cell means "don't care": every enum value (or a set of
representative ints) is tried, and every combination must return the
expected value.
"""
import argparse
import csv
import ctypes
import itertools
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
HEADER = ROOT / 'Common' / 'vehicle_control.h'
CASES = ROOT / 'test' / 'testcases'
SOURCES = ['HeadLight/headLight.c', 'Wiper/Wiper.c', 'Door/Door.c',
           'sensor/sensor.c', 'Dashboard Warning/ClusterWarning.c']
INT_TYPES = {'int': ctypes.c_int, 'unsigned int': ctypes.c_uint}
LARGE_INT = 100000  # blank int sample beyond every threshold in the rules


def parse_header():
    text = re.sub(r'/\*.*?\*/', '', HEADER.read_text(encoding='utf-8'), flags=re.S)
    enums = {}
    for body, name in re.findall(r'typedef\s+enum\s*\{(.*?)\}\s*(\w+)\s*;', text, re.S):
        values, value = {}, 0
        for item in filter(None, (s.strip() for s in body.split(','))):
            key, _, explicit = (p.strip() for p in item.partition('='))
            value = int(explicit, 0) if explicit else value
            values[key] = value
            value += 1
        enums[name] = values
    functions = {}
    for ret, func, params in re.findall(r'(\w+(?:\s+int)?)\s+(\w+)\s*\(([^)]*)\)\s*;', text):
        args = []
        for param in filter(None, (p.strip() for p in params.split(','))):
            ptype, pname = param.rsplit(None, 1)
            args.append((' '.join(ptype.split()), pname))
        functions[func] = (ret, args)
    return enums, functions


def build_library(workdir):
    lib = Path(workdir) / 'libvehicle.so'
    subprocess.run(['gcc', '-std=c99', '-Wall', '-Wextra', '-shared', '-fPIC',
                    '-I', 'Common', '-o', str(lib), *SOURCES],
                   cwd=ROOT, check=True)
    return ctypes.CDLL(str(lib))


def ctype_of(ctype, enums):
    if ctype in enums:
        return ctypes.c_int
    if ctype in INT_TYPES:
        return INT_TYPES[ctype]
    raise ValueError(f'Unsupported C type: {ctype}')


def parse_value(text, ctype, enums):
    if ctype in enums:
        if text not in enums[ctype]:
            raise ValueError(f'{text!r} is not a {ctype} value')
        return enums[ctype][text]
    return int(text)


def label(value, ctype, enums):
    names = {v: k for k, v in enums.get(ctype, {}).items()}
    return names.get(value, value)


def candidates(ctype, column, rows, enums):
    """Values tried for a blank cell of this column."""
    if ctype in enums:
        return sorted(enums[ctype].values())
    used = {int(r[column]) for r in rows if r[column].strip()}
    return sorted(used | {0, LARGE_INT})


def run(function):
    enums, functions = parse_header()
    if function not in functions:
        raise ValueError(f'Unknown function {function!r}; available: {", ".join(sorted(functions))}')
    ret, params = functions[function]
    path = CASES / f'{function}.csv'
    with path.open(encoding='utf-8-sig', newline='') as f:
        reader = csv.DictReader(f)
        columns = reader.fieldnames or []
        rows = list(reader)
    expected = {'testCaseId', function} | {name for _, name in params}
    if set(columns) != expected:
        raise ValueError(f'{path}: columns must be {sorted(expected)}, got {columns}')

    with tempfile.TemporaryDirectory() as workdir:
        func = getattr(build_library(workdir), function)
        func.argtypes = [ctype_of(ctype, enums) for ctype, _ in params]
        func.restype = ctype_of(ret, enums)

        failed = 0
        for row in rows:
            case = row['testCaseId'].strip()
            want = parse_value(row[function].strip(), ret, enums)
            choices = []
            for ctype, name in params:
                cell = row[name].strip()
                choices.append([parse_value(cell, ctype, enums)] if cell
                               else candidates(ctype, name, rows, enums))
            bad = None
            combos = 0
            for args in itertools.product(*choices):
                combos += 1
                got = func(*args)
                if got != want and bad is None:
                    bad = (args, got)
            if bad is None:
                print(f'[PASS] {case} ({combos} combination(s))')
            else:
                failed += 1
                args, got = bad
                shown = ', '.join(f'{name}={label(value, ctype, enums)}'
                                  for (ctype, name), value in zip(params, args))
                print(f'[FAIL] {case} expected {label(want, ret, enums)}, '
                      f'got {label(got, ret, enums)} with {shown}')
    print(f'{function}: {len(rows) - failed}/{len(rows)} passed')
    return 1 if failed else 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('function', help='Function name, e.g. ControlHeadLight')
    args = parser.parse_args()
    try:
        return run(args.function)
    except (OSError, ValueError, subprocess.CalledProcessError) as exc:
        print(f'ERROR: {exc}', file=sys.stderr)
        return 2


if __name__ == '__main__':
    sys.exit(main())

#!/usr/bin/env python3
"""Aggregate existing unit-test results by requirement traceability.

Usage: python3 test/test_requirement.py [REQUIREMENT_ID]

Runs the existing test_driver.py once per linked function and uses its
per-case [PASS]/[FAIL] lines to calculate requirement results.
"""
import argparse
import csv
import re
import subprocess
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
REQUIREMENTS = ROOT / 'requirements' / 'requirements.csv'
TRACEABILITY = ROOT / 'traceability' / 'traceability.csv'
DRIVER = ROOT / 'test' / 'test_unit.py'
RESULT = re.compile(r'^\[(PASS|FAIL)\]\s+(\S+)', re.MULTILINE)


def read_csv(path, fields):
    with path.open(encoding='utf-8-sig', newline='') as f:
        reader = csv.DictReader(f)
        if not set(fields).issubset(reader.fieldnames or []):
            raise ValueError(f'{path}: missing columns {set(fields) - set(reader.fieldnames or [])}')
        return list(reader)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('requirement', nargs='?', help='Requirement ID, e.g. L1')
    args = parser.parse_args()
    try:
        reqs = read_csv(REQUIREMENTS, ['requirementId'])
        links = read_csv(TRACEABILITY, ['requirementId', 'function', 'testFile', 'testCaseId'])
        ids = [r['requirementId'].strip() for r in reqs]
        if len(ids) != len(set(ids)):
            raise ValueError('Duplicate requirement IDs in requirements.csv')
        if args.requirement and args.requirement not in ids:
            raise ValueError(f'Unknown requirement: {args.requirement}')
        selected = [args.requirement] if args.requirement else ids
        linked = defaultdict(set)
        for link in links:
            rid = link['requirementId'].strip()
            if rid not in ids:
                raise ValueError(f'Unknown requirement in traceability.csv: {rid}')
            if rid not in selected:
                continue
            func, case = link['function'].strip(), link['testCaseId'].strip()
            test_file = ROOT / link['testFile'].strip()
            if not test_file.is_file():
                raise ValueError(f'Missing test file: {test_file}')
            if not func or not case:
                raise ValueError(f'Empty function or testCaseId for {rid}')
            if case not in {r['testCaseId'] for r in read_csv(test_file, ['testCaseId'])}:
                raise ValueError(f'Missing test case {case} in {test_file}')
            linked[rid].add((func, case))
        functions = sorted({func for rid in selected for func, _ in linked[rid]})
        results = {}
        for func in functions:
            proc = subprocess.run([sys.executable, str(DRIVER), func], cwd=ROOT,
                                  capture_output=True, text=True)
            output = proc.stdout + '\n' + proc.stderr
            found = RESULT.findall(output)
            if not found:
                raise RuntimeError(f'{func}: cannot parse test results (exit {proc.returncode})\n{output}')
            for status, case in found:
                key = (func, case)
                if key in results:
                    raise ValueError(f'Duplicate test result: {func}/{case}')
                results[key] = status
            # A failed driver process without an explicit case failure is a harness error.
            if proc.returncode != 0 and not any(s == 'FAIL' for s, _ in found):
                raise RuntimeError(f'{func}: driver failed (exit {proc.returncode})\n{output}')
        print(f'{"Requirement":<14} {"Tests":>6} {"PASS":>6} {"FAIL":>6}  Result')
        print('-' * 53)
        total = passed = failed = 0
        bad = False
        for rid in selected:
            cases = linked[rid]
            p = sum(results.get(key) == 'PASS' for key in cases)
            f = sum(results.get(key) == 'FAIL' for key in cases)
            missing = len(cases) - p - f
            status = 'NOT TESTED' if not cases else ('PASS' if p == len(cases) else 'FAIL')
            bad |= status != 'PASS'
            total += len(cases)
            passed += p
            failed += f
            print(f'{rid:<14} {len(cases):>6} {p:>6} {f:>6}  {status}' +
                  (f' ({missing} missing result)' if missing else ''))
        print('-' * 53)
        print(f'{"TOTAL":<14} {total:>6} {passed:>6} {failed:>6}  {"FAIL" if bad else "PASS"}')
        return 1 if bad else 0
    except (OSError, ValueError, RuntimeError) as exc:
        print(f'ERROR: {exc}', file=sys.stderr)
        return 2


if __name__ == '__main__':
    sys.exit(main())

#!/usr/bin/env python3
"""Show candidate change impacts using the requirements traceability matrix."""
import argparse
import csv
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
REQUIREMENTS = ROOT / 'requirements' / 'requirements.csv'
TRACEABILITY = ROOT / 'traceability' / 'traceability.csv'


def load_csv(path, required):
    with path.open(newline='', encoding='utf-8-sig') as stream:
        reader = csv.DictReader(stream)
        if not reader.fieldnames or not set(required).issubset(reader.fieldnames):
            raise ValueError(f'{path}: required columns: {", ".join(required)}')
        return list(reader)


def analyze(requirement_id):
    requirements = load_csv(REQUIREMENTS, ('requirementId', 'description'))
    links = load_csv(TRACEABILITY, ('requirementId', 'sourceFile', 'function', 'testFile', 'testCaseId'))
    by_id = {}
    for requirement in requirements:
        rid = requirement['requirementId'].strip()
        if not rid or rid in by_id:
            raise ValueError(f'Empty or duplicate requirement ID: {rid!r}')
        by_id[rid] = requirement
    if requirement_id not in by_id:
        raise ValueError(f'Unknown requirement {requirement_id!r}; available: {", ".join(sorted(by_id))}')

    print(f'=== Change Impact Candidates: {requirement_id} ===')
    print('Requirement:', by_id[requirement_id]['description'])
    selected = [r for r in links if r['requirementId'].strip() == requirement_id]
    if not selected:
        print('\nNo traceability links found (UNTRACED).')
        return 1

    functions = defaultdict(set)
    test_files = defaultdict(set)
    for row in selected:
        source, function = row['sourceFile'].strip(), row['function'].strip()
        test_file, case_id = row['testFile'].strip(), row['testCaseId'].strip()
        if not all((source, function, test_file, case_id)):
            raise ValueError(f'Incomplete traceability row: {row}')
        functions[source].add(function)
        test_files[test_file].add(case_id)

    print('\nImplementation candidates:')
    for source in sorted(functions):
        suffix = ' [MISSING]' if not (ROOT / source).is_file() else ''
        print(f'  {source}{suffix}')
        for function in sorted(functions[source]):
            print(f'    - {function}()')

    print('\nTest cases to review:')
    for test_file in sorted(test_files):
        path = ROOT / test_file
        suffix = ' [MISSING]' if not path.is_file() else ''
        print(f'  {test_file}{suffix}')
        ids = sorted(test_files[test_file])
        if path.is_file():
            rows = load_csv(path, ('testCaseId',))
            existing = {row['testCaseId'].strip() for row in rows}
            for case_id in ids:
                print(f'    - {case_id}' + (' [MISSING]' if case_id not in existing else ''))
        else:
            for case_id in ids:
                print(f'    - {case_id} [UNVERIFIED]')
    print(f'\nSummary: {len(functions)} source file(s), '
          f'{sum(map(len, functions.values()))} function(s), '
          f'{len(test_files)} test file(s), '
          f'{sum(map(len, test_files.values()))} unique test case(s)')
    print('Note: These are traceability-based candidates, not confirmed code changes.')
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('requirement_id', help='Requirement ID, e.g. D3')
    args = parser.parse_args()
    try:
        return analyze(args.requirement_id)
    except (OSError, ValueError) as exc:
        print(f'Impact analysis error: {exc}', file=sys.stderr)
        return 2


if __name__ == '__main__':
    sys.exit(main())

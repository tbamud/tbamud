#!/usr/bin/env python3
"""Convert Unity test-runner output to JUnit XML.

Usage:
    ./test_binary | python3 unity_to_junit.py <suite_name> <output.xml> [elapsed_seconds]

Unity emits one result line per test:
    path/to/file.c:LINE:TEST_NAME:PASS
    path/to/file.c:LINE:TEST_NAME:FAIL:message
    path/to/file.c:LINE:TEST_NAME:IGNORE:message
followed by a summary line:
    N Tests N Failures N Ignored
"""

import re
import sys
import xml.etree.ElementTree as ET


def parse_unity(lines):
    tests = []
    total = failures = ignored = 0
    for line in lines:
        line = line.rstrip("\n")
        m = re.match(
            r"^.+:\d+:([^:]+):(PASS|FAIL|IGNORE)(?::(.*))?$", line
        )
        if m:
            name, result, message = m.group(1), m.group(2), m.group(3) or ""
            tests.append((name, result, message))
            continue
        m2 = re.match(r"^(\d+) Tests (\d+) Failures (\d+) Ignored", line)
        if m2:
            total, failures, ignored = int(m2.group(1)), int(m2.group(2)), int(m2.group(3))
    if not total:
        total = len(tests)
        failures = sum(1 for _, r, _ in tests if r == "FAIL")
        ignored = sum(1 for _, r, _ in tests if r == "IGNORE")
    return tests, total, failures, ignored


def build_xml(suite_name, tests, total, failures, ignored, elapsed):
    # Distribute total time evenly across tests for per-testcase timing.
    per_test = round(elapsed / total, 6) if total else 0.0
    suite = ET.Element(
        "testsuite",
        name=suite_name,
        tests=str(total),
        failures=str(failures),
        errors="0",
        skipped=str(ignored),
        time=f"{elapsed:.6f}",
    )
    for name, result, message in tests:
        case = ET.SubElement(
            suite, "testcase",
            name=name, classname=suite_name, time=f"{per_test:.6f}",
        )
        if result == "FAIL":
            f = ET.SubElement(case, "failure", message=message)
            f.text = message
        elif result == "IGNORE":
            ET.SubElement(case, "skipped", message=message)
    return ET.ElementTree(suite)


def main():
    if len(sys.argv) < 3 or len(sys.argv) > 4:
        print(f"usage: {sys.argv[0]} <suite_name> <output.xml> [elapsed_seconds]", file=sys.stderr)
        sys.exit(1)
    suite_name, output_file = sys.argv[1], sys.argv[2]
    elapsed = float(sys.argv[3]) if len(sys.argv) == 4 else 0.0
    tests, total, failures, ignored = parse_unity(sys.stdin.readlines())
    tree = build_xml(suite_name, tests, total, failures, ignored, elapsed)
    ET.indent(tree, space="  ")
    tree.write(output_file, encoding="unicode", xml_declaration=True)


if __name__ == "__main__":
    main()

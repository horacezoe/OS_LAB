from subprocess import Popen, PIPE
from random import randint
from typing import Generator, Tuple
from pathlib import Path
import sys


def generate_test_cases(*,
                        count: int = 100_000,
                        min_digit: int = 20,
                        max_digit: int = 29) -> Generator[Tuple[str, str], None, None]:
    for _ in range(count):
        digit = randint(min_digit, max_digit)
        num = randint(10 ** digit, 10 ** (digit + 1))
        yield f"{num} b", bin(num)
        yield f"{num} o", oct(num)
        yield f"{num} h", hex(num)


def start_test(exec_path: str | Path, /) -> None:
    p = Popen([exec_path], stdin=PIPE, stdout=PIPE, stderr=PIPE, encoding="UTF-8")
    for input_str, ground_truth in generate_test_cases():
        p.stdin.write(input_str + "\n")
        p.stdin.flush()
        out = p.stdout.readline()
        out = out.strip()
        if out != ground_truth:
            print(f"Test failed: {input_str} -> {out}, expected: {ground_truth}")
            sys.exit(1)
    p.stdin.write("q" + "\n")
    p.stdin.flush()
    print("All tests passed!")


def main() -> None:
    if len(sys.argv) != 2:
        print("Usage: python3 lab1_test.py <exec_path>")
        return
    start_test(sys.argv[1])


if __name__ == "__main__":
    main()

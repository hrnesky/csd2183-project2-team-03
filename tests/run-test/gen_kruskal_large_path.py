#!/usr/bin/env python3
"""Writes tests/test05.txt: a path of weight-1 bridges plus heavy chords.

To run the file: 
python tests/run-test/gen_kruskal_large_path.py 50000 200000 tests/test05.txt

usage: python gen_kruskal_large_path.py [islands] [chords] [output]
defaults: 200000 islands, 1000000 chords (Table 6 limits)

Why it is adversarial:
  * The chain B i i+1 1 is processed first (lightest). With naive linking it
    builds one long parent chain.
  * Every chord (a, b) with b - a >= 2 and weight >= 2 closes a cycle and must be
    rejected. Each rejection calls find on both ends, so a find without
    path compression walks the long chain again and again.
  * Expected result: total = n - 1, bridges = n - 1.
"""
import random
import sys

n = int(sys.argv[1]) if len(sys.argv) > 1 else 200_000
chords = int(sys.argv[2]) if len(sys.argv) > 2 else 1_000_000
path = sys.argv[3] if len(sys.argv) > 3 else "kruskal_large_path.txt"

# At most this many distinct non-adjacent pairs exist.
max_chords = n * (n - 1) // 2 - (n - 1)
chords = min(chords, max_chords)

rng = random.Random(2183)
seen = set()
lines = [
    f"# kruskal_large_path: n = {n}, chain of {n - 1} weight-1 bridges + {chords} heavy chords.",
    f"# Expected: maintain {n - 1} {n - 1}  (compare the first line only)",
    f"N {n} 0",
]
lines += [f"B {i} {i + 1} 1" for i in range(n - 1)]

# First the chords from island 0, which force the longest find under naive linking.
for j in range(2, n):
    if len(seen) >= chords:
        break
    seen.add((0, j))
    lines.append(f"B 0 {j} {rng.randint(2, 10**9)}")
# Then random chords, no duplicates and no chain bridges.
while len(seen) < chords:
    a, b = rng.randrange(n), rng.randrange(n)
    if a > b:
        a, b = b, a
    if b - a < 2 or (a, b) in seen:
        continue
    seen.add((a, b))
    lines.append(f"B {a} {b} {rng.randint(2, 10**9)}")

lines += ["maintain kruskal", "maintain prim"]
with open(path, "w", newline="\n") as f:
    f.write("\n".join(lines) + "\n")
print(f"wrote {path}: {len(lines)} lines")
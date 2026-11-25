#!/usr/bin/env python3
"""Runs isles on every test file and checks each answer with an independent reference.

The C++ driver only checks the SHAPE of results. This script checks the CONTENT:

  1. Runs isles on each test file (exit code must be 0) and times it.
  2. Walks the command file and the output together, one query at a time.
  3. regions, hops             : exact comparison with a reference answer.
  4. maintain kruskal / prim   : total and count must match a reference Kruskal;
                                 the printed bridges must exist in the input, form a
                                 forest, and add up to the printed total.
  5. fastest, stamina          : distance must match a reference; the printed path (or
                                 negative cycle) must use real edges and add up.
  6. unlock                    : cycle detection, finish time, and the critical chain.
  7. most, best                : the chosen events must not overlap and must reach the
                                 optimal count (most) or reward (best).

Usage (from the repo root):
  python tests/run-test/check_all.py                         # tests/test*.txt with .\\isles.exe
  python tests/run-test/check_all.py tests/test01.txt ...    # chosen files
  python tests/run-test/check_all.py --exe build/isles.exe

Assumptions to confirm against the spec:
  TOUCHING_OK   events with finish == next start are compatible.
  stamina       "exploit" means a negative cycle REACHABLE from the start island.
"""
import argparse
import bisect
import glob
import heapq
import os
import re
import subprocess
import sys
import time
from collections import deque

TOUCHING_OK = True
INF = float("inf")


class CheckError(Exception):
    pass


def need(ok, msg):
    if not ok:
        raise CheckError(msg)


class World:
    def __init__(self, n, k):
        self.n, self.k = n, k
        self.dur = [0] * k
        self.bridges, self.routes, self.pre, self.events = [], [], [], []
        self.cache = {}


def ints(tokens, what):
    try:
        return [int(t) for t in tokens]
    except ValueError:
        raise CheckError(f"{what}: expected integers, got {tokens}")

def dsu_find(n):
    p = list(range(n))

    def find(x):
        r = x
        while p[r] != r:
            r = p[r]
        while p[x] != r:
            p[x], x = r, p[x]
        return r

    return find, p


def adjacency(n, edges, directed):
    adj = [[] for _ in range(n)]
    for a, b, wt in edges:
        adj[a].append((b, wt))
        if not directed:
            adj[b].append((a, wt))
    return adj


def dijkstra(n, adj, s):
    dist = [INF] * n
    dist[s] = 0
    heap = [(0, s)]
    while heap:
        d, u = heapq.heappop(heap)
        if d > dist[u]:
            continue
        for v, wt in adj[u]:
            if d + wt < dist[v]:
                dist[v] = d + wt
                heapq.heappush(heap, (d + wt, v))
    return dist


def spfa(n, adj, s):
    """Distances from s, or None if a negative cycle is reachable from s."""
    dist = [INF] * n
    dist[s] = 0
    cnt = [0] * n
    inq = [False] * n
    dq = deque([s])
    inq[s] = True
    while dq:
        u = dq.popleft()
        inq[u] = False
        for v, wt in adj[u]:
            if dist[u] + wt < dist[v]:
                dist[v] = dist[u] + wt
                cnt[v] = cnt[u] + 1
                if cnt[v] >= n:
                    return None
                if not inq[v]:
                    inq[v] = True
                    dq.append(v)
    return dist


def reachable(n, adj, s):
    seen = [False] * n
    seen[s] = True
    dq = deque([s])
    while dq:
        u = dq.popleft()
        for v, _ in adj[u]:
            if not seen[v]:
                seen[v] = True
                dq.append(v)
    return seen


def quest_info(w):
    if "quests" in w.cache:
        return w.cache["quests"]
    k = w.k
    indeg = [0] * k
    succ = [[] for _ in range(k)]
    preds = [[] for _ in range(k)]
    for a, b in w.pre:
        succ[a].append(b)
        preds[b].append(a)
        indeg[b] += 1
    dq = deque(i for i in range(k) if indeg[i] == 0)
    order = []
    while dq:
        u = dq.popleft()
        order.append(u)
        for v in succ[u]:
            indeg[v] -= 1
            if indeg[v] == 0:
                dq.append(v)
    if len(order) < k:
        info = None
    else:
        fin, maxpre = [0] * k, [0] * k
        for v in order:
            maxpre[v] = max((fin[u] for u in preds[v]), default=0)
            fin[v] = maxpre[v] + w.dur[v]
        info = (fin, maxpre, preds)
    w.cache["quests"] = info
    return info


def event_refs(w):
    if "events" in w.cache:
        return w.cache["events"]
    ev = w.events
    order = sorted(range(len(ev)), key=lambda i: ev[i][1])
    fins = [ev[i][1] for i in order]
    find_p = bisect.bisect_right if TOUCHING_OK else bisect.bisect_left
    dp = [0] * (len(ev) + 1)
    for j, i in enumerate(order, 1):
        s, f, r = ev[i]
        dp[j] = max(dp[j - 1], r + dp[find_p(fins, s)])
    count, last = 0, -INF
    for i in order:
        s, f, _ = ev[i]
        if (s >= last) if TOUCHING_OK else (s > last):
            count += 1
            last = f
    w.cache["events"] = (count, dp[len(ev)])
    return w.cache["events"]

def expect_lines(take, expected):
    for e in expected:
        got = take()
        need(got == e, f"expected '{e}' but got '{got}'")


def check_regions(w, take):
    find, p = dsu_find(w.n)
    for a, b, _ in w.bridges:
        ra, rb = find(a), find(b)
        if ra != rb:
            p[ra] = rb
    groups, order = {}, []
    for v in range(w.n):
        r = find(v)
        if r not in groups:
            groups[r] = []
            order.append(r)
        groups[r].append(v)
    expect_lines(take, [f"regions {len(order)}"] + [" ".join(map(str, groups[r])) for r in order])


def check_hops(w, s, t, take):
    adj = adjacency(w.n, w.bridges, False)
    dist = [-1] * w.n
    dist[s] = 0
    dq = deque([s])
    while dq:
        u = dq.popleft()
        for v, _ in adj[u]:
            if dist[v] < 0:
                dist[v] = dist[u] + 1
                dq.append(v)
    ans = "unreachable" if dist[t] < 0 else str(dist[t])
    expect_lines(take, [f"hops {s} {t} {ans}"])


def check_maintain(w, name, take):
    if "mst" not in w.cache:
        find, p = dsu_find(w.n)
        total = cnt = 0
        for a, b, wt in sorted(w.bridges, key=lambda e: e[2]):
            ra, rb = find(a), find(b)
            if ra != rb:
                p[ra] = rb
                total += wt
                cnt += 1
        w.cache["mst"] = (total, cnt, {(min(a, b), max(a, b)): wt for a, b, wt in w.bridges})
    total, cnt, bridge_w = w.cache["mst"]
    head = take().split()
    need(len(head) == 3 and head[0] == "maintain", f"bad header {head}")
    got_total, got_cnt = ints(head[1:], "maintain header")
    need(got_total == total, f"{name}: total {got_total}, minimum is {total}")
    need(got_cnt == cnt, f"{name}: {got_cnt} bridges, a forest needs {cnt}")
    find, p = dsu_find(w.n)
    s = 0
    for _ in range(got_cnt):
        a, b, wt = ints(take().split(), "maintain edge")
        key = (min(a, b), max(a, b))
        need(bridge_w.get(key) == wt, f"{name}: bridge {a} {b} {wt} is not in the input")
        ra, rb = find(a), find(b)
        need(ra != rb, f"{name}: bridge {a} {b} closes a cycle")
        p[ra] = rb
        s += wt
    need(s == got_total, f"{name}: bridges add up to {s}, header says {got_total}")


def check_path(tokens, ref, weight, directed_name):
    """tokens = [dist, v0, v1, ...] ; weight maps (a, b) -> length."""
    vals = ints(tokens, "path")
    need(len(vals) >= 2, "missing distance or path")
    dist, path = vals[0], vals[1:]
    need(dist == ref[1], f"{directed_name}: distance {dist}, shortest is {ref[1]}")
    need(path[0] == ref[0] and path[-1] == ref[2], f"path must run from {ref[0]} to {ref[2]}: {path}")
    s = 0
    for a, b in zip(path, path[1:]):
        need((a, b) in weight, f"no edge {a} -> {b} on the path")
        s += weight[(a, b)]
    need(s == dist, f"path edges add up to {s}, printed distance is {dist}")


def check_fastest(w, s, t, take):
    dist = dijkstra(w.n, adjacency(w.n, w.bridges, False), s)
    tok = take().split()
    need(tok[:3] == ["fastest", str(s), str(t)], f"bad prefix {tok[:3]}")
    if dist[t] == INF:
        need(tok[3:] == ["unreachable"], "expected unreachable")
        return
    weight = {}
    for a, b, wt in w.bridges:
        weight[(a, b)] = weight[(b, a)] = wt
    check_path(tok[3:], (s, dist[t], t), weight, "fastest")


def check_stamina(w, s, t, take):
    adj = adjacency(w.n, w.routes, True)
    dist = spfa(w.n, adj, s)
    tok = take().split()
    need(tok[:3] == ["stamina", str(s), str(t)], f"bad prefix {tok[:3]}")
    weight = {(a, b): wt for a, b, wt in w.routes}
    if dist is None:
        need(tok[3] == "exploit", "a negative cycle is reachable, expected 'exploit'")
        cyc = ints(tok[4:], "cycle")
        need(len(cyc) >= 3 and cyc[0] == cyc[-1], f"cycle must return to its start: {cyc}")
        total = 0
        for a, b in zip(cyc, cyc[1:]):
            need((a, b) in weight, f"no route {a} -> {b} on the cycle")
            total += weight[(a, b)]
        need(total < 0, f"cycle total is {total}, must be negative")
        need(reachable(w.n, adj, s)[cyc[0]], "cycle is not reachable from the start island")
        return
    need(tok[3:4] != ["exploit"], "no negative cycle is reachable, but 'exploit' was printed")
    if dist[t] == INF:
        need(tok[3:] == ["unreachable"], "expected unreachable")
        return
    check_path(tok[3:], (s, dist[t], t), weight, "stamina")


def check_unlock(w, q, take):
    info = quest_info(w)
    tok = take().split()
    need(tok[:2] == ["unlock", str(q)], f"bad prefix {tok[:2]}")
    if info is None:
        need(tok[2:] == ["cycle"], "prerequisites contain a cycle, expected 'cycle'")
        return
    need(tok[2:3] != ["cycle"], "there is no cycle, but 'cycle' was printed")
    fin, maxpre, preds = info
    vals = ints(tok[2:], "unlock")
    finish, chain = vals[0], vals[1:]
    need(finish == fin[q], f"finish {finish}, correct is {fin[q]}")
    need(chain and chain[-1] == q, f"chain must end at {q}: {chain}")
    need(sum(w.dur[c] for c in chain) == finish, "chain durations do not add up to the finish time")
    need(not preds[chain[0]], f"first quest {chain[0]} still has prerequisites")
    for a, b in zip(chain, chain[1:]):
        need(a in preds[b], f"{a} is not a prerequisite of {b}")
        need(fin[a] == maxpre[b], f"{a} is not a critical prerequisite of {b}")


def check_events(w, weighted, take):
    best_count, best_reward = event_refs(w)
    tok = take().split()
    name = "best" if weighted else "most"
    need(tok[0] == name, f"bad header {tok[:1]}")
    vals = ints(tok[1:], name)
    need(vals, f"{name}: missing count/reward")
    head, chosen = vals[0], vals[1:]
    need(len(set(chosen)) == len(chosen), "an event is chosen twice")
    need(all(0 <= i < len(w.events) for i in chosen), "event index out of range")
    seq = sorted(chosen, key=lambda i: w.events[i][0])
    for a, b in zip(seq, seq[1:]):
        fa, sb = w.events[a][1], w.events[b][0]
        need(sb >= fa if TOUCHING_OK else sb > fa, f"events {a} and {b} overlap")
    reward = sum(w.events[i][2] for i in chosen)
    if weighted:
        need(head == reward, f"printed reward {head}, chosen events give {reward}")
        need(reward == best_reward, f"reward {reward}, best is {best_reward}")
    else:
        need(head == len(chosen), f"printed count {head}, {len(chosen)} events listed")
        need(len(chosen) == best_count, f"{len(chosen)} events, most is {best_count}")

def verify(path, out_lines):
    pos = [0]

    def take():
        need(pos[0] < len(out_lines), "output ended too early")
        pos[0] += 1
        return out_lines[pos[0] - 1].strip()

    w, answers = None, 0
    for number, raw in enumerate(open(path, encoding="utf-8"), 1):
        line = raw.rstrip("\r\n")
        if not line or line.startswith("#"):
            continue
        f = line.split(" ")
        c = f[0]
        try:
            if c == "N":
                w = World(int(f[1]), int(f[2]))
            elif c == "B":
                w.bridges.append((int(f[1]), int(f[2]), int(f[3]))); w.cache.clear()
            elif c == "R":
                w.routes.append((int(f[1]), int(f[2]), int(f[3]))); w.cache.clear()
            elif c == "Q":
                w.dur[int(f[1])] = int(f[2]); w.cache.clear()
            elif c == "P":
                w.pre.append((int(f[1]), int(f[2]))); w.cache.clear()
            elif c == "E":
                w.events.append((int(f[1]), int(f[2]), int(f[3]))); w.cache.clear()
            else:
                if c == "regions":
                    check_regions(w, take)
                elif c == "hops":
                    check_hops(w, int(f[1]), int(f[2]), take)
                elif c == "maintain":
                    check_maintain(w, f[1], take)
                elif c == "fastest":
                    check_fastest(w, int(f[1]), int(f[2]), take)
                elif c == "stamina":
                    check_stamina(w, int(f[1]), int(f[2]), take)
                elif c == "unlock":
                    check_unlock(w, int(f[1]), take)
                elif c in ("most", "best"):
                    check_events(w, c == "best", take)
                else:
                    raise CheckError(f"unknown command {c}")
                answers += 1
        except CheckError as e:
            raise CheckError(f"line {number} `{line}`: {e}")
    need(pos[0] == len(out_lines), f"{len(out_lines) - pos[0]} extra output lines")
    return answers


def natural(p):
    return [int(t) if t.isdigit() else t for t in re.split(r"(\d+)", p)]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("files", nargs="*")
    ap.add_argument("--exe", default="isles.exe" if os.path.exists("isles.exe") else "./isles")
    ap.add_argument("--timeout", type=int, default=120)
    args = ap.parse_args()
    files = args.files or sorted(glob.glob("tests/test*.txt"), key=natural)
    failed = 0
    for path in files:
        name = os.path.basename(path)
        start = time.time()
        try:
            run = subprocess.run([args.exe, path], capture_output=True, timeout=args.timeout)
        except subprocess.TimeoutExpired:
            print(f"FAIL {name}: no answer within {args.timeout} s")
            failed += 1
            continue
        secs = time.time() - start
        if run.returncode != 0:
            print(f"FAIL {name}: exit code {run.returncode}\n     {run.stderr.decode(errors='replace').strip()}")
            failed += 1
            continue
        try:
            n = verify(path, run.stdout.decode("utf-8").splitlines())
            print(f"PASS {name}  {n} answers checked, {secs:.2f} s")
        except CheckError as e:
            print(f"FAIL {name}: {e}")
            failed += 1
    print(f"\n{len(files) - failed}/{len(files)} passed")
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
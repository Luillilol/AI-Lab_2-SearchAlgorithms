import heapq
import itertools
import time
from collections import deque
from operator import itemgetter

PERMUTATIONS = {
    "F":(0, 1, 27, 3, 4, 28, 6, 7, 29, 9, 10, 11, 12, 13, 14, 8, 5, 2, 24, 21, 18, 25, 22, 19, 26, 23, 20, 42, 39, 36, 30, 31, 32, 33, 34, 35, 15, 37, 38, 16, 40, 41, 17, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53),
    "F'":(0, 1, 17, 3, 4, 16, 6, 7, 15, 9, 10, 11, 12, 13, 14, 36, 39, 42, 20, 23, 26, 19, 22, 25, 18, 21, 24, 2, 5, 8, 30, 31, 32, 33, 34, 35, 29, 37, 38, 28, 40, 41, 27, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53),
    "B":(11, 1, 2, 10, 4, 5, 9, 7, 8, 38, 41, 44, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 0, 3, 6, 36, 37, 35, 39, 40, 34, 42, 43, 33, 51, 48, 45, 52, 49, 46, 53, 50, 47),
    "B'":(33, 1, 2, 34, 4, 5, 35, 7, 8, 6, 3, 0, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 44, 41, 38, 36, 37, 9, 39, 40, 10, 42, 43, 11, 47, 50, 53, 46, 49, 52, 45, 48, 51),
    "L":(6, 3, 0, 7, 4, 1, 8, 5, 2, 53, 10, 11, 50, 13, 14, 47, 16, 17, 9, 19, 20, 12, 22, 23, 15, 25, 26, 18, 28, 29, 21, 31, 32, 24, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 33, 48, 49, 30, 51, 52, 27),
    "L'":(2, 5, 8, 1, 4, 7, 0, 3, 6, 18, 10, 11, 21, 13, 14, 24, 16, 17, 27, 19, 20, 30, 22, 23, 33, 25, 26, 53, 28, 29, 50, 31, 32, 47, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 15, 48, 49, 12, 51, 52, 9),
    "R":(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 20, 12, 13, 23, 15, 16, 26, 18, 19, 29, 21, 22, 32, 24, 25, 35, 27, 28, 51, 30, 31, 48, 33, 34, 45, 42, 39, 36, 43, 40, 37, 44, 41, 38, 17, 46, 47, 14, 49, 50, 11, 52, 53),
    "R'":(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 51, 12, 13, 48, 15, 16, 45, 18, 19, 11, 21, 22, 14, 24, 25, 17, 27, 28, 20, 30, 31, 23, 33, 34, 26, 38, 41, 44, 37, 40, 43, 36, 39, 42, 35, 46, 47, 32, 49, 50, 29, 52, 53),
    "U":(18, 19, 20, 3, 4, 5, 6, 7, 8, 15, 12, 9, 16, 13, 10, 17, 14, 11, 36, 37, 38, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 45, 46, 47, 39, 40, 41, 42, 43, 44, 0, 1, 2, 48, 49, 50, 51, 52, 53),
    "U'":(45, 46, 47, 3, 4, 5, 6, 7, 8, 11, 14, 17, 10, 13, 16, 9, 12, 15, 0, 1, 2, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 18, 19, 20, 39, 40, 41, 42, 43, 44, 36, 37, 38, 48, 49, 50, 51, 52, 53),
    "D":(0, 1, 2, 3, 4, 5, 51, 52, 53, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 6, 7, 8, 33, 30, 27, 34, 31, 28, 35, 32, 29, 36, 37, 38, 39, 40, 41, 24, 25, 26, 45, 46, 47, 48, 49, 50, 42, 43, 44),
    "D'":(0, 1, 2, 3, 4, 5, 24, 25, 26, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 42, 43, 44, 29, 32, 35, 28, 31, 34, 27, 30, 33, 36, 37, 38, 39, 40, 41, 51, 52, 53, 45, 46, 47, 48, 49, 50, 6, 7, 8),
}

INVERSE = {
    "F": "F'", "F'": "F",
    "B": "B'", "B'": "B",
    "L": "L'", "L'": "L",
    "R": "R'", "R'": "R",
    "U": "U'", "U'": "U",
    "D": "D'", "D'": "D",
}

# Precomputed fast getters (one per move) for tuples and strings
GETTERS = {name: itemgetter(*perm) for name, perm in PERMUTATIONS.items()}


def get_neighbors(state):
    return [(name, get(state)) for name, get in GETTERS.items()]


# ----------------------------------------------------------------------
# Bidirectional BFS (your original, for comparison)
# ----------------------------------------------------------------------
def bidirectionalBFS(initial_state, final_state):
    forward_queue = deque([(initial_state, [])])
    backward_queue = deque([(final_state, [])])
    forward_visited = {initial_state: []}
    backward_visited = {final_state: []}

    if initial_state == final_state:
        return []

    while forward_queue and backward_queue:
        current, path = forward_queue.popleft()
        for move, nxt in get_neighbors(current):
            if nxt not in forward_visited:
                new_path = path + [move]
                forward_visited[nxt] = new_path
                forward_queue.append((nxt, new_path))
                if nxt in backward_visited:
                    back = backward_visited[nxt]
                    return new_path + [INVERSE[m] for m in reversed(back)]

        current, path = backward_queue.popleft()
        for move, nxt in get_neighbors(current):
            if nxt not in backward_visited:
                new_path = path + [move]
                backward_visited[nxt] = new_path
                backward_queue.append((nxt, new_path))
                if nxt in forward_visited:
                    fwd = forward_visited[nxt]
                    return fwd + [INVERSE[m] for m in reversed(new_path)]
    return None


# ----------------------------------------------------------------------
# Pattern databases
# ----------------------------------------------------------------------
def cubie_groups():
    """Group sticker indices by cubie, using the faces that move them."""
    moving = {}
    for name, perm in PERMUTATIONS.items():
        for i, p in enumerate(perm):
            if i != p:
                moving.setdefault(i, set()).add(name[0])
    groups = {}
    for i, faces in moving.items():
        groups.setdefault(frozenset(faces), []).append(i)
    corners = sorted(sorted(g) for f, g in groups.items() if len(f) == 3)
    edges = sorted(sorted(g) for f, g in groups.items() if len(f) == 2)
    assert len(corners) == 8 and len(edges) == 12, "cubie grouping failed"
    return corners, edges


def make_labeler(goal):
    """Returns label(state) -> 54-char string where every sticker is replaced by
    the index (as a character) of the solved-position sticker it came from.
    This tracks cubie identity, which is what a pattern database needs."""
    corners, edges = cubie_groups()
    groups = corners + edges
    in_group = set(i for g in groups for i in g)
    # color set of each home cubie -> (group positions, {color: home sticker idx})
    home = {}
    for g in groups:
        home[frozenset(goal[i] for i in g)] = {goal[i]: i for i in g}

    def label(state):
        lab = [None] * 54
        for i in range(54):
            if i not in in_group:
                lab[i] = chr(48 + i)          # centers never move
        for g in groups:
            h_map = home[frozenset(state[i] for i in g)]
            for i in g:
                lab[i] = chr(48 + h_map[state[i]])
        return "".join(lab)

    return label


def make_table(cubies):
    """Translation table: labels of cubies in the subset stay, all others -> '.'"""
    keep = set(chr(48 + i) for c in cubies for i in c)
    return {48 + i: (chr(48 + i) if chr(48 + i) in keep else ".")
            for i in range(54)}


def build_pdb(cubies, goal, label):
    """BFS backward from the solved cube over the masked (labelled) cube."""
    table = make_table(cubies)
    start = label(goal).translate(table)
    getters = list(GETTERS.values())
    dist = {start: 0}
    q = deque([start])
    while q:
        st = q.popleft()
        d = dist[st] + 1
        for get in getters:
            n = "".join(get(st))
            if n not in dist:
                dist[n] = d
                q.append(n)
    return table, dist


def build_all(goal):
    corners, edges = cubie_groups()
    label = make_labeler(goal)
    groups = [
        ("corners 0-3", corners[0:4]),
        ("corners 4-7", corners[4:8]),
        ("edges 0-3", edges[0:4]),
        ("edges 4-7", edges[4:8]),
        ("edges 8-11", edges[8:12]),
    ]
    pdbs = []
    for name, g in groups:
        t = time.time()
        table, dist = build_pdb(g, goal, label)
        print(f"  PDB {name}: {len(dist):,} states, max depth {max(dist.values())}, "
              f"{time.time() - t:.1f}s")
        pdbs.append((table, dist))
    return pdbs, label


def make_heuristic(pdbs, label):
    def h(state):
        lab = label(state)
        best = 0
        for table, dist in pdbs:
            d = dist[lab.translate(table)]
            if d > best:
                best = d
        return best

    return h


# ----------------------------------------------------------------------
# A*
# ----------------------------------------------------------------------
def astar(initial, goal, h):
    counter = itertools.count()
    open_heap = [(h(initial), next(counter), 0, initial, None)]
    best_g = {initial: 0}
    parent = {initial: (None, None)}
    expanded = 0

    while open_heap:
        f, _, g, state, last = heapq.heappop(open_heap)

        if state == goal:
            path = []
            while parent[state][0] is not None:
                state, m = parent[state]
                path.append(m)
            return path[::-1], expanded

        if g > best_g[state]:
            continue  # stale heap entry

        expanded += 1
        for move, nxt in get_neighbors(state):
            if last is not None and move == INVERSE[last]:
                continue  # don't undo the previous move
            ng = g + 1
            if ng < best_g.get(nxt, float("inf")):
                best_g[nxt] = ng
                parent[nxt] = (state, move)
                heapq.heappush(open_heap, (ng + h(nxt), next(counter), ng, nxt, move))
    return None, expanded


# ----------------------------------------------------------------------
# Main
# ----------------------------------------------------------------------
def apply_moves(state, moves):
    for m in moves:
        state = GETTERS[m](state)
    return state


def validate(state):
    from collections import Counter
    c = Counter(state)
    assert len(state) == 54 and all(c[x] == 9 for x in "BRWOGY"), \
        f"invalid cube, color counts: {dict(c)}"


def main():
    goal_state = tuple("B" * 9 + "R" * 9 + "W" * 9 + "O" * 9 + "G" * 9 + "Y" * 9)

    # Build the scramble by applying moves to the solved cube (avoids typos).
    scramble = ["F", "U", "B", "F'", "R", "U", "L", "D", "R", "B'", "R", "U'", "F"]
    initial_state = apply_moves(goal_state, scramble)

    # Or paste your own 54 stickers here (must have exactly 9 of each color):
    # initial_state = ("Y", "R", ...)
    # validate(initial_state)

    print("Building pattern databases...")
    t = time.time()
    pdbs, label = build_all(goal_state)
    h = make_heuristic(pdbs, label)
    print(f"PDBs ready in {time.time() - t:.1f}s")
    print(f"h(initial) = {h(initial_state)}")

    print("\nRunning A*...")
    t = time.time()
    path, expanded = astar(initial_state, goal_state, h)
    print(f"Solution ({len(path)} moves): {path}")
    print(f"Expanded nodes: {expanded:,}   time: {time.time() - t:.1f}s")

    # Verify the solution actually solves the cube
    assert apply_moves(initial_state, path) == goal_state, "solution is wrong!"
    print("Verified: solution reaches the goal.")

    # Uncomment to compare with your bidirectional search
    # t = time.time()
    # bi = bidirectionalBFS(initial_state, goal_state)
    # print(f"\nBidirectional: {len(bi)} moves in {time.time() - t:.1f}s")


if __name__ == "__main__":
    main()
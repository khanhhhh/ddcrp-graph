import networkx as nx
import random
from typing import List


def random_walk(g: nx.Graph, walks_per_node: int = 1, walk_length: int = 1) -> List[List[int]]:
    walks: List[List[int]] = []
    for start in g.nodes():
        for _ in range(walks_per_node):
            path: List[int] = [start, ]
            for _ in range(walk_length):
                neighbors = list(g.neighbors(path[-1]))
                if len(neighbors) == 0:
                    break
                next = random.choice(neighbors)
                path.append(next)
            walks.append(path)
    return walks

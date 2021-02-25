"""Tiny graph library for use with edge computing Mininet extensions"""

def spt(graph, source):
    "Compute the shortest-path tree to source"

    Q = []
    dist = {}
    prev = {}
    for v in graph.keys():
        Q.append(v)
        dist[v] = 100000
        prev[v] = None

    if source not in dist:
        raise Exception('Cannot compute the path to node {} that is not in {}'.format(source, Q))

    dist[source] = 0

    while len(Q) > 0:
        u = None
        last_value = None
        for node in Q:
            value = dist[node]
            if not u or value < last_value:
                u = node
                last_value = value
        Q.remove(u)

        for v in Q:
            if v not in graph[u]:
                continue
            # v is in Q and a neighbor of u
            alt = dist[u] + 1
            if alt < dist[v]:
                dist[v] = alt
                prev[v] = u

    return (prev, dist)

def traversing(tree, src, dst):
    "Return the nodes traversed from src to dst according to the given SPT"

    ret = []
    nxt = src

    while nxt != dst:
        nxt = tree[nxt]
        if nxt != dst:
            ret.append(nxt)

    return ret


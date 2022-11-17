#!/usr/bin/env python3

# network costs per data unit
netcost = {"cloud": 12, "edge-near": 6, "edge-far": 2}

# storage access cost per data unit
storage = {"cloud": 0, "edge-near": 10, "edge-far": 10}

# r_k, m_k, d_k
classes = [[1, 100, 10], [10, 1, 0]]

for k, curclass in zip(range(len(classes)), classes):
    assert len(curclass) == 3
    r_k = curclass[0]
    m_k = curclass[1]
    d_k = curclass[2]

    print(f"application type#{k} (r_k = {r_k})")
    for tier, c in netcost.items():
        assert tier in storage
        cost_mu = c * r_k * m_k
        cost_lambda = c * m_k + storage[tier] * d_k
        print(f"\t{tier} ->\tcost_mu {cost_mu}\tcost_lambda {cost_lambda}")

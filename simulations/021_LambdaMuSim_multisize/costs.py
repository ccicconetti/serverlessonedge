#!/usr/bin/env python3

# network costs per data unit
netcost = {"cloud": 12, "edge-near": 6, "edge-far": 2}

# storage access cost per data unit
storage = {"cloud": 0, "edge-near": 10, "edge-far": 10}


def costs(tier: str, params):
    assert len(params) == 3
    r_k = params[0]
    m_k = params[1]
    d_k = params[2]

    assert tier in storage
    cost_mu = c * r_k * m_k
    cost_lambda = c * m_k + storage[tier] * d_k

    return [cost_mu, cost_lambda]


# r_k, m_k, d_k
param_ref = [5, 100, 10]


for tier, c in netcost.items():
    costs_ref = costs(tier, param_ref)
    print(f"{tier} ->\tcost_mu_ref {costs_ref[0]}")
    for m_k_other in range(0, 101, 10):
        line = ""
        for d_k_other in range(0, 11, 1):
            params_cur = [param_ref[0], m_k_other, d_k_other]
            line += "\t" + str(costs(tier, params_cur)[0])
        print(line)

for tier, c in netcost.items():
    print(f"{tier} ->\tcost_lambda_ref {costs_ref[1]}")
    for m_k_other in range(0, 101, 10):
        line = ""
        for d_k_other in range(0, 11, 1):
            params_cur = [param_ref[0], m_k_other, d_k_other]
            line += "\t" + str(costs(tier, params_cur)[1])
        print(line)

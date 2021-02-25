import json

def makeSingleCpuComputerJson(outfile, cpu_name, speed, memory, lambdas, num_workers):
    "Save to file an edge computer template"

    computer_file = open(outfile, 'w')

    processor = dict()
    processor['name'] = cpu_name
    processor['type'] = 'GenericCpu'
    processor['speed'] = speed
    processor['memory'] = memory

    computer = dict()
    computer['version'] = '1.0'
    computer['processors'] = [ processor ]
    computer['containers'] = [ ]

    for i in lambdas:
        jlambda = dict()
        jlambda['name'] = 'clambda{}'.format(i)
        jlambda['requirements'] = 'proportional'
        jlambda['op-coeff'] = 1e6
        jlambda['op-offset'] = 4e6
        jlambda['mem-coeff'] = 100
        jlambda['mem-offset'] = 0

        container = dict()
        container['name'] = 'ccontainer{}'.format(i)
        container['processor'] = cpu_name
        container['lambda'] = jlambda
        container['num-workers'] = int(num_workers)

        computer['containers'].append(container)

    computer_file.write(json.dumps(computer))

    computer_file.close()

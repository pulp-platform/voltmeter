#!/usr/bin/env python3

# Copyright 2023 ETH Zurich and University of Bologna.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0
#
# Author: Sergio Mazzola, ETH Zurich <smazzola@iis.ee.ethz.ch>

import os
import yaml
import copy
from cerberus import Validator
import pprint

if __name__ == '__main__':
    # input
    CONFIG_YML = os.getenv('VOLTMETER_YML')
    # outputs
    OUTPUT_MK = os.getenv('VOLTMETER_MK')

    if CONFIG_YML is None:
        raise Exception('VOLTMETER_YML not set')
    if OUTPUT_MK is None:
        raise Exception('VOLTMETER_MK not set')

    ############################
    # parse voltmeter.yml
    ############################

    # get config from YAML file
    with open(CONFIG_YML, 'r') as f:
        config_yml = yaml.safe_load(f)
    # verify config syntax with cerberus
    schema = eval(open(os.path.join(os.path.dirname(__file__), 'yml_schema.py'), 'r').read())
    v = Validator(schema)
    if not v.validate(config_yml):
        raise Exception('Invalid {}: \n'.format(CONFIG_YML) + pprint.pformat(v.errors))

    ############################
    # process configuration
    ############################

    config_yml = v.normalized(config_yml)
    # remove empty arguments
    config = copy.deepcopy(config_yml)
    for key in config_yml['arguments']:
        continue
    # convert paths to absolute
    for key in config['arguments']:
        if key in ['config_cpu', 'config_gpu', 'trace_dir']:
            config['arguments'][key] = os.path.abspath(config['arguments'][key])
    # process benchmarks
    temp_bench = copy.deepcopy(config['arguments']['benchmarks'])
    for item, item_copy in zip(config['arguments']['benchmarks'], temp_bench):
        # delete None arguments, convert the others to string
        for key in item_copy:
            if item[key] is None:
                del item[key]
            else:
                item[key] = str(item[key])
        # convert paths to absolute
        item['path'] = os.path.abspath(item['path'])
        # process benchmark arguments
        temp_args = copy.deepcopy(item['args'])
        if 'args' in item:
            item['args'] = ''
            # check that arguments string does not contain commas
            if ',' in temp_args:
                raise Exception('Invalid {}: benchmark arguments must not contain \',\''.format(CONFIG_YML))
            # normalize spaces
            temp_args = temp_args.replace('\t', ' ')
            temp_args = ' '.join(temp_args.split())
            # convert arguments to comma-separated values (also handle quotes "" or '')
            quotes_found = False
            for c in temp_args:
                if c == '"' or c == "'":
                    quotes_found = not quotes_found
                    continue
                if c == ' ' and not quotes_found:
                    item['args'] += ','
                else:
                    item['args'] += c
            if quotes_found:
                raise Exception('Invalid {}: unbalanced quotes in benchmark arguments'.format(CONFIG_YML))

    ############################
    # generate makefrag
    ############################

    # each couple lib_path+arguments must be recognized as a single element by the shell so
    # that they refer to only 1 voltemter call; at the same time, also the benchmark arguments
    # are to be passed as a unique string to the flag --benchmark_args
    with open(OUTPUT_MK, 'w') as f:
        f.write('# This file is automatically generated by parse_config.py\n\n')
        f.write('-include ./config.mk\n\n')
        f.write('# Voltmeter compilation parameter\n')
        for key in [k for k in config if k != 'arguments']:
            for param in config[key]:
                f.write('{} := {}\n'.format(param, int(config[key][param]) if type(config[key][param]) is bool else config[key][param]))
        f.write('\n')
        f.write('# Voltmeter CLI arguments\n')
        for key in config['arguments']:
            # benchmarks
            if key == 'benchmarks':
                # passed as a string to the shell (i.e., surrounded by ''); a shell 'for' loop can
                # differentiate the different iterable elements thanks to the character '\n'; the
                # '\\\n' are only used to make the voltmeter.mk more readable
                f.write('BENCHMARKS := $$\' \\\n')
                for b in config['arguments']['benchmarks'][:-1]:
                    f.write('\t--benchmark={}'.format(b['path']))
                    if b['args'] is not None:
                        f.write(' --benchmark_args={}'.format(b['args']))
                    f.write('\\n \\\n')
                # last element
                b = config['arguments']['benchmarks'][-1]
                f.write('\t--benchmark={}'.format(b['path']))
                if b['args'] is not None:
                    f.write(' --benchmark_args={}'.format(b['args']))
                f.write(' \\\n\'\n')
            # command-line events
            elif key[:4] == 'cli_':
                f.write('VOLTMETER_ARGS += --{}={}\n'.format(key, ','.join(map(str, config['arguments'][key]))))
            # other arguments
            else:
                f.write('VOLTMETER_ARGS += --{}={}\n'.format(key, config['arguments'][key]))

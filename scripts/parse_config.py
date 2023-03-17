#!/usr/bin/env python3

# Copyright 2023 ETH Zurich and University of Bologna.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0
#
# Author: Sergio Mazzola, ETH Zurich <smazzola@iis.ee.ethz.ch>

import os
import yaml

# define expected YAML syntax
config_syntax = {'parameters':{}, 'benchmarks':[{'name':True, 'libs':None, 'path':True, 'args':None}]}

if __name__ == '__main__':
    # input
    CONFIG_YML = os.getenv('VOLTMETER_YML')
    # outputs
    OUTPUT_MK = os.getenv('VOLTMETER_MK')
    BENCHMARKS_SH = os.getenv('BENCHMARKS_SH')

    if CONFIG_YML is None:
        raise Exception('VOLTMETER_YML not set')
    if OUTPUT_MK is None:
        raise Exception('VOLTMETER_MK not set')
    if BENCHMARKS_SH is None:
        raise Exception('BENCHMARKS_SH not set')

    # get config from YAML file
    with open(CONFIG_YML, 'r') as f:
        config = yaml.safe_load(f)
    # verify config syntax (top-level)
    for key in config_syntax:
        if key not in config:
            raise Exception('Invalid {}: key \'{}\' not found'.format(CONFIG_YML, str(key)))
        if type(config[key]) is not type(config_syntax[key]):
            raise Exception('Invalid {}: key \'{}\' must be of type \'{}\''.format(CONFIG_YML, str(key), str(type(config_syntax[key]))))
    # verify config syntax (benchmarks list)
    for item in config['benchmarks']:
        for key in config_syntax['benchmarks'][0]:
            if (key not in item) and (config_syntax['benchmarks'][0][key] is True):
                # only complain for not found key if it is a required key (i.e., True)
                raise Exception('Invalid {}: key \'{}\' not found'.format(CONFIG_YML, str(key)))
            if (key in item) and (type(item[key]) is not str) and (item[key] is not None):
                raise Exception('Invalid {}: key \'{}\' must be of type \'{}\' or empty'.format(CONFIG_YML, str(key), str(type(config_syntax['benchmarks'][0][key]))))
    # check if there are no unexpected keys
    for key in config:
        if key not in config_syntax:
            raise Exception('Invalid {}: key {} not expected'.format(CONFIG_YML, str(key)))

    # process config
    for b in config['benchmarks']:
        for key in config_syntax['benchmarks'][0]:
            # if benchmark key is not found and it is not required, set it to None
            if (key not in b) and (config_syntax['benchmarks'][0][key] is None):
                b[key] = None
        b['path'] = os.path.abspath(b['path'])

    # collect all libraries used by benchmarks
    libs = set()
    for b in config['benchmarks']:
        libs = libs.union(b['libs'].split(' ') if b['libs'] is not None else [])
    # generate makefrag
    with open(OUTPUT_MK, 'w') as f:
        for key in config['parameters']:
            f.write('{} := {}\n'.format(key, config['parameters'][key]))
        f.write('BENCHMARK_LIBS := {}\n'.format(' '.join(libs)))
    # generate script to launch benchmarks
    #TODO: Does not really need the script, but shared library path + arguments
    with open(BENCHMARKS_SH, 'w') as f:
        f.write('#!/bin/bash\n')
        for b in config['benchmarks']:
            f.write('{} {}\n'.format(b['path'], b['args'] if b['args'] is not None else ''))

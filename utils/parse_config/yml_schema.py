# Copyright 2023 ETH Zurich and University of Bologna.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0
#
# Author: Sergio Mazzola, ETH Zurich <smazzola@iis.ee.ethz.ch>

# Schema to validate Voltmeter YML manifest file config
{
    'param-platform': {
        'required': True,
        'type': 'dict',
        'schema': {
            'platform': {
                'required': True,
                'type': 'string',
                'allowed': ['jetson_agx_xavier']
            },
            'profile_cpu': {
                'required': True,
                'anyof': [
                    {'dependencies': {'profile_cpu': False}},
                    {'allof': [
                        {'dependencies': {'profile_cpu': True}},
                        {'dependencies': ['frequencies_cpu']}
                    ]}
                ],
                'type': 'boolean',
                'default': False
            },
            'profile_gpu': {
                'required': True,
                'anyof': [
                    {'dependencies': {'profile_gpu': False}},
                    {'allof': [
                        {'dependencies': {'profile_gpu': True}},
                        {'dependencies': ['frequencies_gpu']}
                    ]}
                ],
                'type': 'boolean',
                'default': False
            },
            'frequencies_cpu': {
                'dependencies': {'profile_cpu': True},
                'type': 'list',
                'nullable': False,
                'empty': False,
                'schema': {
                    'type': 'integer',
                    'min': 1
                }
            },
            'frequencies_gpu': {
                'dependencies': {'profile_gpu': True},
                'type': 'list',
                'nullable': False,
                'empty': False,
                'schema': {
                    'type': 'integer',
                    'min': 1
                }
            }
        }
    },
    'param-profiler': {
        'required': True,
        'type': 'dict',
        'schema': {
            'num_run': {
                'required': True,
                'type': 'integer',
                'default': 3,
                'min': 1
            },
            'sample_period_us': {
                'required': True,
                'type': 'integer',
                'default': 100000,
                'min': 1
            }
        }
    },
    'arguments': {
        'required': True,
        'type': 'dict',
        'schema': {
            'events': {
                'required': True,
                'anyof': [
                    {'dependencies': {'events': 'all_events'}},
                    {'allof': [
                        {'dependencies': {'events': 'config'}},
                        {'anyof': [ # strong check left to voltemter argparse
                            {'dependencies': ['config_cpu']},
                            {'dependencies': ['config_gpu']},
                        ]},
                    ]},
                    {'allof': [
                        {'dependencies': {'events': 'cli'}},
                        {'anyof': [ # strong check left to voltemter argparse
                            {'dependencies': ['cli_cpu']},
                            {'dependencies': ['cli_gpu']},
                        ]},
                    ]},
                ],
                'type': 'string',
                'allowed': ['all_events', 'config', 'cli']
            },
            'config_cpu': {
                'dependencies': {'events': 'config'},
                'type': 'string',
                'nullable': False,
            },
            'config_gpu': {
                'dependencies': {'events': 'config'},
                'type': 'string',
                'nullable': False,
            },
            'cli_cpu': {
                'dependencies': {'events': 'cli'},
                'type': 'list',
                'nullable': False,
                'empty': False,
                'schema': {
                    'type': 'integer',
                    'min': 0
                }
            },
            'cli_gpu': {
                'dependencies': {'events': 'cli'},
                'type': 'list',
                'nullable': False,
                'empty': False,
                'schema': {
                    'type': 'integer',
                    'min': 0
                }
            },
            'mode': {
                'required': True,
                'type': 'string',
                'allowed': ['characterization', 'profile', 'num_passes']
            },
            'trace_dir': {
                'required': True,
                'type': 'string'
            },
            'benchmarks': {
                'required': True,
                'noneof': [{'dependencies': {'mode': 'num_passes'}}],
                'type': 'list',
                'schema': {
                    'type': 'dict',
                    'schema': {
                        'name': {
                            'required': False,
                            'type': 'string'
                        },
                        'path': {
                            'required': True,
                            'type': 'string'
                        },
                        'args': {
                            'required': False,
                            'nullable': True
                        }
                    }
                }
            }
        }
    }
}

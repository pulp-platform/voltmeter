# Copyright 2023 ETH Zurich and University of Bologna.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0
#
# Author: Sergio Mazzola, ETH Zurich <smazzola@iis.ee.ethz.ch>

# Schema to validate config/voltmeter.yml config
{
    'param-platform': {
        'required': True,
        'type': 'dict',
        'schema': {
            'PLATFORM': {
                'required': True,
                'type': 'string',
                'allowed': ['jetson_agx_xavier']
            },
            'PROFILE_CPU': {
                'required': True,
                'type': 'boolean',
                'default': False
            },
            'PROFILE_GPU': {
                'required': True,
                'type': 'boolean',
                'default': False
            }
        }
    },
    'param-profiler': {
        'required': True,
        'type': 'dict',
        'schema': {
            'NUM_RUN': {
                'required': True,
                'type': 'integer',
                'default': 3,
                'min': 1
            },
            'SAMPLE_PERIOD_US': {
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
                'type': 'string',
                'allowed': ['all_events', 'config', 'cli']
            },
            'config_cpu': {
                'dependencies': {'events': 'config'},
                'type': 'string'
            },
            'config_gpu': {
                'dependencies': {'events': 'config'},
                'type': 'string'
            },
            'cli_cpu': {
                'dependencies': {'events': 'cli'},
                'type': 'list',
                'schema': {
                    'type': 'integer',
                    'min': 0
                }
            },
            'cli_gpu': {
                'dependencies': {'events': 'cli'},
                'type': 'list',
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
                'type': 'list',
                'schema': {
                    'type': 'dict',
                    'schema': {
                        'name': {
                            'required': True,
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

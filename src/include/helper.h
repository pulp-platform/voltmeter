// Copyright 2023 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Author: Sergio Mazzola, ETH Zurich <smazzola@iis.ee.ethz.ch>

#ifndef _HELPER_H
#define _HELPER_H

#include <jsmn.h>

int cat_path(char *path, char *filename, char *full_path);
char *path_basename(char *path);
int jsmn_parse_token(const char *json_string, jsmntok_t *tok, const char *format, ...);

#endif // _HELPER_H

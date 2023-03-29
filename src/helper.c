// Copyright 2023 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Author: Sergio Mazzola, ETH Zurich <smazzola@iis.ee.ethz.ch>

// standard includes
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
// voltmeter libraries
#include <helper.h>
// third-party libraries
#include <jsmn.h>

// concatenate a path with a filename
int cat_path(char *path, char *filename, char *full_path) {
  char format_string[20];
  if (path[strlen(path) - 1] == '/')
    sprintf(format_string, "%%s%%s");
  else
    sprintf(format_string, "%%s/%%s");
  return sprintf(full_path, format_string, path, filename);
}

// strip path from filename and remove extension, if any
char *path_basename(char *path) {
  char *base = strrchr(path, '/');
  if (base == NULL)
    base = path;
  else
    base++;
  char *ext = strrchr(base, '.');
  if (ext != NULL)
    *ext = '\0';
  return base;
}

// parse a token from a json string into a variable (based on jsmn library)
int jsmn_parse_token(const char *json_string, jsmntok_t *tok, const char *format, ...) {
  char temp[200];
  va_list args;
  va_start(args, format);
  sprintf(temp, "%.*s", tok->end - tok->start, json_string + tok->start);
  int ret = vsscanf(temp, format, args);
  va_end(args);
  return ret;
}

// both print to stdout and to a file
int printf_file(FILE *file, const char *format, ...) {
  va_list args;
  va_start(args, format);
  int ret = vfprintf(stdout, format, args);
  va_end(args);
  va_start(args, format);
  ret += vfprintf(file, format, args);
  va_end(args);
  return ret;
}

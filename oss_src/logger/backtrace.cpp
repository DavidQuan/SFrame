/**
 * Copyright (C) 2016 Turi
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 */
/**
 * Copyright (c) 2009 Carnegie Mellon University.
 *     All rights reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an "AS
 *  IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 *  express or implied.  See the License for the specific language
 *  governing permissions and limitations under the License.
 *
 * For more about this software visit:
 *
 *      http://www.graphlab.ml.cmu.edu
 *
 */


#ifndef _WIN32
#include <execinfo.h>
#endif
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <cxxabi.h>
#include <parallel/pthread_tools.hpp>

/** Code from http://mykospark.net/2009/09/runtime-backtrace-in-c-with-name-demangling/ */
std::string demangle(const char* symbol) {
  size_t size;
  int status;
  char temp[1024];
  char* demangled;
  //first, try to demangle a c++ name
  if (1 == sscanf(symbol, "%*[^(]%*[^_]%127[^)+]", temp)) {
    if (NULL != (demangled = abi::__cxa_demangle(temp, NULL, &size, &status))) {
      std::string result(demangled);
      free(demangled);
      return result;
    }
  }
  //if that didn't work, try to get a regular c symbol
  if (1 == sscanf(symbol, "%127s", temp)) {
    return temp;
  }

  //if all else fails, just return the symbol
  return symbol;
}


static graphlab::mutex back_trace_file_lock;
static size_t write_count = 0;
static bool write_error = 0;
static int backtrace_file_number = 0;


extern void __set_back_trace_file_number(int number) {
  backtrace_file_number = number;
}

/* Obtain a backtrace and print it to ofile. */
void __print_back_trace() {
//TODO: Implement this functionality in Windows.
// Available through either StackWalk64 or CaptureStackBackTrace
#ifndef _WIN32
    void    *array[1024];
    size_t  size, i;
    char    **strings;

    try {
      std::lock_guard<graphlab::mutex> guard(back_trace_file_lock);

      if (write_error) {
        return;
      }
      char filename[1024];
      sprintf(filename, "backtrace.%d", backtrace_file_number);

      FILE* ofile = NULL;
      if (write_count == 0) {
        ofile = fopen(filename, "w");
      }
      else {
        ofile = fopen(filename, "a");
      }
      // if unable to open the file for output
      if (ofile == NULL) {
        // print an error, set the error flag so we don't ever print it again
        fprintf(stderr, "Unable to open output backtrace file.\n");
        write_error = 1;
        return;
      }
      ++write_count;

      size = backtrace(array, 1024);
      strings = backtrace_symbols(array, size);

      fprintf(ofile, "Pointers\n");
      fprintf(ofile, "------------\n");
      for (i = 0; i < size; ++i) {
          fprintf(ofile, "%p\n", array[i]);
      }


      fprintf(ofile, "Raw\n");
      fprintf(ofile, "------------\n");
      for (i = 0; i < size; ++i) {
          fprintf(ofile, "%s\n", strings[i]);
      }
      fprintf(ofile, "\nDemangled\n");
      fprintf(ofile, "------------\n");

      for (i = 0; i < size; ++i) {
          std::string ret = demangle(strings[i]);
          fprintf(ofile, "%s\n", ret.c_str());
      }
      free(strings);

      fprintf(ofile, "-------------------------------------------------------\n");
      fprintf(ofile, "\n\n");

      fclose(ofile);
    } catch (...) {
      std::cerr << "Unable to print back trace on termination" << std::endl;
    }
#endif
}


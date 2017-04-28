/* -*-mode:c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#include <getopt.h>

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <chrono>

#include "ivf.hh"
#include "yuv4mpeg.hh"
#include "frame.hh"
#include "decoder.hh"
#include "encoder.hh"

using namespace std;

void usage_error( const string & program_name )
{
  cerr << program_name << " INPUT1 INPUT2" << endl;
}

int main( int argc, char *argv[] )
{
  try {
    if ( argc <= 0 ) {
      abort();
    }

    if ( argc < 3 ) {
      usage_error( argv[ 0 ] );
      return EXIT_FAILURE;
    }

  }
  catch ( const exception &  e ) {
    print_exception( argv[ 0 ], e );
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

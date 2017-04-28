/* -*-mode:c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#include <getopt.h>

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <chrono>
#include <vector>

#include "ivf.hh"
#include "yuv4mpeg.hh"
#include "frame.hh"
#include "decoder.hh"
#include "encoder.hh"
#include "uncompressed_chunk.hh"
#include "vp8_raster.hh"

using namespace std;

void usage_error( const string & program_name )
{
  cerr << program_name << " FROM TO" << endl;
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

    IVF ivf[] = { { argv[ 1 ] }, { argv[ 2 ] } };

    uint16_t width = ivf[ 0 ].width();
    uint16_t height = ivf[ 0 ].height();

    vector<RasterHandle> outputs[ 2 ];
    vector<pair<Optional<KeyFrame>, Optional<InterFrame> > > frames[ 2 ];

    Decoder decoder[] = { { width, height }, { width, height } };

    assert( ivf[0].frame_count() == ivf[1].frame_count() );

    for ( size_t frame_number = 0; frame_number < ivf[0].frame_count(); frame_number++ ) {
      UncompressedChunk uncompressed_chunk[] =
       {
         { ivf[ 0 ].frame( frame_number ), width, height, false },
         { ivf[ 1 ].frame( frame_number ), width, height, false }
       };

       for ( size_t i = 0; i < 2; i++ ) {
         if ( uncompressed_chunk[ i ].key_frame() ) {
           KeyFrame frame = decoder[ i ].parse_frame<KeyFrame>( uncompressed_chunk[ i ] );
           pair<bool, RasterHandle> output = decoder[ i ].decode_frame( frame );

           frames[ i ].emplace_back( move( frame ), Optional<InterFrame>() );
           outputs[ i ].push_back( output.second );
         }
         else {
           InterFrame frame = decoder[ i ].parse_frame<InterFrame>( uncompressed_chunk[ i ] );
           pair<bool, RasterHandle> output = decoder[ i ].decode_frame( frame );

           frames[ i ].emplace_back( Optional<KeyFrame>(), move( frame ) );
           outputs[ i ].push_back( output.second );
         }
       }
    }


  }
  catch ( const exception &  e ) {
    print_exception( argv[ 0 ], e );
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

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
  cerr << program_name << " FROM TO OUTPUT" << endl;
}

uint8_t get_qi( uint8_t low, uint8_t high, size_t index, size_t total )
{
  double alpha = 1.0 * index / ( total - 1 );

  if ( alpha > 0.99 ) alpha = 1.0;

  return high * alpha + low * ( 1 - alpha );
}

int main( int argc, char *argv[] )
{
  try {
    if ( argc <= 0 ) {
      abort();
    }

    if ( argc < 4 ) {
      usage_error( argv[ 0 ] );
      return EXIT_FAILURE;
    }

    string output_name = argv[ 3 ];

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

    //vector<pair<Optional<KeyFrame>, Optional<InterFrame> > > result;

    size_t switch_start = 1;
    size_t switch_length = 12;
    size_t switch_index = 0;

    IVFWriter out_writer { output_name, "VP80", width, height, 1, 1 };

    Encoder encoder { width, height, false, BEST_QUALITY };

    for ( size_t frame_no = 0; frame_no < switch_start + switch_length; frame_no++ ) {
      if ( frame_no < switch_start ) {
        out_writer.append_frame( encoder.write_frame( frames[ 0 ][ frame_no ].first.get() ) );
        continue;
      }

      uint16_t q[] = {
        frames[ 0 ][ frame_no ].second.get().header().quant_indices.y_ac_qi,
        frames[ 1 ][ frame_no ].second.get().header().quant_indices.y_ac_qi
      };

      QuantIndices new_qi;
      new_qi.y_ac_qi = get_qi( q[ 0 ], q[ 1 ], switch_index, switch_length );

      out_writer.append_frame( encoder.write_frame( encoder.update_residues( outputs[ 1 ][ frame_no ],
                                                    frames[ 1 ][ frame_no ].second.get(),
                                                    new_qi, false ) ) );


      switch_index++;
    }
  }
  catch ( const exception &  e ) {
    print_exception( argv[ 0 ], e );
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

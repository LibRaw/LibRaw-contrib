/**
  * This program converts a raw image-(such as canon's cr2 or nikon's nef) to
  * a tiff image. It accomplishes this using the libraw library available at
  * www.libraw.org. It emulates the dcraw -D -4 -T command. It has only been
  * tested using canon CR2 files.
  *
  * In addition to writing out a tiff image it also allows the user to specify
  * that only a region of the captured raw image be inserted into the tiff 
  * image. In this way it emulates the dcraw_emu code which has crop box option.
  * This program is free software: you can use, modify and/or
  * redistribute it under the terms of the simplified BSD License.

Author: West Suhanic <west.suhanic@gmail.com>

Copyright (c) 2012, West Suhanic, gDial Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

    Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    Neither the name of the gDial Inc. nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 *
 * A copy of the bsd license is available at:
 * http://www.opensource.org/licenses/bsd-license.html.
***/

/**
  * This code has been tested against LibRaw-0.15.0-Beta2. It also needs
  * libtiff v4.0 or greater.
  *
  * To compile the code do:
  *
g++ -D_FILE_OFFSET_BITS=64 -Wall -Wextra -Wno-unused -Wno-parentheses -Wno-unknown-pragmas -g -c -fno-strict-aliasing -fPIC -fno-omit-frame-pointer -I/install_dir/libs/libraw/v0150/include -I/install_dir/libs/tiff/v400/include raw2tiff.cc
  *
  * To link the code do:
  *
  * g++ raw2tiff.o -o raw2tiff /install_dir/libs/libraw/v0150/lib/libraw.so /install_dir/libs/tiff/v400/lib/libtiff.so
  *
  * To get a region of a target.cr2 image do the following at the command
  * line:
  *
  * >./raw2tiff source.cr2 ./x4.tif yes 0 1900 5202 400
  *
  * This will take the image source.cr2 and generate a tif image-(x4.tif)
  * that has 400 rows and 5202 columns. The image-(x4.tif)-contains image
  * data from source.cr2 starting at row 1900 and column position 0.
  */
#include <algorithm>
#include <fstream>
#include <limits>
#include <iostream>
#include <sstream>
#include <vector>

#include <string.h>

#include "libraw/libraw.h"
#include "tiffio.h"

/**
  * Needed constant values.
  */
const char FWD_SLASH	= '/';

const std::string YES	= "yes";
const std::string NO	= "no";

const unsigned short COL_NUMBER_START	= 0;
const unsigned short ROW_NUMBER_START	= 1;
const unsigned short NUMBER_OF_COLS	= 2;
const unsigned short NUMBER_OF_ROWS	= 3;

/**
  * This class is used to convert a number held in a string
  * into its actual type like an int or a double.
  *
  * Author: West.Suhanic, Nov.20.2012. Code written in Toronto,Canada.
  */
class stringConverter
{
	private:

		const std::string EMPTY;

	public:

		stringConverter(): EMPTY(""){}

		/**
		  * This methods initializes the stream.
		  *
		  * Author: West.Suhanic, Nov.20.2012.
		  * Code written in Toronto,Canada.
		  */
		template< typename TYPE1 > void streamInitialize( TYPE1& ss ) const
		{
			ss.str( EMPTY );
			ss.clear();
			return;
		}


		/**
		  * This methods converts the value held in the
		  * string into the specified type.
		  *
		  * Author: West.Suhanic, Nov.20.2012.
		  * Code written in Toronto,Canada.
		  */
		template<typename TYPE1, typename TYPE2> int convertTheString( TYPE1& ss, const std::string& strValue, TYPE2& value )
		{
		const std::string method = "convertTheString";

			/**
			  * Make sure we have been passed
			  * something useable.
			  */
			if( strValue.length() == 0 )
			{
				std::cerr << method << " cannot proceed. The string value is not specified. " << std::endl;
				return -1;
			}

			/**
			  * Initialize the converter stream.
			  */
			this->streamInitialize( ss );

			/**
			  * Attempt the conversion.
			  */
			ss.str( strValue );
			ss >> value;
			if( ss.bad() == true )
			{
				std::cerr << method << " failed to convert string value: " << strValue << std::endl;
				return -1;
			}

		return 0;
		}
};

/**
  * Display some information about the raw image.
  *
  * Author: West.Suhanic, Dec.12.2012. Code written in Toronto,Canada.
  */
void getImageInformation( const LibRaw& RawProcessor )
{

	/**
	  * Print out some image information.
	  */
	std::cerr << "Raw Image Width-(number of columns): " << RawProcessor.imgdata.sizes.raw_width << std::endl;
	std::cerr << "Raw Image Height-(number of rows): " << RawProcessor.imgdata.sizes.raw_height << std::endl;
	std::cerr << "Image Width-(number of columns): " << RawProcessor.imgdata.sizes.width << std::endl;
	std::cerr << "Image Height-(number of rows): " << RawProcessor.imgdata.sizes.height << std::endl;
	std::cerr << "Margin, top: " << RawProcessor.imgdata.sizes.top_margin << std::endl;
	std::cerr << "Margin, left: " << RawProcessor.imgdata.sizes.left_margin << std::endl;
	std::cerr << "Pixel Aspect Ratio: " << RawProcessor.imgdata.sizes.pixel_aspect << std::endl;
	std::cerr << "Colors: " << RawProcessor.imgdata.idata.colors << std::endl;
	std::cerr << "Make: " << RawProcessor.imgdata.idata.make << std::endl;
	std::cerr << "Model: " << RawProcessor.imgdata.idata.model << std::endl;
	std::cerr << "Aperture: " << RawProcessor.imgdata.other.aperture << std::endl;
	std::cerr << "Focal Length-(mm): " << RawProcessor.imgdata.other.focal_len << std::endl;
	std::cerr << "ISO Speed: " << RawProcessor.imgdata.other.iso_speed << std::endl;
	std::cerr << "Shutter Speed: " << RawProcessor.imgdata.other.shutter << std::endl;
	std::cerr << "Time Stamp: " << ctime(&(RawProcessor.imgdata.other.timestamp)) << std::endl;

}

/**
  * Verify that the dimensions of the requested cropbox
  * makes sense.
  *
  * Author: West.Suhanic,Nov.14.2012. Code written in Toronto,Canada.
  */
int verifyCropBoxValues( const LibRaw& lr )
{
const std::string method="verifyCropBoxValues";

	/**
	  * Do some basic checking.
	  */
	if( lr.imgdata.params.cropbox[ ROW_NUMBER_START ] > lr.imgdata.sizes.height )
	{
		std::cerr << method << " The starting row number must be less than the image height." << std::endl;
		return -1;
	}

	if( lr.imgdata.params.cropbox[ COL_NUMBER_START ] > lr.imgdata.sizes.width )
	{
		std::cerr << method << " The starting column number must be less than the image width." << std::endl;
		return -1;
	}

	if( lr.imgdata.params.cropbox[ NUMBER_OF_ROWS ] > lr.imgdata.sizes.height )
	{
		std::cerr << method << " The number of rows must be less than the image height." << std::endl;
		return -1;
	}

	if( lr.imgdata.params.cropbox[ NUMBER_OF_COLS ] > lr.imgdata.sizes.width )
	{
		std::cerr << method << " The number of columns must be less than the image width." << std::endl;
		return -1;
	}

	/**
	  * Check that the sum of the starting row number
	  * and the requested number of rows is less than the
	  * total number of rows.
	  */
	if( ( lr.imgdata.params.cropbox[ ROW_NUMBER_START ] + lr.imgdata.params.cropbox[ NUMBER_OF_ROWS ] ) > lr.imgdata.sizes.height )
	{
		std::cerr << method << " cannot proceed. The requested number of rows plust the starting row number exceeds the total image height." << std::endl;
		return -1;
	}

	/**
	  * Check that the sum of the starting column number
	  * and the requested number of columns is less than the
	  * total number of columns.
	  */
	if( ( lr.imgdata.params.cropbox[ COL_NUMBER_START ] + lr.imgdata.params.cropbox[ NUMBER_OF_COLS ] ) > lr.imgdata.sizes.width )
	{
		std::cerr << method << " cannot proceed. The requested number of columns plus the starting column number exceeds the total image width." << std::endl;
		return -1;
	}

return 0;
}

/**
  * This method opens a classic tiff file.
  *
  * Author: West.Suhanic, Dec.22.2012. Code written in Toronto,Canada.
  */
int openTiffFile( const std::string& fileName, TIFF*& out )
{
const std::string method = "openTiffFile";

	/**
	  * Check.
	  */
	if( fileName.length() == 0 )
	{
		std::cerr << method << " failed. The file name is not set." << std::endl;
		return -1;
	}

	/**
	  * Actually try to open the file. If successful a ClassTiff file
	  * will be created.
	  */
	out = NULL;
	out = TIFFOpen(fileName.c_str(), "w");
	if( out == NULL )
	{
		std::cerr << method << " failed to create the tiff file." << std::endl;
		return -1;
	}

return 0;
}

/**
  * This method actually writes the data out to the tiff file.
  * The only thing different is that I have placed the byte
  * stream to be written out in a vector-(your welcome).
  *
  * Author: West.Suhanc, Dec.23.2012, Code written in Toronto,Canada.
  */
int writeDataToTiffFile( TIFF*& out, unsigned int& rowNumber, std::vector< unsigned short >& data )
{
const std::string method = "writeDataToTiffFile";

	/**
	  * Do some basic checks.
	  */
	if( out == NULL )
	{
		std::cerr << method << " failed, The tiff file handle is null." << std::endl;
		return -1;
	}

	if( data.empty() == true )
	{
		std::cerr << method << " failed, There is no data to write into the specified file." << std::endl;
		return -1;
	}

	/**
	  * Write the data to the file.
	  */
	if ( TIFFWriteScanline( out, &data[ 0 ], rowNumber, 0 ) < 0 )
	{
		std::cerr << method << " failed to write data to the tiff file." << std::endl;
		return -1;
	}

return 0;
}

/**
  * This method does as it name implies, it sets various tiff tags.
  * 
  * Author: West.Suhanic, Dec.23.2012. Code written in Toronto, Canada.
  */
int setTiffTags( TIFF*& out, const int& width, const int& length, const std::string& imageDescription, const char* dateTime )
{
const std::string method = "setTiffTags";

	/**
	  * Do some checks.
	  */
	if( out == NULL )
	{
		std::cerr << method << " failed. The tiff handle is not set." << std::endl;
		return -1;
	}

	if( width == 0 )
	{
		std::cerr << method << " failed. The width is not set." << std::endl;
		return -1;
	}

	if( length == 0 )
	{
		std::cerr << method << " failed. The length is not set." << std::endl;
		return -1;
	}

	if( imageDescription.length() == 0 )
	{
		std::cerr << method << " failed. The imageDescription is not set." << std::endl;
		return -1;
	}

	if( dateTime == static_cast< const char * >(NULL) )
	{
		std::cerr << method << " failed. The date time is not set." << std::endl;
		return -1;
	}

	/**
	  * Populate the tiff tags.
	  */
	TIFFSetField( out, TIFFTAG_ARTIST,		"you"       );
	TIFFSetField( out, TIFFTAG_IMAGEDESCRIPTION,	imageDescription.c_str() );
	TIFFSetField( out, TIFFTAG_SAMPLEFORMAT,	1         );
	TIFFSetField( out, TIFFTAG_IMAGEWIDTH,		width         );
	TIFFSetField( out, TIFFTAG_IMAGELENGTH,		length        );
	TIFFSetField( out, TIFFTAG_ORIENTATION,		ORIENTATION_TOPLEFT );
	TIFFSetField( out, TIFFTAG_SAMPLESPERPIXEL,	1             );
	TIFFSetField( out, TIFFTAG_BITSPERSAMPLE,	16	      );
	TIFFSetField( out, TIFFTAG_PLANARCONFIG,	PLANARCONFIG_CONTIG   );
	TIFFSetField( out, TIFFTAG_PHOTOMETRIC,		PHOTOMETRIC_MINISBLACK);
	TIFFSetField( out, TIFFTAG_COMPRESSION,		COMPRESSION_NONE      );
	TIFFSetField( out, TIFFTAG_ROWSPERSTRIP,	length );
	TIFFSetField( out, TIFFTAG_FILLORDER,           FILLORDER_LSB2MSB );
	TIFFSetField( out, TIFFTAG_SAMPLEFORMAT,	SAMPLEFORMAT_UINT );
	TIFFSetField( out, TIFFTAG_DATETIME,		dateTime );

return 0;
}

int main( int argc, char *argv[] )
{
const std::string method = argv[0];

	if( argc != 8 )
	{
		std::cerr << "usage: raw2tiff input_file_name output_file_name crop_box_yes_or_no col_pos_start-(x) row_pos_start-(y) number_cols number_rows " << std::endl;
		return 0;
	}

	/**
	  * Need a consistent timezone.
	  */
	putenv ((char*)"TZ=UTC");

	/**
	  * Start parsing the command line arguments.
	  */
	std::string inputFileName = argv[1];
	if( inputFileName.length() < 5 )
	{
		std::cerr << method << " failed. The input file name is invalid." << std::endl;
		return 0;
	}

	std::string outputFileName = argv[2];
	if( outputFileName.length() == 0 )
	{
		std::cerr << method << " failed. The output file name is invalid." << std::endl;
		return 0;
	}

	/**
	  * Allocate the raw processor.
	  */
	LibRaw RawProcessor;

	/**
	  * Initialize the crop box.
	  */
	RawProcessor.imgdata.params.cropbox[ COL_NUMBER_START ] = 0;
	RawProcessor.imgdata.params.cropbox[ ROW_NUMBER_START ] = 0;
	RawProcessor.imgdata.params.cropbox[ NUMBER_OF_COLS   ] = std::numeric_limits< unsigned long >::max();
	RawProcessor.imgdata.params.cropbox[ NUMBER_OF_ROWS   ] = std::numeric_limits< unsigned long >::max();

	/**
	  * If you want a cropbox specify and store its dimensions.
	  * This is the -B option from dcraw_emu.
	  */
	stringConverter sc;
	std::stringstream ss;
	std::string wantCropBox = argv[3];
	int ret = 0;
	if( wantCropBox.compare( YES ) == 0 )
	{

		/**
		  * Set the starting column number-(x).
		  */
		const std::string col_pos_start = argv[4];
		ret = sc.convertTheString< std::stringstream, unsigned int >( ss, col_pos_start, RawProcessor.imgdata.params.cropbox[ COL_NUMBER_START ] );
		if( ret == -1 )
		{
			std::cerr << method << " failed on convertTheString for the value " << col_pos_start << std::endl;
			return -1;
		}
	
		const unsigned int colNumberStart = RawProcessor.imgdata.params.cropbox[ COL_NUMBER_START ];

		/**
		  * Set the the starting row number-(y).
		  */
		const std::string row_pos_start = argv[5];
		ret = sc.convertTheString< std::stringstream, unsigned int >( ss, row_pos_start, RawProcessor.imgdata.params.cropbox[ ROW_NUMBER_START ] );
		if( ret == -1 )
		{
			std::cerr << method << " failed on convertStringToUnsignedInt for the value " << row_pos_start << std::endl;
			return -1;
		}

		const unsigned int rowNumberStart = RawProcessor.imgdata.params.cropbox[ ROW_NUMBER_START ];

		/**
		  * Set the number of columns.
		  */
		const std::string numberOfCols = argv[6];
		ret = sc.convertTheString< std::stringstream, unsigned int >( ss, numberOfCols, RawProcessor.imgdata.params.cropbox[ NUMBER_OF_COLS ] );
		if( ret == -1 )
		{
			std::cerr << method << " failed on convertStringToUnsignedInt for the value " << numberOfCols << std::endl;
			return -1;
		}

		const unsigned int imageWidth = RawProcessor.imgdata.params.cropbox[ NUMBER_OF_COLS ];

		/**
		  * Set the number of rows.
		  */
		const std::string numberOfRows = argv[7];
		ret = sc.convertTheString< std::stringstream, unsigned int >( ss, numberOfRows, RawProcessor.imgdata.params.cropbox[ NUMBER_OF_ROWS ] );
		if( ret == -1 )
		{
			std::cerr << method << " failed on convertStringToUnsignedInt for the value " << numberOfRows << std::endl;
			return -1;
		}

		const unsigned int imageHeight = RawProcessor.imgdata.params.cropbox[ NUMBER_OF_ROWS ];

		std::cerr << "Crop Box[COL_NUMBER_START]->" << RawProcessor.imgdata.params.cropbox[ 0 ] << std::endl;
		std::cerr << "Crop Box[ROW_NUMBER_START]->" << RawProcessor.imgdata.params.cropbox[ 1 ] << std::endl;
		std::cerr << "Crop Box[NUMBER_OF_COLS]->" << RawProcessor.imgdata.params.cropbox[ 2 ] << std::endl;
		std::cerr << "Crop Box[NUMBER_OF_ROWS]->" << RawProcessor.imgdata.params.cropbox[ 3 ] << std::endl;

	}

	/**
	  * Attempt to open the specified the file.
	  */
	ret = RawProcessor.open_file( inputFileName.c_str() );
	if( ret != LIBRAW_SUCCESS )
	{
		std::cerr << method << " failed on open_file for the file " << inputFileName << std::endl;
		std::cerr << " The error is " << libraw_strerror(ret) << std::endl;
		RawProcessor.recycle();
		return 1;
	}

	/**
	  * Set the dimensions of the image.
	  */
	unsigned int colNumberStart = 0;
	unsigned int rowNumberStart = 0;
	unsigned int imageHeight = RawProcessor.imgdata.sizes.height;
	unsigned int imageWidth = RawProcessor.imgdata.sizes.width;

	/**
	  * Verify the dimensions of the crop box.
	  */
	if( wantCropBox.compare( YES ) == 0 )
	{
		ret = verifyCropBoxValues( RawProcessor );
		if( ret == -1 )
		{
			std::cerr << method << " cannot proceed. The crop box values are not acceptable." << std::endl;
			RawProcessor.recycle();
			return ret;
		}

		/**
		  * Set the image dimensions to the crop box specification.
		  */
		colNumberStart = RawProcessor.imgdata.params.cropbox[ COL_NUMBER_START ];
		rowNumberStart = RawProcessor.imgdata.params.cropbox[ ROW_NUMBER_START ];
		imageWidth = colNumberStart + RawProcessor.imgdata.params.cropbox[ NUMBER_OF_COLS ];
		imageHeight = rowNumberStart + RawProcessor.imgdata.params.cropbox[ NUMBER_OF_ROWS ];
	}

	/**
	  * Get the image information.
	  */
	getImageInformation( RawProcessor );

	/**
	  * Try to unpack the data.
	  */
	ret = RawProcessor.unpack();
	if( ret != LIBRAW_SUCCESS )
	{
		std::cerr << method << " failed on unpack for the file " << inputFileName << std::endl;
		std::cerr << " The error is " << libraw_strerror(ret) << std::endl;
		RawProcessor.recycle();
		return 1;
	}

	/**
	  * Call raw2image.
	  */
	ret = RawProcessor.raw2image();
	if( ret != LIBRAW_SUCCESS )
	{
		std::cerr << method << " failed on raw2image for the file " << inputFileName << std::endl;
		std::cerr << " The error is " << libraw_strerror(ret) << std::endl;
		RawProcessor.recycle();
		return 1;
	}

	/**
	  * Call subtract_black.
	  */
	RawProcessor.subtract_black();

	/**
	  * Open the tiff file.
	  */
	TIFF *out = NULL;
	if( -1 == openTiffFile( outputFileName, out ) )
	{
		std::cerr << method << " failed to create the tiff file " << outputFileName << std::endl;
		return -1;
	}

	/**
	  * Set the image description.
	  */
	std::stringstream imageDescription;
	imageDescription.clear();

	size_t pos = inputFileName.find_last_of( FWD_SLASH );
	imageDescription << "TIFF of " << inputFileName.substr( pos + 1 ) << std::ends;
	if( imageDescription.bad() == true )
	{
		std::cerr << method << " failed to populate the image description for the file " << inputFileName << std::endl;
		return -1;
	}
	
	/**
	  * Allocate the needed date/time structures.
	  */
	time_t rawtime;
	struct tm * timeinfo;

	/**
	  * Populate the date/time structures.
	  */
	time ( &rawtime );
	timeinfo = localtime ( &rawtime );
	if( timeinfo == static_cast< struct tm * >( NULL ) )
	{
		std::cerr << method << " failed on localtime." << std::endl;
		return -1;
	}

	/**
	  * Format date-time according to the datetime tifftag specification.
	  */
	char dateTimeBuffer[ 80 ];
	memset( &dateTimeBuffer[0], 0, 80 );
	ret = strftime( dateTimeBuffer, sizeof( dateTimeBuffer ), "%Y:%m:%d %H:%M:%S", timeinfo );
	if( ret <= 0 )
	{
		std::cerr << method << " failed on strftime." << std::endl;
		return -1;
	}

	/**
	  * Set the tiff tags.
	  */
	if( wantCropBox.compare( YES ) == 0 )
	{
		if( -1 == setTiffTags( out, RawProcessor.imgdata.params.cropbox[ NUMBER_OF_COLS ], RawProcessor.imgdata.params.cropbox[ NUMBER_OF_ROWS ], imageDescription.str(), &dateTimeBuffer[0] ) )
		{
			std::cerr << method << " failed to set the tiff tags for the file-(cropbox) " << outputFileName << std::endl;
			return -1;
		}
	}
	else
	{
		if( -1 == setTiffTags( out, imageWidth, imageHeight, imageDescription.str(), &dateTimeBuffer[0] ) )
		{
			std::cerr << method << " failed to set the tiff tags for the file " << outputFileName << std::endl;
			return -1;
		}
	}

	/**
	  * Using a vector to hold the image data.
	  * Place the image buffer into the vector dataVector.
	  */
	std::vector< unsigned short > dataVector;
	dataVector.reserve( imageWidth );//must be here.
	dataVector.assign( imageWidth, 0 );//must be here.

	/**
	  * Show the image dimensions to be used.
	  */
	std::cerr << "----Image Area----" << std::endl;
	std::cerr << "Row Number Start->" << rowNumberStart << std::endl;
	std::cerr << "Row Number End  ->" << imageHeight << std::endl;
	std::cerr << "Col Number Start->" << colNumberStart << std::endl;
	std::cerr << "Col Number End  ->" << imageWidth << std::endl;

	/**
	  * Actually write out the data. Please note that
	  * the fcol method returns the 'color' of the given
	  * bayer pixel. This number ranges from 0 to 3 for
	  * RGB sensors where 0 is Red, 1 is Green, 2 is Blue,
	  * and 3 is the second Green pixel. I got this information
	  * from Alex Tutubalin the author LibRaw.
	  */
	unsigned int rowPos = 0;//this is need for the cropbox.
	unsigned int colPos = 0;//this is need for the cropbox.
	int rv = 0;
	for( unsigned int row = rowNumberStart; row < imageHeight; row++ )
	{
		/**
		  * Fill the datavector.
		  */
		colPos = 0;
		for( unsigned int col = colNumberStart; col < imageWidth; col++ )
		{
			dataVector.at( colPos ) = RawProcessor.imgdata.image[ row * imageWidth + col ][ RawProcessor.fcol( row, col ) ];
			colPos++;
		}

		/**
		  * Write the datavector to the target tiff file.
		  */
		rv = writeDataToTiffFile( out, rowPos, dataVector );
		if( rv == -1 )
		{
			std::cerr << argv[0] << " failed on writeDataToTiffFile. " << std::endl;
			break;
		}

		/**
		  * re-initialize the datavector.
		  */
		dataVector.assign( imageWidth, 0 );

		/**
		  * Next row.
		  */
		rowPos++;
	}

	/**
	  * Clear the vector.
	  */
	dataVector.clear();

	/**
	  * Close the tiff file.
	  */
	TIFFClose(out);

	/**
	  * It is over, be happy.
	  */
	RawProcessor.recycle();

return 0;
}

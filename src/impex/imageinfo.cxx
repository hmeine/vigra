/************************************************************************/
/*                                                                      */
/*               Copyright 2002 by Gunnar Kedenburg                     */
/*                                                                      */
/*    This file is part of the VIGRA computer vision library.           */
/*    The VIGRA Website is                                              */
/*        http://hci.iwr.uni-heidelberg.de/vigra/                       */
/*    Please direct questions, bug reports, and contributions to        */
/*        ullrich.koethe@iwr.uni-heidelberg.de    or                    */
/*        vigra@informatik.uni-hamburg.de                               */
/*                                                                      */
/*    Permission is hereby granted, free of charge, to any person       */
/*    obtaining a copy of this software and associated documentation    */
/*    files (the "Software"), to deal in the Software without           */
/*    restriction, including without limitation the rights to use,      */
/*    copy, modify, merge, publish, distribute, sublicense, and/or      */
/*    sell copies of the Software, and to permit persons to whom the    */
/*    Software is furnished to do so, subject to the following          */
/*    conditions:                                                       */
/*                                                                      */
/*    The above copyright notice and this permission notice shall be    */
/*    included in all copies or substantial portions of the             */
/*    Software.                                                         */
/*                                                                      */
/*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND    */
/*    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES   */
/*    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND          */
/*    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT       */
/*    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,      */
/*    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING      */
/*    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR     */
/*    OTHER DEALINGS IN THE SOFTWARE.                                   */
/*                                                                      */
/************************************************************************/
/* Modifications by Pablo d'Angelo
 * updated to vigra 1.4 by Douglas Wilkins
 * as of 18 Febuary 2006:
 *  - Added UINT16 and UINT32 pixel types.
 *  - Added support for obtaining extra bands beyond RGB.
 *  - Added support for a position field that indicates the start of this
 *    image relative to some global origin.
 *  - Added support for x and y resolution fields.
 *  - Added support for ICC profiles
 */

#include <iostream>
#include <algorithm>
#include <functional>
#include <iterator>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <iterator>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "vigra/array_vector.hxx"
#include "vigra/imageinfo.hxx"
#include "codecmanager.hxx"

#if defined(_WIN32)
#  include "vigra/windows.h"
#else
#  include <dirent.h>
#endif

namespace vigra
{

// build a string from a sequence.
#if defined(_MSC_VER) && (_MSC_VER < 1300)
template <class iterator>
std::string stringify (const iterator &start, const iterator &end)
{
    return stringifyImpl(start, end, *start);
}

template <class iterator, class Value>
std::string stringifyImpl (const iterator &start, const iterator &end, Value const &)
{
    std::ostringstream out;
    // do not place a space character after the last sequence element.
    std::copy (start, end - 1,
               std::ostream_iterator <Value> (out, " "));
    out << *(end-1);
    return out.str ();
}

#else

template <class iterator>
std::string stringify (const iterator &start, const iterator &end)
{
    typedef typename std::iterator_traits<iterator>::value_type value_type;
    std::ostringstream out;
    // do not place a space character after the last sequence element.
    std::copy (start, end - 1,
               std::ostream_iterator <value_type> (out, " "));
    out << *(end-1);
    return out.str ();
}

#endif // _MSC_VER < 1300

void validate_filetype( std::string filetype )
{
    vigra_precondition( codecManager().fileTypeSupported(filetype),
                        "given file type is not supported" );
}

std::string impexListFormats()
{
    std::vector<std::string> ft = codecManager().supportedFileTypes();
    return stringify( ft.begin(), ft.end() );
}

std::string impexListExtensions()
{
    std::vector<std::string> ft = codecManager().supportedFileExtensions();
    return stringify( ft.begin(), ft.end() );
}

bool isImage(char const * filename)
{
    return CodecManager::manager().getFileTypeByMagicString(filename) != "";
}

// class ImageExportInfo

ImageExportInfo::ImageExportInfo( const char * filename, const char * mode )
    : m_filename(filename), m_mode(mode),
      m_x_res(0), m_y_res(0),
      fromMin_(0.0), fromMax_(0.0), toMin_(0.0), toMax_(0.0)
{}

ImageExportInfo::~ImageExportInfo()
{
}

ImageExportInfo & ImageExportInfo::setFileType( const char * filetype )
{
    m_filetype = filetype;
    return *this;
}

ImageExportInfo & ImageExportInfo::setForcedRangeMapping(double fromMin, double fromMax,
                                                     double toMin, double toMax)
{
    fromMin_ = fromMin;
    fromMax_ = fromMax;
    toMin_ = toMin;
    toMax_ = toMax;
    return *this;
}

bool ImageExportInfo::hasForcedRangeMapping() const
{
    return (fromMax_ > fromMin_) || (toMax_ > toMin_);
}

double ImageExportInfo::getFromMin() const
{
    return fromMin_;
}

double ImageExportInfo::getFromMax() const
{
    return fromMax_;
}

double ImageExportInfo::getToMin() const
{
    return toMin_;
}

double ImageExportInfo::getToMax() const
{
    return toMax_;
}

ImageExportInfo & ImageExportInfo::setCompression( const char * comp )
{
    m_comp = comp;
    return *this;
}

ImageExportInfo & ImageExportInfo::setFileName(const char * name)
{
    m_filename = name;
    return *this;
}

const char * ImageExportInfo::getFileName() const
{
    return m_filename.c_str();
}

const char * ImageExportInfo::getMode() const
{
    return m_mode.c_str();
}

const char * ImageExportInfo::getFileType() const
{
    return m_filetype.c_str();
}

ImageExportInfo & ImageExportInfo::setPixelType( const char * s )
{
    m_pixeltype = s;
    return *this;
}

const char * ImageExportInfo::getPixelType() const
{
    return m_pixeltype.c_str();
}

const char * ImageExportInfo::getCompression() const
{
    return m_comp.c_str();
}

float ImageExportInfo::getXResolution() const
{
    return m_x_res;
}

float ImageExportInfo::getYResolution() const
{
    return m_y_res;
}

ImageExportInfo & ImageExportInfo::setXResolution( float val )
{
    m_x_res = val;
    return *this;
}

ImageExportInfo & ImageExportInfo::setYResolution( float val )
{
    m_y_res = val;
    return *this;
}

ImageExportInfo & ImageExportInfo::setPosition(const vigra::Diff2D & pos)
{
    m_pos = pos;
    return *this;
}

vigra::Size2D ImageExportInfo::getCanvasSize() const
{
    return m_canvas_size ;
}

ImageExportInfo & ImageExportInfo::setCanvasSize(const Size2D & size)
{
    m_canvas_size = size;
    return *this;
}

vigra::Diff2D ImageExportInfo::getPosition() const
{
    return m_pos;
}

const ImageExportInfo::ICCProfile & ImageExportInfo::getICCProfile() const
{
    return m_icc_profile;
}

ImageExportInfo & ImageExportInfo::setICCProfile(
    const ImageExportInfo::ICCProfile &profile)
{
    m_icc_profile = profile;
    return *this;
}

// return an encoder for a given ImageExportInfo object
std::auto_ptr<Encoder> encoder( const ImageExportInfo & info )
{
    std::auto_ptr<Encoder> enc;

    std::string filetype = info.getFileType();
    if ( filetype != "" ) {
        validate_filetype(filetype);
        std::auto_ptr<Encoder> enc2
            = getEncoder( std::string( info.getFileName() ), filetype, std::string( info.getMode() ) );
        enc = enc2;
    } else {
        std::auto_ptr<Encoder> enc2
            = getEncoder( std::string( info.getFileName() ), "undefined", std::string( info.getMode() ) );
        enc = enc2;
    }

    std::string comp = info.getCompression();
    if ( comp != "" ) {

        // check for quality parameter of JPEG compression
        int quality = -1;

        // possibility 1: quality specified as "JPEG QUALITY=N" or "JPEG-ARITH QUALITY=N"
        // possibility 2 (deprecated): quality specified as just a number "10"
        std::string sq(" QUALITY="), parsed_comp;
        std::string::size_type pos = comp.rfind(sq), start = 0;

        if(pos != std::string::npos)
        {
            start = pos + sq.size();
            parsed_comp = comp.substr(0, pos);
        }

        std::istringstream compstream(comp.substr(start));
        compstream >> quality;
        if ( quality != -1 )
        {
            if(parsed_comp == "")
                parsed_comp = "JPEG";
             enc->setCompressionType( parsed_comp, quality );
        }
        else
        {
            // leave any other compression type to the codec
            enc->setCompressionType(comp);
        }
    }

    std::string pixel_type = info.getPixelType();
    if ( pixel_type != "" ) {
        if(!isPixelTypeSupported( enc->getFileType(), pixel_type ))
        {
            std::string msg("exportImage(): file type ");
            msg += enc->getFileType() + " does not support requested pixel type "
                                      + pixel_type + ".";
            vigra_precondition(false, msg.c_str());
        }
        enc->setPixelType(pixel_type);
    }

    // set other properties
    enc->setXResolution(info.getXResolution());
    enc->setYResolution(info.getYResolution());
    enc->setPosition(info.getPosition());
    enc->setCanvasSize(info.getCanvasSize());

    if ( info.getICCProfile().size() > 0 ) {
        enc->setICCProfile(info.getICCProfile());
    }

    return enc;
}

// class ImageImportInfo

ImageImportInfo::ImageImportInfo( const char * filename, unsigned int imageIndex )
    : m_filename(filename), m_image_index(imageIndex)
{
    readHeader_();
}

ImageImportInfo::~ImageImportInfo() {
}

const char * ImageImportInfo::getFileName() const
{
    return m_filename.c_str();
}

const char * ImageImportInfo::getFileType() const
{
    return m_filetype.c_str();
}

const char * ImageImportInfo::getPixelType() const
{
    return m_pixeltype.c_str();
}

ImageImportInfo::PixelType ImageImportInfo::pixelType() const
{
    const std::string pixeltype=ImageImportInfo::getPixelType();
   if (pixeltype == "UINT8")
     return UINT8;
   if (pixeltype == "INT16")
     return INT16;
   if (pixeltype == "UINT16")
     return UINT16;
   if (pixeltype == "INT32")
     return INT32;
   if (pixeltype == "UINT32")
     return UINT32;
   if (pixeltype == "FLOAT")
     return FLOAT;
   if (pixeltype == "DOUBLE")
     return DOUBLE;
   vigra_fail( "internal error: unknown pixel type" );
   return ImageImportInfo::PixelType();
}

int ImageImportInfo::width() const
{
    return m_width;
}

int ImageImportInfo::height() const
{
    return m_height;
}

int ImageImportInfo::numBands() const
{
    return m_num_bands;
}

int ImageImportInfo::numExtraBands() const
{
    return m_num_extra_bands;
}

int ImageImportInfo::numImages() const
{
    return m_num_images;
}

void ImageImportInfo::setImageIndex(int index)
{
    m_image_index = index;
    readHeader_();
}

int ImageImportInfo::getImageIndex() const
{
    return m_image_index;
}

Size2D ImageImportInfo::size() const
{
    return Size2D( m_width, m_height );
}

MultiArrayShape<2>::type ImageImportInfo::shape() const
{
    return MultiArrayShape<2>::type( m_width, m_height );
}

bool ImageImportInfo::isGrayscale() const
{
    return (m_num_bands - m_num_extra_bands) == 1;
}

bool ImageImportInfo::isColor() const
{
    return (m_num_bands - m_num_extra_bands) == 3;
}

bool ImageImportInfo::isByte() const
{
    return m_pixeltype == "UINT8";
}

Diff2D ImageImportInfo::getPosition() const
{
    return m_pos;
}

Size2D ImageImportInfo::getCanvasSize() const
{
    return m_canvas_size;
}

float ImageImportInfo::getXResolution() const
{
    return m_x_res;
}

float ImageImportInfo::getYResolution() const
{
    return m_y_res;
}

const ImageImportInfo::ICCProfile & ImageImportInfo::getICCProfile() const
{
    return m_icc_profile;
}

void ImageImportInfo::readHeader_()
{
    std::auto_ptr<Decoder> decoder = getDecoder(m_filename, "undefined", m_image_index);
    m_num_images = decoder->getNumImages();

    m_filetype = decoder->getFileType();
    m_pixeltype = decoder->getPixelType();
    m_width = decoder->getWidth();
    m_height = decoder->getHeight();
    m_num_bands = decoder->getNumBands();
    m_num_extra_bands = decoder->getNumExtraBands();
    m_pos = decoder->getPosition();
    m_canvas_size = decoder->getCanvasSize();
    m_x_res = decoder->getXResolution();
    m_y_res = decoder->getYResolution();

    m_icc_profile = decoder->getICCProfile();

    decoder->abort(); // there probably is no better way than this
}

// return a decoder for a given ImageImportInfo object
std::auto_ptr<Decoder> decoder( const ImageImportInfo & info )
{
    std::string filetype = info.getFileType();
    validate_filetype(filetype);
    return getDecoder( std::string( info.getFileName() ), filetype, info.getImageIndex() );
}

} // namespace vigra

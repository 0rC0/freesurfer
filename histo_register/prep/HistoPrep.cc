#include "prep/HistoPrep.h"
#include <sbl/core/Command.h>
#include <sbl/core/PathConfig.h>
#include <sbl/math/MathUtil.h>
#include <sbl/system/FileSystem.h>
#include <sbl/image/ImageUtil.h>
#include <sbl/image/ImageDraw.h>
#include <sbl/image/ImageRegister.h>
#include <sbl/other/Plot.h>
using namespace sbl;
namespace hb {


/// extract mask of histology foreground regions
aptr<ImageGrayU> createHistologyMask( const ImageGrayU &image, const String &visPath, const String &fileName ) {
	int width = image.width(), height = image.height();
	int histogramBorder = 200;
	int initThresh = 200;
	int minSize = 200;
	bool savePlots = true;

	// compute a histogram
	VectorF hist = toFloat( imageHistogram( image, histogramBorder, width - histogramBorder, histogramBorder, height - histogramBorder ) );
	hist = gaussFilter( hist, 3 );
	int thresh = nearestMinIndex( hist, initThresh );
	disp( 1, "input: %s, thresh: %d, hist at thresh: %f", fileName.c_str(), thresh, hist[ thresh ] );

	// extract mask
	aptr<ImageGrayU> blur = blurGauss( image, 4 );
	aptr<ImageGrayU> mask = threshold( *blur, (float) thresh, true );
	filterMaskComponents( *mask, minSize * minSize, width * height );
	fillMaskHoles( *mask, 30 * 30 );

	// save histogram plot
	if (savePlots) {
		Plot plot;
		plot.add( toDouble( hist ) );
		plot.addVertLine( (double) thresh );
		String plotFileName = visPath + "/histogram_" + fileName.leftOfLast( '.' ) + ".svg";
		plot.save( plotFileName );
	}
	return mask;
}


/// find lower and upper thresholds in a histogram corresponding to upper and lower percentiles
void findHistogramBounds( const VectorF &histogram, float lowerFrac, float upperFrac, int &lowerThresh, int &upperThresh ) {
	VectorF histogramBlur = gaussFilter( histogram, 3 );
	float fullSum = histogramBlur.sum();
	float sum = 0;
	for (int i = 0; i < 256; i++) {
		sum += histogramBlur[ i ];
		if (sum > fullSum * lowerFrac && lowerThresh == 0)
			lowerThresh = i;
		if (sum > fullSum * upperFrac && upperThresh == 0)
			upperThresh = i;
	}
}


/// normalize the brightness of each channel separately
void normalizeChannels( ImageColorU &image ) {
	int width = image.width(), height = image.height();

	// loop over channels
	for (int c = 0; c < 3; c++) {

		// loop over image, creating histogram of non-background pixels
		VectorF histogram( 256 );
		histogram.clear( 0 );
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				int v = image.data( x, y, c );
				if (v > 0 && v < 254) {
					histogram[ v - 1 ] += 0.5f;
					histogram[ v ] += 1.0f;
					histogram[ v + 1 ] += 0.5f;
				}
			}
		}

		// compute thresholds from histogram
		int lowerThresh = 0, upperThresh = 0;
		findHistogramBounds( histogram, 0.1f, 0.9f, lowerThresh, upperThresh );

		// apply thresholds
		if (lowerThresh != upperThresh ) {
			for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++) {
					int v = image.data( x, y, c );
					if (v < 255) {
						v = 64 + (v - lowerThresh) * 128 / (upperThresh - lowerThresh);
						v = bound( v, 5, 250 );
						image.data( x, y, c ) = v;
					}
				}
			}
		}
	}
}


/// combine multiple histology wavelengths into a single image
aptr<ImageColorU> combineHistoWavelengths( const ImageGrayU &image700, const ImageGrayU &image800, const ImageGrayU &mask ) {
	int width = image700.width(), height = image700.height();
	assertAlways( image800.width() == width && image800.height() == height );

	// create combined image using each wavelength as a separate color channel
	aptr<ImageColorU> colorHisto( new ImageColorU( width, height ) );
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			if (mask.data( x, y )) {
				int r = image800.data( x, y );
				int g = image700.data( x, y );
				colorHisto->setRGB( x, y, r, g, 0 );
			} else {
				colorHisto->setRGB( x, y, 255, 255, 255 );
			}
		}
	}
	return colorHisto;
}


/// extract a foreground region (one tissue slice, hopefully) from the slide image
void extractRegion( const ImageColorU &input, ImageGrayU &mask, int xCheck, int yCheck, bool firstPass, int &outputWidth, int &outputHeight, const String &outputPath, int rotateAngle, int id ) {
	int border = 100;

	// find region nearest to check point
	int width = input.width(), height = input.height();
	int distSqdBest = width * width + height * height;
	int xBest = -1, yBest = -1;
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			if (mask.data( x, y ) == 255) {
				int xDiff = x - xCheck;
				int yDiff = y - yCheck;
				int distSqd = xDiff * xDiff + yDiff * yDiff;
				if (distSqd < distSqdBest) {
					xBest = x;
					yBest = y;
					distSqdBest = distSqd;
				}
			}
		}
	}
	if (xBest == -1) {
		warning( "no points found" );
		return;
	} 

	// fill found region
	float xCent = 0, yCent = 0;
	int xMin = 0, xMax = 0, yMin = 0, yMax = 0;
	floodFill( mask, 255, 255, 100, xBest, yBest, &xCent, &yCent, &xMin, &xMax, &yMin, &yMax );

	// if first pass, compute desired output size and return
	if (firstPass) {

		// compute output width/height
		int curOutputWidth = xMax - xMin + 2 * border;
		int curOutputHeight = yMax - yMin + 2 * border;
		curOutputWidth -= (curOutputWidth & 7); // make divisible by 8
		curOutputHeight -= (curOutputHeight & 7); // make divisible by 8

		// update global output size
		if (curOutputWidth > outputWidth)
			outputWidth = curOutputWidth;
		if (curOutputHeight > outputHeight)
			outputHeight = curOutputHeight;
		return;
	}

	// compute border needed to center the image
	int xBorder = (outputWidth - (xMax - xMin)) / 2;
	int yBorder = (outputHeight - (yMax - yMin)) / 2;

	// create output image
	ImageColorU output( outputWidth, outputHeight );
	output.clear( 255, 255, 255 );
	for (int y = yMin; y <= yMax; y++) {
		for (int x = xMin; x <= xMax; x++) {
			if (mask.data( x, y ) == 100) {
				int xOut = x - xMin + xBorder;
				int yOut = y - yMin + yBorder;
				if (output.inBounds( xOut, yOut )) {
					for (int c = 0; c < 3; c++)
						output.data( xOut, yOut, c ) = input.data( x, y, c );
				}
				mask.data( x, y ) = 50;
			}
		}
	}

	// normalize image channels
	normalizeChannels( output );

	// save result
	String outputFileName = outputPath + "/" + sprintF( "%03d", id ) + ".png";
	saveImage( output, outputFileName );

	// save individual channels as grayscale images
	ImageGrayU output700( outputWidth, outputHeight );
	ImageGrayU output800( outputWidth, outputHeight );
	for (int y = 0; y < outputHeight; y++) {
		for (int x = 0; x < outputWidth; x++) {
			output700.data( x, y ) = output.g( x, y );
			output800.data( x, y ) = output.r( x, y );
		}
	}
	createDir( outputPath + "/gray" );
	outputFileName = outputPath + "/gray/" + sprintF( "%03d", id ) + "_700.tif";
	saveImage( output700, outputFileName );
	outputFileName = outputPath + "/gray/" + sprintF( "%03d", id ) + "_800.tif";
	saveImage( output800, outputFileName );
}


/// prepare histology slide images: segment fg/bg, split multiple samples, normalize each sample
/// (note: we assume that slide photo images are a subset of the licor images)
void prepareHistologyImages( Config &conf ) {

	// get command parameters
	int rotateAngle = conf.readInt( "rotateAngle", 0 );
	String inputPath = addDataPath( conf.readString( "inputPath", "histo/raw" ) );
	String outputPath = addDataPath( conf.readString( "outputPath", "histo/split" ) );

	// create output directory
	createDir( outputPath );

	// prepare visualization path
	String visPath = outputPath + "/vis";
	createDir( visPath );

	// these will hold the output size
	int outputWidth = 0, outputHeight = 0;

	// get list of input images
	Array<String> fileList = dirFileList( inputPath, "", "_700.png" );

	// do two passes:
	// 1. determine size bounds
	// 2. save images
	for (int pass = 0; pass < 2; pass++) {

		// loop over input images
		for (int i = 0; i < fileList.count(); i++) {

			// load input image
			String inputFileName = inputPath + "/" + fileList[ i ];
			aptr<ImageGrayU> image700 = load<ImageGrayU>( inputFileName );
			aptr<ImageGrayU> image800 = load<ImageGrayU>( inputFileName.leftOfLast( '_' ) + "_800.png" );
			int width = image700->width(), height = image700->height();

			// get slice numbers
			// we assume that the file name is of the form:
			//     x_sliceId1_sliceId2_y.png 
			// or:
			//     x_sliceId2_y.png 
			Array<String> split = fileList[ i ].split( "_" );
			if (split.count() < 3) {
				warning( "unexpected file name format (expected at least 2 underscores)" );
				return;
			}
			int sliceId1 = split[ split.count() - 3 ].toInt(); // will be zero if not numeric
			int sliceId2 = split[ split.count() - 2 ].toInt();
			disp( 2, "id1: %d, id2: %d", sliceId1, sliceId2 );

			// compute mask
			aptr<ImageGrayU> mask = createHistologyMask( *image700, visPath, fileList[ i ] );

			// combine histo wavelengths into single color image
			aptr<ImageColorU> image = combineHistoWavelengths( *image700, *image800, *mask );

			// if two IDs, extract two regions
			if (sliceId1) {
				extractRegion( *image, *mask, width / 4, height / 4, pass == 0, outputWidth, outputHeight, outputPath, rotateAngle, sliceId1 );
				extractRegion( *image, *mask, width * 3 / 4, height * 3 / 4, pass == 0, outputWidth, outputHeight, outputPath, rotateAngle, sliceId2 );

			// if one ID, extract one region
			} else {
				extractRegion( *image, *mask, width / 2, height / 2, pass == 0, outputWidth, outputHeight, outputPath, rotateAngle, sliceId2 );
			}

			// check for user cancel
			if (checkCommandEvents())
				return;
		}

		// check for user cancel
		if (checkCommandEvents())
			return;
	}
}


//-------------------------------------------
// INIT / CLEAN-UP
//-------------------------------------------


// register commands, etc. defined in this module
void initHistoPrep() {
	registerCommand( "hprep", prepareHistologyImages );
}


} // end namespace hb

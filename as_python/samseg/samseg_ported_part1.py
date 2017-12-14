# ﻿function [ FreeSurferLabels, names, volumesInCubicMm ] = samsegment( imageFileNames, transformedTemplateFileName, ...
#                                                                      modelSpecifications, optimizationOptions, ...
#                                                                      savePath, showFigures )
# %
# %
#
#
#
# % Print input options
# disp( '==========================' );
# disp( 'imageFileNames: ' )
# for i = 1 : length( imageFileNames )
#   disp( imageFileNames{ i } )
# end
# fprintf( '-----\n' )
#
# disp( 'transformedTemplateFileName: ' )
# disp( transformedTemplateFileName )
# fprintf( '-----\n' )
#
# disp( 'modelSpecifications: ' )
# fieldNames = fieldnames( modelSpecifications );
# for i = 1 : length( fieldNames )
#   fieldName = fieldNames{ i };
#   disp( fieldName )
#   fields = getfield( modelSpecifications, fieldName );
#   for j = 1 : length( fields )
#     disp( fields(j) )
#   end
#   disp( ' ' )
# end
# fprintf( '-----\n' )
#
# disp( 'optimizationOptions:' )
# fieldNames = fieldnames( optimizationOptions );
# for i = 1 : length( fieldNames )
#   fieldName = fieldNames{ i };
#   disp( fieldName )
#   fields = getfield( optimizationOptions, fieldName );
#   for j = 1 : length( fields )
#     disp( fields(j) )
#   end
#   disp( ' ' )
# end
# fprintf( '-----\n' )
#
# disp( 'savePath: ' )
# disp( savePath )
# fprintf( '-----\n' )
#
# disp( 'showFigures: ' )
# disp( showFigures )
# fprintf( '-----\n' )
#
#
# % Save input variables in a "history" structure
# history = struct;
# history.input = struct;
# history.input.imageFileNames = imageFileNames;
# history.input.transformedTemplateFileName = transformedTemplateFileName;
# history.input.modelSpecifications = modelSpecifications;
# history.input.optimizationOptions = optimizationOptions;
# history.input.savePath = savePath;
# history.input.showFigures = showFigures;
#
#
#
# %
# numberOfContrasts = length( imageFileNames );
#
# % Read the image data from disk. At the same time, construct a 3-D affine transformation (i.e.,
# % translation, rotation, scaling, and skewing) as well - this transformation will later be used
# % to initially transform the location of the atlas mesh's nodes into the coordinate system of
# % the image.
# %
# imageBuffers = [];
# for contrastNumber = 1 : numberOfContrasts
#   % Get the pointers to image and the corresponding transform
#   [ images( contrastNumber ), transform, nonCroppedImageSize, croppingOffset ] = ...
#                                     kvlReadCroppedImage( imageFileNames{ contrastNumber }, transformedTemplateFileName );
#   imageBuffers( :, :, :, contrastNumber ) = kvlGetImageBuffer( images( contrastNumber ) ); % Get the actual imageBuffer
# end
# imageSize = [ size( imageBuffers, 1 ) size( imageBuffers, 2 ) size( imageBuffers, 3 ) ];
#
# if ( showFigures )
#   for contrastNumber = 1 : numberOfContrasts
#     figure
#     showImage( imageBuffers( :, :, :, contrastNumber ) ); % Automatically displays middle slices in each direction
#   end
# end
#
# % Also read in the voxel spacing -- this is needed since we'll be specifying bias field smoothing kernels, downsampling
# % steps etc in mm.
# [ ~, imageToWorldTransform ] = kvlReadImage( imageFileNames{1} );
# imageToWorldTransformMatrix = kvlGetTransformMatrix( imageToWorldTransform );
# voxelSpacing = sum( imageToWorldTransformMatrix( 1 : 3, 1 : 3 ).^2 ).^( 1/2 );
#
#
# % Read the atlas mesh from file, immediately applying the previously determined transform to the location
# % of its nodes. Rather than one mesh, the atlas consists of a so-called "collection" of meshes: they're
# % all the same except for the location of their mesh nodes. The first mesh, so-called "reference mesh"
# % has index -1: it represents the average shape (location of the mesh nodes) of all the other meshes. If
# % the atlas was built from N training subjects, there will be N other meshes as well, each one warping
# % the atlas optimally to each of the training subjects
# %
# % You also need to provide a value for K, which determines the flexibility of the atlas mesh, i.e., how
# % much it will typically deform. Higher values correspond to stiffer meshes.
# %
# meshCollection = kvlReadMeshCollection( modelSpecifications.atlasFileName, transform, modelSpecifications.K );
#
# % Retrieve the reference mesh, i.e., the mesh representing the average shape.
# mesh = kvlGetMesh( meshCollection, -1 );
#
# % Get a Matlab matrix containing a copy of the probability vectors in each mesh node (size numberOfNodes x
# % numberOfLabels ).
# alphas = kvlGetAlphasInMeshNodes( mesh );
#
# % Mask away uninteresting voxels. This is done by a poor man's implementation of a dilation operation on
# % a non-background class mask; followed by a cropping to the area covered by the mesh (needed because
# % otherwise there will be voxels in the data with prior probability zero of belonging to any class)
# labelNumber = 0; % background label
# backgroundPrior = kvlRasterizeAtlasMesh( mesh, imageSize, labelNumber ); % Volume with background probs
# if 1
#   % Threshold background prior at 0.5 - this helps for atlases built from imperfect (i.e., automatic)
#   % segmentations, whereas background areas don't have zero probability for non-background structures
#   backgroundPrior( backgroundPrior > 2^8 ) = 2^16-1;
# end
# if( showFigures )
#   figure
#   subplot( 2, 2, 1 )
#   showImage( backgroundPrior )
#   subplot( 2, 2, 2 )
#   showImage( mosaicImages( 2^16 - 1 - double( backgroundPrior ), double( imageBuffers(:,:,:,1) ), 10 ) )
# end
# smoothedBackgroundPrior = kvlSmoothImageBuffer( backgroundPrior, modelSpecifications.brainMaskingSmoothingSigma ./ voxelSpacing );
# if( showFigures )
#   subplot( 2, 2, 3 )
#   showImage( smoothedBackgroundPrior )
# end
# % 65535 = 2^16 - 1. priors are stored as 16bit ints
# % To put the threshold in perspective: for Gaussian smoothing with a 3D isotropic kernel with variance
# % diag( sigma^2, sigma^2, sigma^2 ) a single binary "on" voxel at distance sigma results in a value of
# % 1/( sqrt(2*pi)*sigma )^3 * exp( -1/2 ).
# % More generally, a single binary "on" voxel at some Eucledian distance d results in a value of
# % 1/( sqrt(2*pi)*sigma )^3 * exp( -1/2*d^2/sigma^2 ). Turning this around, if we threshold this at some
# % value "t", a single binary "on" voxel will cause every voxel within Eucledian distance
# %
# %   d = sqrt( -2*log( t * ( sqrt(2*pi)*sigma )^3 ) * sigma^2 )
# %
# % of it to be included in the mask.
# %
# % As an example, for 1mm isotropic data, the choice of sigma=3 and t=0.01 yields ... complex value ->
# % actually a single "on" voxel will then not make any voxel survive, as the normalizing constant (achieved
# % at Mahalanobis distance zero) is already < 0.01
# brainMask = ( 1 - single( smoothedBackgroundPrior ) / 65535 ) > modelSpecifications.brainMaskingThreshold;
#
# % Crop to area covered by the mesh
# areaCoveredAlphas = [ zeros( size( alphas, 1 ), 1, 'single' ) ones( size( alphas, 1 ), 1, 'single' ) ];
# kvlSetAlphasInMeshNodes( mesh, areaCoveredAlphas );
# areaCoveredByMesh = kvlRasterizeAtlasMesh( mesh, imageSize, 1 );
# kvlSetAlphasInMeshNodes( mesh, alphas );
# brainMask = brainMask & ( areaCoveredByMesh > 0 );
#
#
# % Mask each of the inputs
# for contrastNumber = 1 : numberOfContrasts
#   imageBuffer = imageBuffers( :, :, :, contrastNumber );
#   imageBuffer( find( ~brainMask ) ) = 0;
#   imageBuffers( :, :, :, contrastNumber ) = imageBuffer;
#   % kvlSetImageBuffer( images( contrastNumber ), imageBuffers( :, :, :, contrastNumber ) );
# end
#
# if( showFigures )
#   subplot( 2, 2, 4 )
#   showImage( imageBuffers( :, :, :, 1 ) )
# end
#
#
#
# % Let's prepare for the bias field correction that is part of the imaging model. It assumes
# % an additive effect, whereas the MR physics indicate it's a multiplicative one - so we log
# % transform the data first. In order to do so, mask out zeros from
# % the images.
# % This removes any voxel where any contrast has a zero value
# % (messes up log)
# mask = true( imageSize ); % volume of ones within the mask
# for contrastNumber = 1 : numberOfContrasts
#   mask = mask .* ( imageBuffers( :, :, :, contrastNumber ) > 0 );
# end
# maskIndices = find( mask );
# for contrastNumber = 1 : numberOfContrasts
#   buffer = imageBuffers( :, :, :, contrastNumber );
#   buffer( maskIndices ) = log( buffer( maskIndices ) );
#   buffer = buffer .* mask;
#   imageBuffers( :, :, :, contrastNumber ) = buffer;
# end
#
#
#
# % Merge classes into "super-structures" that define a single Gaussian mixture model shared between the classes belonging
# % to the same super-structure
# FreeSurferLabels = modelSpecifications.FreeSurferLabels;
# names = modelSpecifications.names;
# colors = modelSpecifications.colors;
# [ reducedAlphas, reducedNames, reducedFreeSurferLabels, reducedColors, reducingLookupTable ] = ...
#                         kvlMergeAlphas( alphas, names, modelSpecifications.sharedGMMParameters, FreeSurferLabels, colors );
#
#
# if ( showFigures )
#   % Rasterizing a color-code prior can potentially be done a lot more efficient using the kvl::AtlasMeshSummaryDrawer class,
#   % which first determines the color in each mesh vertex and then interpolates that (only four colors to interpolate, so
#   % much more efficient). However, this requires yet another Matlab wrapper with a kvl::CompressionLookupTable to be
#   % created from the colors (or read from file), which is a bit tedious so I'm leaving it at that for now...
#   fprintf( 'Visualizing the atlas mesh; this takes quite some time and is only here for tutorial purposes...' )
#   priors = kvlRasterizeAtlasMesh( mesh, imageSize ); % Without specifying a specific label, will rasterize all simultaneously, rasterize everything
#   rgbBuffer = kvlColorCodeProbabilityImages( priors, colors );
#   figure
#   showImage( rgbBuffer )
#   clear priors rgbBuffer
#   fprintf( 'done\n' )
#   drawnow;
# end
#
#
#
#
# % The fact that we merge several neuroanatomical structures into "super"-structures for the purpose of model
# % parameter estimaton, but at the same time represent each of these super-structures with a mixture of Gaussians,
# % creates something of a messy situation when implementing this stuff. To avoid confusion, let's define a few
# % conventions that we'll closely follow in the code as follows:
# %
# %   - classNumber = 1 ... numberOfClasses  -> indexes a specific super-structure (there are numberOfClasses superstructures)
# %   - numberOfGaussiansPerClass            -> a numberOfClasses-dimensional vector that indicates the number of components
# %                                             in the Gaussian mixture model associated with each class
# %   - gaussianNumber = 1 .... numberOfGaussians  -> indexes a specific Gaussian distribution; there are
# %                                                   numberOfGaussians = sum( numberOfGaussiansPerClass ) of those in total
# %
# % In certain situations it will be easier to index things using "classNumber" (which is the only relevant thing when
# % estimating mesh deformations) than using "gaussianNumber" (which is the only relevant thing when estimating mixture
# % model parameters) and vice versa, so we need to have a standardized way of going back and forth between those. For a
# % given classNumber, there are numberOfComponents = numberOfGaussiansPerClass( classNumber ) components in its mixture
# % model. By convention we convert the pair ( classNumber, componentNumber ) into gaussianNumber as follows:
# %
# %    gaussianNumber = sum( numberOfGaussiansPerClass( 1 : classNumber-1 ) ) + componentNumber;
# %
# numberOfGaussiansPerClass = [ modelSpecifications.sharedGMMParameters.numberOfComponents ];
# numberOfClasses = length( numberOfGaussiansPerClass );
# numberOfGaussians = sum( numberOfGaussiansPerClass );
#
#
#
# % Our bias model is a linear combination of a set of basis functions. We are using so-called
# % "DCT-II" basis functions, i.e., the lowest few frequency components of the Discrete Cosine
# % Transform.
# %
# kroneckerProductBasisFunctions = cell( 0, 0 );
# numberOfBasisFunctions = zeros( 1, 3 );
# for dimensionNumber = 1 : 3
#   N = imageSize( dimensionNumber ); % Number of data points
#   delta =  modelSpecifications.biasFieldSmoothingKernelSize / voxelSpacing( dimensionNumber ); % Measured in voxels
#   M = ceil( N / delta ) + 1; % Number of frequencies to use
#   Nvirtual = ( M - 1 ) * delta; % Virtual (possibly non-integer) number of data points to make sure
#                                 % we reach the targeted smoothing kernel size
#   js = ( [ 0 : N-1 ]' + 1/2 ) * pi / Nvirtual;
#   A = cos( js * [ 0 : M-1 ] ) * sqrt( 2 / Nvirtual );
#   A( :, 1 ) = A( :, 1 ) / sqrt( 2 );
#
#   if showFigures
#     % Show smoothing kernel
#     figure
#     smootherMatrix = A * ( ( A' * A ) \ A' );
#     subplot( 2, 2, 1 )
#     imshow( smootherMatrix, [] )
#     subplotCounter = 2;
#     for rowNumber = round( [ N/4 N/2 3*N/4 ] )
#       subplot( 2, 2, subplotCounter )
#       plot( smootherMatrix( rowNumber, : ) )
#       set( gca, 'xlim', [ 1 N ] )
#       grid
#       hold on
#       ylim = get( gca, 'ylim' );
#       line( ( rowNumber ) * [ 1 1 ], ylim, 'color', 'k' )
#       line( ( rowNumber + delta ) * [ 1 1 ], ylim, 'color', 'r', 'linestyle', '--' )
#       line( ( rowNumber - delta ) * [ 1 1 ], ylim, 'color', 'r', 'linestyle', '--' )
#       subplotCounter = subplotCounter + 1;
#     end
#   end
#
#   kroneckerProductBasisFunctions{ dimensionNumber } = A;
#   numberOfBasisFunctions( dimensionNumber ) = M;
# end
# biasFieldCoefficients = zeros( prod( numberOfBasisFunctions ), numberOfContrasts ); % No bias field to start with
#
#
# if ( showFigures )
#   posteriorFigure = figure;
#   costFigure = figure;
#   deformationMovieFigure = figure;
#   biasFieldFigure = figure;
# end
#

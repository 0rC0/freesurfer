import logging

import GEMS2Python
import numpy as np
import scipy.io

from as_python.samseg.bias_correction import backprojectKroneckerProductBasisFunctions, \
    projectKroneckerProductBasisFunctions, computePrecisionOfKroneckerProductBasisFunctions
from as_python.samseg.kvlWarpMesh import kvlWarpMesh
from as_python.samseg.kvl_merge_alphas import kvlMergeAlphas

logger = logging.getLogger(__name__)

eps = np.finfo(float).eps


def require_np_array(np_array):
    return np.require(np_array, requirements=['F_CONTIGUOUS', 'ALIGNED'])


def load_mat_file(filename):
    return scipy.io.loadmat(filename, struct_as_record=False, squeeze_me=True)


def ensure_dims(np_array, dims):
    if np_array.ndim < dims:
        return np.expand_dims(np_array, axis=dims)
    elif np_array.ndim == dims:
        return np_array


def samsegment_part2(
        modelSpecifications,
        optimizationOptions,
        part1_results_dict,
        checkpoint_manager=None
):
    biasFieldCoefficients = part1_results_dict['biasFieldCoefficients']
    colors = part1_results_dict['colors']
    FreeSurferLabels = part1_results_dict['FreeSurferLabels']
    imageBuffers = part1_results_dict['imageBuffers']
    kroneckerProductBasisFunctions = part1_results_dict['kroneckerProductBasisFunctions']
    mask = part1_results_dict['mask']
    names = part1_results_dict['names']
    numberOfBasisFunctions = part1_results_dict['numberOfBasisFunctions']
    numberOfClasses = part1_results_dict['numberOfClasses']
    numberOfContrasts = part1_results_dict['numberOfContrasts']
    numberOfGaussians = part1_results_dict['numberOfGaussians']
    numberOfGaussiansPerClass = part1_results_dict['numberOfGaussiansPerClass']
    transformMatrix = part1_results_dict['transformMatrix']
    voxelSpacing = part1_results_dict['voxelSpacing']
    transform = GEMS2Python.KvlTransform(np.asfortranarray(transformMatrix))

    # function Y = backprojectKroneckerProductBasisFunctions( kroneckerProductBasisFunctions, coefficients )

    # function coefficients = projectKroneckerProductBasisFunctions( kroneckerProductBasisFunctions, T )
    # computePrecisionOfKroneckerProductBasisFunctions( kroneckerProductBasisFunctions, B )

    # % We do the optimization in a multi-resolution type of scheme, where large
    # % deformations are quickly found using smoothed versions of the atlas mesh, and the fine
    # % details are then found on gradually less smoothed versions until the original atlas mesh is used for the optimization.
    # % Multi-resolution is a standard procedure in image registration: the initial
    # % blurring avoids getting stuck in the first local optimum of the search space, and get the rough major
    # % deformations right instead.
    # numberOfMultiResolutionLevels = length( optimizationOptions.multiResolutionSpecification );
    numberOfMultiResolutionLevels = len(optimizationOptions.multiResolutionSpecification)
    # historyWithinEachMultiResolutionLevel = struct( [] );
    historyWithinEachMultiResolutionLevel = []
    # for multiResolutionLevel = 1 : numberOfMultiResolutionLevels
    for multiResolutionLevel in range(numberOfMultiResolutionLevels):
        logger.debug('multiResolutionLevel=%d', multiResolutionLevel)
        #
        #   %
        #   maximumNumberOfIterations = optimizationOptions.multiResolutionSpecification( multiResolutionLevel ).maximumNumberOfIterations;
        maximumNumberOfIterations = optimizationOptions.multiResolutionSpecification[
            multiResolutionLevel].maximumNumberOfIterations
        #   estimateBiasField = optimizationOptions.multiResolutionSpecification( multiResolutionLevel ).estimateBiasField;
        estimateBiasField = optimizationOptions.multiResolutionSpecification[multiResolutionLevel].estimateBiasField
        #   historyOfCost = [ 1/eps ];
        historyOfCost = [1 / eps]
        #   historyOfMaximalDeformationApplied = [];
        #   historyOfTimeTakenIntensityParameterUpdating = [];
        #   historyOfTimeTakenDeformationUpdating = [];
        #   fprintf('maximumNumberOfIterations %d\n',maximumNumberOfIterations);
        logger.debug('maximumNumberOfIterations: %d', maximumNumberOfIterations)
        #
        #
        #   % Downsample the images, the mask, the mesh, and the bias field basis functions
        #   % Must be integer
        #   downSamplingFactors = max( round( optimizationOptions.multiResolutionSpecification( multiResolutionLevel ).targetDownsampledVoxelSpacing ...
        #                                     ./ voxelSpacing ), [ 1 1 1 ] )
        downSamplingFactors = np.uint32(np.round(optimizationOptions.multiResolutionSpecification[
                                                     multiResolutionLevel].targetDownsampledVoxelSpacing / voxelSpacing))
        downSamplingFactors[downSamplingFactors < 1] = 1
        #   downSampledMask = mask(  1 : downSamplingFactors( 1 ) : end, ...
        #                            1 : downSamplingFactors( 2 ) : end, ...
        #                            1 : downSamplingFactors( 3 ) : end );
        downSampledMask = mask[::downSamplingFactors[0], ::downSamplingFactors[1], ::downSamplingFactors[2]]
        #   downSampledMaskIndices = find( downSampledMask );
        downSampledMaskIndices = np.where(downSampledMask)
        activeVoxelCount = len(downSampledMaskIndices[0])
        #   downSampledImageBuffers = [];
        downSampledImageBuffers = np.zeros(downSampledMask.shape + (numberOfContrasts,))
        #   for contrastNumber = 1 : numberOfContrasts
        for contrastNumber in range(numberOfContrasts):
            logger.debug('first time contrastNumber=%d', contrastNumber)
            #     if true
            #       % No image smoothing
            #       downSampledImageBuffers( :, :, :, contrastNumber ) = imageBuffers( 1 : downSamplingFactors( 1 ) : end, ...
            #                                                                          1 : downSamplingFactors( 2 ) : end, ...
            #                                                                          1 : downSamplingFactors( 3 ) : end, ...
            #                                                                          contrastNumber );
            # TODO: Remove need to check this. Matlab implicitly lets you expand one dim, our python code should have the shape (x, y, z, numberOfContrasts)
            if imageBuffers.ndim == 3:
                imageBuffers = np.expand_dims(imageBuffers, axis=3)

            downSampledImageBuffers[:, :, :, contrastNumber] = imageBuffers[::downSamplingFactors[0],
                                                               ::downSamplingFactors[1],
                                                               ::downSamplingFactors[2],
                                                               contrastNumber]
            #     else
            #       % Try image smoothing
            #       buffer = imageBuffers( 1 : downSamplingFactors( 1 ) : end, ...
            #                              1 : downSamplingFactors( 2 ) : end, ...
            #                              1 : downSamplingFactors( 3 ) : end, ...
            #                              contrastNumber );
            #       smoothingSigmas = downSamplingFactors / 2 / sqrt( 2 * log( 2 ) ); % Variance chosen to approximately
            #                                                                         % match normalized binomial filter
            #                                                                         % (1/4, 1/2, 1/4) for downsampling
            #                                                                         % factor of 2
            #       smoothingSigmas( find( downSamplingFactors == 1 ) ) = 0.0;
            #       smoothedBuffer = kvlSmoothImageBuffer( single( buffer ), smoothingSigmas );
            #       smoothedMask = kvlSmoothImageBuffer( single( downSampledMask ), smoothingSigmas );
            #       downSampledImageBuffers( :, :, :, contrastNumber ) = downSampledMask .* ( smoothedBuffer ./ ( smoothedMask + eps ) );
            #     end
            #
            #   end
        #
        #   %
        #   downSampledKroneckerProductBasisFunctions = cell( 0, 0 );
        #   for dimensionNumber = 1 : 3
        #     A = kroneckerProductBasisFunctions{ dimensionNumber };
        #     downSampledKroneckerProductBasisFunctions{ dimensionNumber } = A( 1 : downSamplingFactors( dimensionNumber ) : end, : );
        #   end
        downSampledKroneckerProductBasisFunctions = [np.array(kroneckerProductBasisFunction[::downSamplingFactor])
                                                     for kroneckerProductBasisFunction, downSamplingFactor in
                                                     zip(kroneckerProductBasisFunctions, downSamplingFactors)]
        #   downSampledImageSize = size( downSampledImageBuffers( :, :, :, 1 ) );
        downSampledImageSize = downSampledImageBuffers[:, :, :, 0].shape
        #
        #
        #   % Read the atlas mesh to be used for this multi-resolution level, taking into account the downsampling to position it
        #   % correctly
        #   downSamplingTransformMatrix = diag( [ 1./downSamplingFactors 1 ] );
        downSamplingTransformMatrix = np.diag(1. / downSamplingFactors)
        downSamplingTransformMatrix = np.pad(downSamplingTransformMatrix, (0, 1), mode='constant', constant_values=0)
        downSamplingTransformMatrix[3][3] = 1

        # totalTransformationMatrix = downSamplingTransformMatrix @ kvlGetTransformMatrix( transform )
        totalTransformationMatrix = downSamplingTransformMatrix @ transform.as_numpy_array

        #   meshCollection = ...
        #         kvlReadMeshCollection( optimizationOptions.multiResolutionSpecification( multiResolutionLevel ).atlasFileName, ...
        #                                 kvlCreateTransform( totalTransformationMatrix ), modelSpecifications.K );
        mesh_collection = GEMS2Python.KvlMeshCollection()
        mesh_collection.read(optimizationOptions.multiResolutionSpecification[multiResolutionLevel].atlasFileName)
        mesh_collection.k = modelSpecifications.K
        mesh_collection.transform(GEMS2Python.KvlTransform(require_np_array(totalTransformationMatrix)))

        #   mesh = kvlGetMesh( meshCollection, -1 );
        mesh = mesh_collection.reference_mesh

        #
        #   % Get the initial mesh node positions, also transforming them back into template space
        #   % (i.e., undoing the affine registration that we applied) for later usage
        #   initialNodePositions = kvlGetMeshNodePositions( mesh );
        initialNodePositions = mesh.points
        #   numberOfNodes = size( initialNodePositions, 1 );
        numberOfNodes = len(initialNodePositions)
        #   tmp = ( totalTransformationMatrix \ [ initialNodePositions ones( numberOfNodes, 1 ) ]' )';
        tmp = np.linalg.solve(totalTransformationMatrix,
                              np.pad(initialNodePositions, ((0, 0), (0, 1)), mode='constant', constant_values=1).T).T
        #   initialNodePositionsInTemplateSpace = tmp( :, 1 : 3 );
        initialNodePositionsInTemplateSpace = tmp[:, 0:3]
        #
        #
        #   % If this is not the first multi-resolution level, apply the warp computed during the previous level
        #   if ( multiResolutionLevel > 1 )
        if multiResolutionLevel > 0:
            logger.debug('starting multiResolutionLevel=%d', multiResolutionLevel)
            #     % Get the warp in template space
            #     nodeDeformationInTemplateSpaceAtPreviousMultiResolutionLevel = ...
            #             historyWithinEachMultiResolutionLevel( multiResolutionLevel-1 ).finalNodePositionsInTemplateSpace - ...
            #             historyWithinEachMultiResolutionLevel( multiResolutionLevel-1 ).initialNodePositionsInTemplateSpace;
            #     initialNodeDeformationInTemplateSpace = kvlWarpMesh( ...
            #                   optimizationOptions.multiResolutionSpecification( multiResolutionLevel-1 ).atlasFileName, ...
            #                   nodeDeformationInTemplateSpaceAtPreviousMultiResolutionLevel, ...
            #                   optimizationOptions.multiResolutionSpecification( multiResolutionLevel ).atlasFileName );

            [initialNodeDeformationInTemplateSpace, initial_averageDistance, initial_maximumDistance] = kvlWarpMesh(
                optimizationOptions.multiResolutionSpecification[multiResolutionLevel - 1].atlasFileName,
                nodeDeformationInTemplateSpaceAtPreviousMultiResolutionLevel,
                optimizationOptions.multiResolutionSpecification[multiResolutionLevel].atlasFileName)

            logger.debug('alpha multiResolutionLevel={0}'.format(multiResolutionLevel))

            #     % Apply this warp on the mesh node positions in template space, and transform into current space
            #     desiredNodePositionsInTemplateSpace = initialNodePositionsInTemplateSpace + initialNodeDeformationInTemplateSpace;
            logger.debug('beta multiResolutionLevel=%d', multiResolutionLevel)
            desiredNodePositionsInTemplateSpace = initialNodePositionsInTemplateSpace + initialNodeDeformationInTemplateSpace
            #     tmp = ( totalTransformationMatrix * ...
            #             [ desiredNodePositionsInTemplateSpace ones( numberOfNodes, 1 ) ]' )';

            logger.debug('gamma multiResolutionLevel=%d', multiResolutionLevel)
            tmp = (totalTransformationMatrix @ np.pad(desiredNodePositionsInTemplateSpace, ((0, 0), (0, 1)), 'constant',
                                                      constant_values=1).T).T
            #     desiredNodePositions = tmp( :, 1 : 3 );
            logger.debug('delta multiResolutionLevel=%d}', multiResolutionLevel)
            desiredNodePositions = tmp[:, 0:3]
            logger.debug('epsilon multiResolutionLevel=%d', multiResolutionLevel)
            #
            #     %
            #     kvlSetMeshNodePositions( mesh, desiredNodePositions );
            mesh.points = require_np_array(desiredNodePositions)
            logger.debug('finish multiResolutionLevel=%d', multiResolutionLevel)
            if checkpoint_manager:
                checkpoint_manager.increment_and_save({
                    'desiredNodePositions': desiredNodePositions,
                    'tmp': tmp,
                    'desiredNodePositionsInTemplateSpace': desiredNodePositionsInTemplateSpace,
                    'nodeDeformationInTemplateSpaceAtPreviousMultiResolutionLevel': nodeDeformationInTemplateSpaceAtPreviousMultiResolutionLevel,
                    'initialNodeDeformationInTemplateSpace': initialNodeDeformationInTemplateSpace,
                }, 'multiresWarp')
                # fixture = checkpoint_manager.load('multiresWarp')
            #
            #   end
        #
        #
        #
        #   % Set priors in mesh to the reduced (super-structure) ones
        #   alphas = kvlGetAlphasInMeshNodes( mesh );
        alphas = mesh.alphas
        #   reducedAlphas = kvlMergeAlphas( alphas, names, modelSpecifications.sharedGMMParameters, FreeSurferLabels, colors );
        reducedAlphas, _, _, _, _ = kvlMergeAlphas(alphas, names, modelSpecifications.sharedGMMParameters,
                                                   FreeSurferLabels, colors);
        if checkpoint_manager:
            checkpoint_manager.increment_and_save({'reducedAlphas': reducedAlphas}, 'reducedAlphas')
        # reducedAlphas = checkpoint_manager.load('reducedAlphas')['reducedAlphas']
        #   kvlSetAlphasInMeshNodes( mesh, reducedAlphas )
        mesh.alphas = reducedAlphas

        #   % Algorithm-wise, we're just estimating sets of parameters for one given data (MR scan) that is
        #   % known and fixed throughout. However, in terms of bias field correction it will be computationally
        #   % more efficient to pre-compute the bias field corrected version of the scan ("corrected" with
        #   % the current estimate of the bias field) once and pass that on to different routines instead of the
        #   % original data.
        #   % For convenience (although potentially a recipe for future bug introduction), I'm also keeping a
        #   % vectorized form of that around -- this will be useful in various places in the EM-parts. So
        #   % effectively I have two redundant variables "downSampledBiasCorrectedImageBuffers" and "biasCorrectedData"
        #   % that really just encode the variable "biasFieldCoefficients" and so need to be meticiously updated each time
        #   % "biasFieldCoefficients" is updated (!)
        #   downSampledBiasCorrectedImageBuffers = zeros( [ downSampledImageSize numberOfContrasts ] );
        downSampledBiasCorrectedImageBuffers = np.zeros(downSampledImageSize + (numberOfContrasts,))
        #   biasCorrectedData = zeros( [ length( downSampledMaskIndices ) numberOfContrasts ] );
        biasCorrectedData = np.zeros((activeVoxelCount, numberOfContrasts))

        #   for contrastNumber = 1 : numberOfContrasts
        # TODO: remove this ensure_dims when part 1 is done
        biasFieldCoefficients = ensure_dims(biasFieldCoefficients, 2)
        for contrastNumber in range(numberOfContrasts):
            logger.debug('second time contrastNumber=%d', contrastNumber)
            #     downSampledBiasField = backprojectKroneckerProductBasisFunctions( downSampledKroneckerProductBasisFunctions, biasFieldCoefficients( :, contrastNumber ) );
            downSampledBiasField = backprojectKroneckerProductBasisFunctions(downSampledKroneckerProductBasisFunctions,
                                                                             biasFieldCoefficients[:, contrastNumber])
            #     tmp = downSampledImageBuffers( :, :, :, contrastNumber ) - downSampledBiasField .* downSampledMask;
            tmp = downSampledImageBuffers[:, :, :, contrastNumber] - downSampledBiasField * downSampledMask
            #     downSampledBiasCorrectedImageBuffers( :, :, :, contrastNumber ) = tmp;
            downSampledBiasCorrectedImageBuffers[:, :, :, contrastNumber] = tmp
            #     biasCorrectedData( :, contrastNumber ) = tmp( downSampledMaskIndices );
            biasCorrectedData[:, contrastNumber] = tmp[downSampledMaskIndices]
            #   end
        #
        #
        #   % Compute a color coded version of the atlas prior in the atlas's current pose, i.e., *before*
        #   % we start deforming. We'll use this just for visualization purposes
        #   if ( showFigures )
        #     oldColorCodedPriors = kvlColorCodeProbabilityImages( kvlRasterizeAtlasMesh( mesh, downSampledImageSize ), reducedColors );
        #   end
        #
        #
        #
        #   historyWithinEachIteration = struct( [] );
        historyWithinEachIteration = []
        #   priors = zeros( length( downSampledMaskIndices ), numberOfClasses );
        priors = np.zeros((activeVoxelCount, numberOfClasses))
        #   posteriors = zeros( length( downSampledMaskIndices ), numberOfGaussians ); % Gaussian mixture models burst out into
        #                                                                              % individual Gaussian components
        posteriors = np.zeros((activeVoxelCount, numberOfGaussians))
        #
        #   % Easier to work with vector notation in the EM computations
        #   % reshape into a matrix
        #   data = zeros( [ length( downSampledMaskIndices ) numberOfContrasts ] );
        data = np.zeros((activeVoxelCount, numberOfContrasts))
        #   for contrastNumber = 1:numberOfContrasts
        for contrastNumber in range(numberOfContrasts):
            #     tmp = reshape( downSampledImageBuffers( :, :, :, contrastNumber ), [ prod(downSampledImageSize) 1 ] );
            tmp = downSampledImageBuffers[:, :, :, contrastNumber]
            #     data( :, contrastNumber ) = tmp( downSampledMaskIndices );
            data[:, contrastNumber] = tmp[downSampledMaskIndices]
            #   end
        #
        #   % Main iteration loop over both EM and deformation
        #   for iterationNumber = 1 : maximumNumberOfIterations
        for iterationNumber in range(maximumNumberOfIterations):
            logger.debug('iterationNumber=%d', iterationNumber)
            #     %
            #     startTimeIntensityParameterUpdating = tic;
            #
            #     %
            #     % Part I: estimate Gaussian mixture model parameters, as well as bias field parameters using EM.
            #     %
            #
            #     % Get the priors at the current mesh position
            #     tmp = reshape( kvlRasterizeAtlasMesh( mesh, downSampledImageSize ), [ prod( downSampledImageSize ) numberOfClasses ] );
            tmp = mesh.rasterize(downSampledImageSize, -1)
            #     priors( : ) = double( tmp( downSampledMaskIndices, : ) ) / 65535;
            priors = tmp[downSampledMaskIndices] / 65535
            #
            #     if ( iterationNumber == 1 )
            #       historyWithinEachMultiResolutionLevel( multiResolutionLevel ).priorsAtStart = priors;
            #     end
            #
            #
            #     % Start EM iterations.
            #     if ( ( multiResolutionLevel == 1 ) && ( iterationNumber == 1 ) )
            if ((multiResolutionLevel == 0) and (iterationNumber == 0)):
                #
                #       % Initialize the mixture parameters if this is the first time ever you run this
                #       means = zeros( numberOfGaussians, numberOfContrasts );
                means = np.zeros((numberOfGaussians, numberOfContrasts))
                #       variances = zeros( numberOfGaussians, numberOfContrasts, numberOfContrasts );
                variances = np.zeros((numberOfGaussians, numberOfContrasts, numberOfContrasts))
                #       mixtureWeights = zeros( numberOfGaussians, 1 );
                mixtureWeights = np.zeros((numberOfGaussians, 1))
                #       for classNumber = 1 : numberOfClasses
                for classNumber in range(numberOfClasses):
                    #         % Calculate the global weighted mean and variance of this class, where the weights are given by the prior
                    #         prior = priors( :, classNumber );
                    prior = priors[:, classNumber]
                    #         mean = data' * prior / sum( prior );
                    mean = data.T @ prior / np.sum(prior)
                    #         tmp = data - repmat( mean', [ size( data, 1 ) 1 ] );
                    tmp = data - mean
                    #         variance = tmp' * ( tmp .* repmat( prior, [ 1 numberOfContrasts ] ) ) / sum( prior );
                    prior = np.expand_dims(prior, 1)
                    variance = tmp.T @ (tmp * prior) / np.sum(prior)
                    #         if modelSpecifications.useDiagonalCovarianceMatrices
                    if modelSpecifications.useDiagonalCovarianceMatrices:
                        #           % Force diagonal covariance matrices
                        #           variance = diag( diag( variance ) );
                        variance = np.diag(np.diag(variance))
                        #         end
                    #
                    #
                    #         % Based on this, initialize the mean and variance of the individual Gaussian components in this class'
                    #         % mixture model: variances are simply copied from the global class variance, whereas the means are
                    #         % determined by splitting the [ mean-sqrt( variance ) mean+sqrt( variance ) ] domain into equal intervals,
                    #         % the middle of which are taken to be the means of the Gaussians. Mixture weights are initialized to be
                    #         % all equal.
                    #         %
                    #         % This actually creates a mixture model that mimics the single Gaussian quite OK-ish: to visualize this
                    #         % do e.g.,
                    #         %
                    #         %  for numberOfComponents = 1 : 7
                    #         %    intervalSize = 2 / numberOfComponents;
                    #         %    means = -1 + intervalSize/2 + [ 0 : numberOfComponents-1 ] * intervalSize;
                    #         %    figure
                    #         %    x = [ -6 : .1 : 6 ];
                    #         %    gauss = exp( -x.^2/2 );
                    #         %    plot( gauss )
                    #         %    hold on
                    #         %    mixture = zeros( size( x ) );
                    #         %    for i = 1 : numberOfComponents
                    #         %      gauss = exp( -( x - means( i ) ).^2/2 );
                    #         %      plot( gauss / numberOfComponents, 'g' )
                    #         %      mixture = mixture + gauss / numberOfComponents;
                    #         %    end
                    #         %    plot( mixture, 'r' )
                    #         %    grid
                    #         %    title( [ num2str( numberOfComponents ) ' components' ] )
                    #         %  end
                    #         %
                    #         numberOfComponents = numberOfGaussiansPerClass( classNumber );
                    numberOfComponents = numberOfGaussiansPerClass[classNumber]

                    #         for componentNumber = 1 : numberOfComponents
                    for componentNumber in range(numberOfComponents):
                        #           gaussianNumber = sum( numberOfGaussiansPerClass( 1 : classNumber-1 ) ) + componentNumber;
                        gaussianNumber = sum(numberOfGaussiansPerClass[: classNumber]) + componentNumber
                        #
                        #           variances( gaussianNumber, :, : ) = variance;
                        variances[gaussianNumber, :, :] = variance
                        #           intervalSize = 2 * sqrt( diag( variance ) ) / numberOfComponents;
                        intervalSize = 2 * np.sqrt(np.diag(variance)) / numberOfComponents
                        #           means( gaussianNumber, : ) = ( mean - sqrt( diag( variance ) ) + intervalSize/2 + ( componentNumber - 1 ) * intervalSize )';
                        means[gaussianNumber, :] = (mean - np.sqrt(np.diag(variance)) + intervalSize / 2 + (
                            componentNumber) * intervalSize).T
                        #           mixtureWeights( gaussianNumber ) = 1 / numberOfComponents;
                        mixtureWeights[gaussianNumber] = 1 / numberOfComponents
                        #         end
                        #
                        #       end % End loop over classes
            #
            #
            #       % Also remember the overall data variance for later usage in a conjugate prior on the variances
            #       dataMean = sum( data )' / size( data, 1 );
            dataMean = np.mean(data)
            #       tmp = data - repmat( dataMean', [ size( data, 1 ) 1 ] );
            tmp = data - dataMean
            #       dataVariance = diag( diag( tmp' * tmp ) ) / size( data, 1 );
            dataVariance = np.var(tmp)
            #       numberOfPseudoMeasurementsOfWishartPrior = 1; % In Oula's code this was effectively 2 * ( numberOfContrasts + 2 )
            #                                                     % although I have no clue why
            numberOfPseudoMeasurementsOfWishartPrior = 1
            #       pseudoVarianceOfWishartPrior = dataVariance / numberOfPseudoMeasurementsOfWishartPrior;
            pseudoVarianceOfWishartPrior = dataVariance / numberOfPseudoMeasurementsOfWishartPrior
            #
            #     end % End test need for initialization
            #
            #     % stopCriterionEM = 1e-5;
            #     historyOfEMCost = [ 1/eps ];
            historyOfEMCost = [1 / eps]
            #     for EMIterationNumber = 1 : 100
            for EMIterationNumber in range(100):
                logger.debug('EMIterationNumber=%d', EMIterationNumber)
                #       %
                #       % E-step: compute the posteriors based on the current parameters.
                #       %
                #       for classNumber = 1 : numberOfClasses
                for classNumber in range(numberOfClasses):
                    #         prior = priors( :, classNumber );
                    prior = priors[:, classNumber]
                    #
                    #         numberOfComponents = numberOfGaussiansPerClass( classNumber );
                    numberOfComponents = numberOfGaussiansPerClass[classNumber]
                    #         for componentNumber = 1 : numberOfComponents
                    for componentNumber in range(numberOfComponents):
                        #           gaussianNumber = sum( numberOfGaussiansPerClass( 1 : classNumber-1 ) ) + componentNumber;
                        gaussianNumber = sum(numberOfGaussiansPerClass[:classNumber]) + componentNumber
                        #
                        #           mean = means( gaussianNumber, : )';
                        mean = means[gaussianNumber, :].T
                        #           variance = squeeze( variances( gaussianNumber, :, : ) );
                        variance = variances[gaussianNumber, :, :]
                        #           L = chol( variance, 'lower' );  % variance = L * L'
                        L = np.linalg.cholesky(variance)
                        #           tmp = L \ ( biasCorrectedData' - repmat( mean, [ 1 size( biasCorrectedData, 1 ) ] ) );
                        tmp = np.linalg.solve(L, biasCorrectedData.T - mean)
                        #           squaredMahalanobisDistances = ( sum( tmp.^2, 1 ) )';
                        squaredMahalanobisDistances = (np.sum(tmp ** 2, axis=0)).T
                        #           sqrtDeterminantOfVariance = prod( diag( L ) ); % Same as sqrt( det( variance ) )
                        sqrtDeterminantOfVariance = np.prod(np.diag(L))
                        #           gaussianLikelihoods = exp( -squaredMahalanobisDistances / 2 ) / ( 2 * pi )^( numberOfContrasts / 2 ) / sqrtDeterminantOfVariance;
                        gaussianLikelihoods = np.exp(-squaredMahalanobisDistances / 2) / (2 * np.pi) ** (
                                numberOfContrasts / 2) / sqrtDeterminantOfVariance
                        #
                        #           posteriors( :, gaussianNumber ) = gaussianLikelihoods .* ( mixtureWeights( gaussianNumber ) * prior );
                        posteriors[:, gaussianNumber] = gaussianLikelihoods * (mixtureWeights[gaussianNumber] * prior)
                        #
                        #         end % End loop over mixture components
                    #
                    #       end % End loop over classes
                #       normalizer = sum( posteriors, 2 ) + eps;
                normalizer = np.sum(posteriors, axis=1) + eps
                #       if 0
                #         x = zeros( downSampledImageSize );
                #         x( downSampledMaskIndices ) = -log( normalizer );
                #         figure
                #         showImage( x )
                #       end
                #       posteriors = posteriors ./ repmat( normalizer, [ 1 numberOfGaussians ] );
                posteriors = posteriors / np.expand_dims(normalizer, 1)

                #       minLogLikelihood = -sum( log( normalizer ) );
                minLogLikelihood = -np.sum(np.log(normalizer))
                #       intensityModelParameterCost = 0;
                intensityModelParameterCost = 0
                #       for gaussianNumber = 1 : numberOfGaussians
                for gaussianNumber in range(numberOfGaussians):
                    #         variance = squeeze( variances( gaussianNumber, :, : ) );
                    variance = variances[gaussianNumber, :, :]
                    #
                    #         % Evaluate unnormalized Wishart distribution (conjugate prior on precisions) with parameters
                    #         %
                    #         %   scale matrix V = inv( pseudoVarianceOfWishartPrior * numberOfPseudoMeasurementsOfWishartPrior )
                    #         %
                    #         % and
                    #         %
                    #         %   degrees of freedom n = numberOfPseudoMeasurementsOfWishartPrior + numberOfContrasts + 1
                    #         %
                    #         % which has pseudoVarianceOfWishartPrior as the MAP solution in the absence of any data
                    #         %
                    #         minLogUnnormalizedWishart = ...
                    #             trace( variance \ pseudoVarianceOfWishartPrior ) * numberOfPseudoMeasurementsOfWishartPrior / 2 + ...
                    #             numberOfPseudoMeasurementsOfWishartPrior / 2 * log( det( variance ) );
                    minLogUnnormalizedWishart = \
                        np.trace(np.linalg.solve(variance, np.array(pseudoVarianceOfWishartPrior).reshape(1,
                                                                                                          1))) * numberOfPseudoMeasurementsOfWishartPrior / 2 + \
                        numberOfPseudoMeasurementsOfWishartPrior / 2 * np.log(np.linalg.det(variance))
                    #         intensityModelParameterCost = intensityModelParameterCost + minLogUnnormalizedWishart;
                    intensityModelParameterCost = intensityModelParameterCost + minLogUnnormalizedWishart
                    #       end
                #       historyOfEMCost = [ historyOfEMCost; minLogLikelihood + intensityModelParameterCost ];
                historyOfEMCost.append(minLogLikelihood + intensityModelParameterCost)
                #
                #       % Show some figures
                #       if ( showFigures )
                #         for classNumber = 1 : numberOfClasses
                #           posterior = zeros( downSampledImageSize );
                #           numberOfComponents = numberOfGaussiansPerClass( classNumber );
                #           for componentNumber = 1 : numberOfComponents
                #             gaussianNumber = sum( numberOfGaussiansPerClass( 1 : classNumber-1 ) ) + componentNumber;
                #             posterior( downSampledMaskIndices ) = posterior( downSampledMaskIndices ) + ...
                #                                                   posteriors( :, gaussianNumber );
                #           end
                #           figure( posteriorFigure )
                #           subplot( floor( sqrt( numberOfClasses ) ), ...
                #                    ceil( numberOfClasses / floor( sqrt( numberOfClasses ) ) ), ...
                #                    classNumber )
                #           showImage( posterior )
                #         end
                #         clear posterior
                #
                #         figure( costFigure )
                #         subplot( 2, 1, 1 )
                #         plot( historyOfEMCost( 2 : end ) )
                #         title( 'EM cost' )
                #         subplot(2, 1, 2 )
                #         plot( historyOfCost( 2 : end ) )
                #         title( 'Cost' )
                #
                #         figure( biasFieldFigure )
                #         for contrastNumber = 1 : numberOfContrasts
                #           subplot( numberOfContrasts, 2, ( contrastNumber - 1 ) * numberOfContrasts + 1 )
                #           showImage( exp( downSampledBiasCorrectedImageBuffers( :, :, :, contrastNumber ) ) );
                #           subplot( numberOfContrasts, 2, ( contrastNumber - 1 ) * numberOfContrasts + 2 )
                #           downSampledBiasField = backprojectKroneckerProductBasisFunctions( downSampledKroneckerProductBasisFunctions, ...
                #                                                                             biasFieldCoefficients( :, contrastNumber ) );
                #           showImage( exp( downSampledBiasField ) .* downSampledMask )
                #         end
                #         drawnow
                #
                #       end % End test if we need to show some figures
                #
                #
                #       % Check for convergence
                #       % relativeChangeCost = ( historyOfEMCost(end-1) - historyOfEMCost(end) ) /  historyOfEMCost(end)
                #       % if ( relativeChangeCost < stopCriterionEM )
                #       changeCostPerVoxel = ( historyOfEMCost(end-1) - historyOfEMCost(end) ) / length( downSampledMaskIndices );
                #       if ( changeCostPerVoxel < optimizationOptions.absoluteCostPerVoxelDecreaseStopCriterion )
                # if changeCostPerVoxel < optimizationOptions.absoluteCostPerVoxelDecreaseStopCriterion:
                priorEMCost = historyOfEMCost[-2]
                currentEMCost = historyOfEMCost[-1]
                costChangeEM = priorEMCost - currentEMCost
                changeCostEMPerVoxel = costChangeEM / activeVoxelCount
                changeCostEMPerVoxelThreshold = optimizationOptions.absoluteCostPerVoxelDecreaseStopCriterion
                if changeCostEMPerVoxel < changeCostEMPerVoxelThreshold:
                    #         % Converged
                    #         disp( 'EM converged!' )
                    print('EM converged!')
                    #         break;
                    if checkpoint_manager:
                        checkpoint_manager.increment_and_save({
                            'activeVoxelCount': activeVoxelCount,
                            'priorEMCost': priorEMCost,
                            'currentEMCost': currentEMCost,
                            'costChangeEM': costChangeEM,
                            'changeCostEMPerVoxel': changeCostEMPerVoxel,
                            'changeCostEMPerVoxelThreshold': changeCostEMPerVoxelThreshold,
                            'minLogLikelihood': minLogLikelihood,
                            'intensityModelParameterCost': intensityModelParameterCost,
                        }, 'optimizerEmExit')
                    break
                    #       end
                #       %
                #       % M-step: update the model parameters based on the current posterior
                #       %
                #       % First the mixture model parameters
                #       for gaussianNumber = 1 : numberOfGaussians
                for gaussianNumber in range(numberOfGaussians):
                    #         posterior = posteriors( :, gaussianNumber );
                    posterior = posteriors[:, gaussianNumber]
                    posterior = posterior.reshape(-1, 1)
                    #
                    #         mean = biasCorrectedData' * posterior ./ sum( posterior );
                    mean = biasCorrectedData.T @ posterior / np.sum(posterior)
                    #         tmp = biasCorrectedData - repmat( mean', [ size( biasCorrectedData, 1 ) 1 ] );
                    tmp = biasCorrectedData - mean.T
                    #         %variance = ( tmp' * ( tmp .* repmat( posterior, [ 1 numberOfContrasts ] ) ) + dataVariance ) ...
                    #         %            / ( 2 * ( numberOfContrasts + 2 ) + sum( posterior ) );
                    #         variance = ( tmp' * ( tmp .* repmat( posterior, [ 1 numberOfContrasts ] ) ) + ...
                    #                                 pseudoVarianceOfWishartPrior * numberOfPseudoMeasurementsOfWishartPrior ) ...
                    #                     / ( sum( posterior ) + numberOfPseudoMeasurementsOfWishartPrior );
                    variance = (tmp.T @ (tmp * posterior) + \
                                pseudoVarianceOfWishartPrior * numberOfPseudoMeasurementsOfWishartPrior) \
                               / (np.sum(posterior) + numberOfPseudoMeasurementsOfWishartPrior)
                    #         if modelSpecifications.useDiagonalCovarianceMatrices
                    if modelSpecifications.useDiagonalCovarianceMatrices:
                        #           % Force diagonal covariance matrices
                        #           variance = diag( diag( variance ) );
                        variance = np.diag(np.diag(variance));
                        #         end
                    #
                    #         variances( gaussianNumber, :, : ) = variance;
                    variances[gaussianNumber, :, :] = variance
                    #         means( gaussianNumber, : ) = mean';
                    means[gaussianNumber, :] = mean.T
                #
                #       end
                #       mixtureWeights = sum( posteriors + eps )';
                mixtureWeights = np.sum(posteriors + eps, axis=0).T
                #       for classNumber = 1 : numberOfClasses
                for classNumber in range(numberOfClasses):
                    #         % mixture weights are normalized (those belonging to one mixture sum to one)
                    #         numberOfComponents = numberOfGaussiansPerClass( classNumber );
                    numberOfComponents = numberOfGaussiansPerClass[classNumber]
                    #         gaussianNumbers = sum( numberOfGaussiansPerClass( 1 : classNumber-1 ) ) + [ 1 : numberOfComponents ];
                    gaussianNumbers = np.array(
                        np.sum(numberOfGaussiansPerClass[:classNumber]) + np.array(range(numberOfComponents)),
                        dtype=np.uint32)
                    #
                    #         mixtureWeights( gaussianNumbers ) = mixtureWeights( gaussianNumbers ) / sum( mixtureWeights( gaussianNumbers ) );
                    mixtureWeights[gaussianNumbers] = mixtureWeights[gaussianNumbers] / np.sum(
                        mixtureWeights[gaussianNumbers])
                    #       end
                    #
                #
                #       % Now update the parameters of the bias field model.
                #       %  if ( ( multiResolutionLevel == 1 ) && ( iterationNumber ~= 1 ) ) % Don't attempt bias field correction until
                #       %                                                                    % decent mixture model parameters are available
                #       if ( estimateBiasField && ( iterationNumber > 1 ) ) % Don't attempt bias field correction until
                #                                                           % decent mixture model parameters are available
                if (estimateBiasField and (iterationNumber > 0)):
                    #         %
                    #         % Bias field correction: implements Eq. 8 in the paper
                    #         %
                    #         %    Van Leemput, "Automated Model-based Bias Field Correction of MR Images of the Brain", IEEE TMI 1999
                    #         %
                    #         precisions = zeros( size( variances ) );
                    precisions = np.zeros_like(variances)
                    #         for classNumber = 1 : numberOfGaussians
                    for classNumber in range(numberOfGaussians):
                        #           precisions( classNumber, :, : ) = reshape( inv( squeeze( variances( classNumber, :, : ) ) ), ...
                        #                                                      [ 1 numberOfContrasts numberOfContrasts ] );
                        precisions[classNumber, :, :] = np.linalg.inv(variances[classNumber, :, :]).reshape(
                            (1, numberOfContrasts, numberOfContrasts))
                        #         end
                    #
                    #         lhs = zeros( prod( numberOfBasisFunctions ) * numberOfContrasts ); % left-hand side of linear system
                    lhs = np.zeros((np.prod(numberOfBasisFunctions) * numberOfContrasts, np.prod(
                        numberOfBasisFunctions) * numberOfContrasts))  # left-hand side of linear system
                    #         rhs = zeros( prod( numberOfBasisFunctions ) * numberOfContrasts, 1 ); % right-hand side of linear system                #
                    rhs = np.zeros(
                        (np.prod(numberOfBasisFunctions) * numberOfContrasts, 1))  # right-hand side of linear system
                    #         weightsImageBuffer = zeros( downSampledImageSize );
                    weightsImageBuffer = np.zeros(downSampledImageSize)
                    #         tmpImageBuffer = zeros( downSampledImageSize );
                    tmpImageBuffer = np.zeros(downSampledImageSize)
                    #         for contrastNumber1 = 1 : numberOfContrasts
                    numberOfBasisFunctions_prod = np.prod(numberOfBasisFunctions)
                    for contrastNumber1 in range(numberOfContrasts):
                        logger.debug('third time contrastNumber=%d', contrastNumber)
                        #           tmp = zeros( size( data, 1 ), 1 );
                        tmp = np.zeros((data.shape[0], 1))
                        #           for contrastNumber2 = 1 : numberOfContrasts
                        for contrastNumber2 in range(numberOfContrasts):
                            #             classSpecificWeights = posteriors .* repmat( squeeze( precisions( :, contrastNumber1, contrastNumber2 ) )', ...
                            #                                                          [ size( posteriors, 1 ) 1 ] );
                            classSpecificWeights = posteriors * precisions[:, contrastNumber1, contrastNumber2].T
                            #             weights = sum( classSpecificWeights, 2 );
                            weights = np.sum(classSpecificWeights, 1);
                            #
                            #             % Build up stuff needed for rhs
                            #             predicted = sum( classSpecificWeights .* repmat( means( :, contrastNumber2 )', [ size( posteriors, 1 ) 1 ] ), 2 ) ...
                            #                         ./ ( weights + eps );
                            predicted = np.sum(classSpecificWeights * np.expand_dims(means[:, contrastNumber2], 2).T / (
                                    np.expand_dims(weights, 1) + eps), 1)
                            #             residue = data( :, contrastNumber2 ) - predicted;
                            residue = data[:, contrastNumber2] - predicted
                            #             tmp = tmp + weights .* residue;
                            tmp = tmp + weights.reshape(-1, 1) * residue.reshape(-1, 1)
                            #
                            #             % Fill in submatrix of lhs
                            #             weightsImageBuffer( downSampledMaskIndices ) = weights;
                            #             lhs( ( contrastNumber1 - 1 ) * prod( numberOfBasisFunctions ) + [ 1 : prod( numberOfBasisFunctions ) ], ...
                            #                  ( contrastNumber2 - 1 ) * prod( numberOfBasisFunctions ) + [ 1 : prod( numberOfBasisFunctions ) ] ) = ...
                            #                   computePrecisionOfKroneckerProductBasisFunctions( downSampledKroneckerProductBasisFunctions, weightsImageBuffer );
                            #
                            #             % Fill in submatrix of lhs
                            weightsImageBuffer[downSampledMaskIndices] = weights
                            computedPrecisionOfKroneckerProductBasisFunctions = computePrecisionOfKroneckerProductBasisFunctions(
                                downSampledKroneckerProductBasisFunctions, weightsImageBuffer)
                            if checkpoint_manager:
                                checkpoint_manager.increment('bias_field_inner_loop')
                            lhs[
                            contrastNumber1 * numberOfBasisFunctions_prod: contrastNumber1 * numberOfBasisFunctions_prod + numberOfBasisFunctions_prod,
                            contrastNumber2 * numberOfBasisFunctions_prod:contrastNumber2 * numberOfBasisFunctions_prod + numberOfBasisFunctions_prod] = computedPrecisionOfKroneckerProductBasisFunctions

                            #           end % End loop over contrastNumber2
                            #
                        #           tmpImageBuffer( downSampledMaskIndices ) = tmp;
                        tmpImageBuffer[downSampledMaskIndices] = tmp.squeeze()
                        #           rhs( ( contrastNumber1 - 1 ) * prod( numberOfBasisFunctions ) + [ 1 : prod( numberOfBasisFunctions ) ] ) = ...
                        #
                        rhs[
                        contrastNumber1 * numberOfBasisFunctions_prod: contrastNumber1 * numberOfBasisFunctions_prod + numberOfBasisFunctions_prod] \
                            = projectKroneckerProductBasisFunctions(downSampledKroneckerProductBasisFunctions,
                                                                    tmpImageBuffer).reshape(-1, 1)

                        #         end % End loop over contrastNumber1
                    #
                    #         % lhs = lhs + diag( 0.001 * diag( lhs ) );
                    #
                    #         biasFieldCoefficients = reshape( lhs \ rhs, [ prod( numberOfBasisFunctions ) numberOfContrasts ] );
                    ## TODO: There may a striding error somewhere here
                    biasFieldCoefficients = np.linalg.solve(lhs, rhs).reshape(
                        (np.prod(numberOfBasisFunctions), numberOfContrasts))
                    #         for contrastNumber = 1 : numberOfContrasts
                    for contrastNumber in range(numberOfContrasts):
                        #           downSampledBiasField = backprojectKroneckerProductBasisFunctions( downSampledKroneckerProductBasisFunctions, biasFieldCoefficients( :, contrastNumber ) );
                        downSampledBiasField = backprojectKroneckerProductBasisFunctions(
                            downSampledKroneckerProductBasisFunctions, biasFieldCoefficients[:, contrastNumber])
                        #           tmp = downSampledImageBuffers( :, :, :, contrastNumber ) - downSampledBiasField .* downSampledMask;
                        tmp = downSampledImageBuffers[:, :, :, contrastNumber] - downSampledBiasField * downSampledMask
                        #           downSampledBiasCorrectedImageBuffers( :, :, :, contrastNumber ) = tmp;
                        downSampledBiasCorrectedImageBuffers[:, :, :, contrastNumber] = tmp
                        #           biasCorrectedData( :, contrastNumber ) = tmp( downSampledMaskIndices );
                        biasCorrectedData[:, contrastNumber] = tmp[downSampledMaskIndices]
                        #         end
                    #
                    #       end % End test if multiResolutionLevel == 1
                    #
                    if checkpoint_manager:
                        checkpoint_manager.increment_and_save(
                            {
                                'biasFieldCoefficients': biasFieldCoefficients,
                                'lhs': lhs,
                                'rhs': rhs,
                                'downSampledBiasField': downSampledBiasField,
                                'downSampledBiasCorrectedImageBuffers': downSampledBiasCorrectedImageBuffers,
                                'biasCorrectedData': biasCorrectedData,
                                'computedPrecisionOfKroneckerProductBasisFunctions': computedPrecisionOfKroneckerProductBasisFunctions,
                            }, 'estimateBiasField')
                    pass
                #
                #     end % End EM iterations
            #     historyOfEMCost = historyOfEMCost( 2 : end );
            historyOfEMCost = historyOfEMCost[1:]
            #     timeTakenIntensityParameterUpdating = toc( startTimeIntensityParameterUpdating );
            #     historyOfTimeTakenIntensityParameterUpdating = [ historyOfTimeTakenIntensityParameterUpdating; ...
            #                                                      timeTakenIntensityParameterUpdating ];
            #
            #
            #     %
            #     % Part II: update the position of the mesh nodes for the current mixture model and bias field parameter estimates
            #     %
            #
            #     %
            #     startTimeDeformationUpdating = tic;
            #
            #     % Create ITK images to pass on to the mesh node position cost calculator
            #     if ( exist( 'downSampledBiasCorrectedImages' ) == 1 )
            #       % Clean up mess from any previous iteration
            #       for contrastNumber = 1 : numberOfContrasts
            #         kvlClear( downSampledBiasCorrectedImages( contrastNumber ) );
            #       end
            #     end
            #     for contrastNumber = 1 : numberOfContrasts
            downSampledBiasCorrectedImages = []
            for contrastNumber in range(numberOfContrasts):
                #       downSampledBiasCorrectedImages( contrastNumber ) = ...
                #              kvlCreateImage( single( downSampledBiasCorrectedImageBuffers( :, :, :, contrastNumber ) ) );
                downSampledBiasCorrectedImages.append(GEMS2Python.KvlImage(
                    require_np_array(downSampledBiasCorrectedImageBuffers[:, :, :, contrastNumber])))

            #     end
            #
            #     % Set up cost calculator
            #     calculator = kvlGetCostAndGradientCalculator( 'AtlasMeshToIntensityImage', ...
            #                                                    downSampledBiasCorrectedImages, ...
            #                                                    'Sliding', ...
            #                                                    transform, ...
            #                                                    means, variances, mixtureWeights, numberOfGaussiansPerClass );
            calculator = GEMS2Python.KvlCostAndGradientCalculator(
                typeName='AtlasMeshToIntensityImage',
                images=downSampledBiasCorrectedImages,
                boundaryCondition='Sliding',
                transform=transform,
                means=means,
                variances=variances,
                mixtureWeights=mixtureWeights,
                numberOfGaussiansPerClass=numberOfGaussiansPerClass)

            #     %optimizerType = 'ConjugateGradient';
            #     optimizerType = 'L-BFGS';
            #     optimizer = kvlGetOptimizer( optimizerType, mesh, calculator, ...
            #                                     'Verbose', optimizationOptions.verbose, ...
            #                                     'MaximalDeformationStopCriterion', optimizationOptions.maximalDeformationStopCriterion, ...
            #                                     'LineSearchMaximalDeformationIntervalStopCriterion', ...
            #                                       optimizationOptions.lineSearchMaximalDeformationIntervalStopCriterion, ...
            #                                     'MaximumNumberOfIterations', optimizationOptions.maximumNumberOfDeformationIterations, ...
            #                                     'BFGS-MaximumMemoryLength', optimizationOptions.BFGSMaximumMemoryLength );
            optimizerType = 'L-BFGS';
            optimization_parameters = {
                'Verbose': optimizationOptions.verbose,
                'MaximalDeformationStopCriterion': optimizationOptions.maximalDeformationStopCriterion,
                'LineSearchMaximalDeformationIntervalStopCriterion': optimizationOptions.lineSearchMaximalDeformationIntervalStopCriterion,
                'MaximumNumberOfIterations': optimizationOptions.maximumNumberOfDeformationIterations,
                'BFGS-MaximumMemoryLength': optimizationOptions.BFGSMaximumMemoryLength
            }
            optimizer = GEMS2Python.KvlOptimizer(optimizerType, mesh, calculator, optimization_parameters)
            #
            #     historyOfDeformationCost = [];
            historyOfDeformationCost = [];
            #     historyOfMaximalDeformation = [];
            historyOfMaximalDeformation = [];
            #     nodePositionsBeforeDeformation = kvlGetMeshNodePositions( mesh );
            nodePositionsBeforeDeformation = mesh.points
            #     deformationStartTime = tic;
            #     while true
            while True:
                #       %
                #       stepStartTime = tic;
                #       [ minLogLikelihoodTimesDeformationPrior, maximalDeformation ] = kvlStepOptimizer( optimizer );
                minLogLikelihoodTimesDeformationPrior, maximalDeformation = optimizer.step_optimizer()
                #       disp( [ 'maximalDeformation ' num2str( maximalDeformation ) ' took ' num2str( toc( stepStartTime ) ) ' sec' ] )
                print("maximalDeformation={} minLogLikelihood={}".format(maximalDeformation, minLogLikelihoodTimesDeformationPrior))
                #
                #       if ( maximalDeformation == 0 )
                #         break;
                #       end
                if maximalDeformation == 0:
                    break
                #
                #       %
                #       historyOfDeformationCost = [ historyOfDeformationCost; minLogLikelihoodTimesDeformationPrior ];
                historyOfDeformationCost.append(minLogLikelihoodTimesDeformationPrior)
                #       historyOfMaximalDeformation = [ historyOfMaximalDeformation; maximalDeformation ];
                historyOfMaximalDeformation.append(maximalDeformation)
                #
                #     end % End loop over iterations
            #     kvlClear( calculator );
            #     kvlClear( optimizer );
            #     % haveMoved = ( length( historyOfDeformationCost ) > 0 );
            #     nodePositionsAfterDeformation = kvlGetMeshNodePositions( mesh );
            nodePositionsAfterDeformation = mesh.points
            #     maximalDeformationApplied = sqrt( max( sum( ...
            #                 ( nodePositionsAfterDeformation - nodePositionsBeforeDeformation ).^2, 2 ) ) );
            maximalDeformationApplied = np.sqrt(
                np.max(np.sum((nodePositionsAfterDeformation - nodePositionsBeforeDeformation) ** 2, 1)))
            #     disp( '==============================' )
            #     disp( [ 'iterationNumber: ' num2str( iterationNumber ) ] )
            #     disp( [ '    maximalDeformationApplied: ' num2str( maximalDeformationApplied ) ] )
            #     disp( [ '  ' num2str( toc( deformationStartTime ) ) ' sec' ] )
            #     disp( '==============================' )
            print('==============================')
            print(['iterationNumber: ', iterationNumber])
            print(['    maximalDeformationApplied: ', maximalDeformationApplied])
            # print( [ '  ' num2str( toc( deformationStartTime ) ) ' sec' ] )
            print('==============================')
            if checkpoint_manager:
                checkpoint_manager.increment_and_save(
                    {
                        'maximalDeformationApplied': maximalDeformationApplied,
                        'nodePositionsAfterDeformation': nodePositionsAfterDeformation,
                    }, 'optimizer')
            #
            #
            #     % Show a little movie comparing before and after deformation so far...
            #     if ( showFigures )
            #       figure( deformationMovieFigure )
            #       newColorCodedPriors = kvlColorCodeProbabilityImages( kvlRasterizeAtlasMesh( mesh, downSampledImageSize ), reducedColors );
            #
            #       set( deformationMovieFigure, 'position', get( 0, 'ScreenSize' ) );
            #       for i = 1 : 10
            #         priorVisualizationAlpha = 0.4;
            #         backgroundImage = exp( downSampledImageBuffers( :, :, :, 1 ) );
            #         backgroundImage = backgroundImage - min( backgroundImage(:) );
            #         backgroundImage = backgroundImage / max( backgroundImage(:) );
            #
            #         % showImage( oldColorCodedPriors )
            #         imageToShow = ( 1 - priorVisualizationAlpha ) * repmat( backgroundImage, [ 1 1 1 3 ] ) + ...
            #                       priorVisualizationAlpha * oldColorCodedPriors;
            #         showImage( imageToShow )
            #         drawnow
            #         pause( 0.1 )
            #         % showImage( newColorCodedPriors )
            #         imageToShow = ( 1 - priorVisualizationAlpha ) * repmat( backgroundImage, [ 1 1 1 3 ] ) + ...
            #                       priorVisualizationAlpha * newColorCodedPriors;
            #         showImage( imageToShow )
            #         drawnow
            #         pause( 0.1 )
            #       end
            #     end
            #
            #     % Keep track of the cost function we're optimizing
            #     historyOfCost = [ historyOfCost; minLogLikelihoodTimesDeformationPrior + intensityModelParameterCost ];
            historyOfCost.append(minLogLikelihoodTimesDeformationPrior + intensityModelParameterCost)
            #     historyOfMaximalDeformationApplied = [ historyOfMaximalDeformationApplied; maximalDeformationApplied ];
            #     timeTakenDeformationUpdating = toc( startTimeDeformationUpdating );
            #     historyOfTimeTakenDeformationUpdating = [ historyOfTimeTakenDeformationUpdating; ...
            #                                               timeTakenDeformationUpdating ];
            #
            #
            #     % Save something about how the estimation proceeded
            #     %historyWithinEachIteration( iterationNumber ).priors = priors;
            #     %historyWithinEachIteration( iterationNumber ).posteriors = posteriors;
            #     historyWithinEachIteration( iterationNumber ).historyOfEMCost = historyOfEMCost;
            #     historyWithinEachIteration( iterationNumber ).mixtureWeights = mixtureWeights;
            #     historyWithinEachIteration( iterationNumber ).means = means;
            #     historyWithinEachIteration( iterationNumber ).variances = variances;
            #     historyWithinEachIteration( iterationNumber ).biasFieldCoefficients = biasFieldCoefficients;
            #     historyWithinEachIteration( iterationNumber ).historyOfDeformationCost = historyOfDeformationCost;
            #     historyWithinEachIteration( iterationNumber ).historyOfMaximalDeformation = historyOfMaximalDeformation;
            #     historyWithinEachIteration( iterationNumber ).maximalDeformationApplied = maximalDeformationApplied;
            #
            #     % Determine if we should stop the overall iterations over the two set of parameters
            #     %  if ( ( ~haveMoved ) || ...
            #     %        ( ( ( historyOfCost( end-1 ) - historyOfCost( end ) ) / historyOfCost( end ) ) ...
            #     %          < relativeCostDecreaseStopCriterion ) || ...
            #     %        ( maximalDeformationApplied < maximalDeformationAppliedStopCriterion ) )
            #     if ( ( ( ( historyOfCost( end-1 ) - historyOfCost( end ) ) / length( downSampledMaskIndices ) ) ...
            priorCost = historyOfCost[-2]
            currentCost = historyOfCost[-1]
            costChange = priorCost - currentCost
            activeVoxelCount = len(downSampledMaskIndices[0])
            perVoxelDecrease = costChange / activeVoxelCount
            perVoxelDecreaseThreshold = optimizationOptions.absoluteCostPerVoxelDecreaseStopCriterion
            # if ((((historyOfCost[-2] - historyOfCost[-1]) / len(downSampledMaskIndices[0]))
            #      #            < optimizationOptions.absoluteCostPerVoxelDecreaseStopCriterion ) ) % If EM converges in one iteration and mesh node optimization doesn't do anything
            #      < optimizationOptions.absoluteCostPerVoxelDecreaseStopCriterion)):  # If EM converges in one iteration and mesh node optimization doesn't do anything
                #       % Converged
            if perVoxelDecrease < perVoxelDecreaseThreshold:
                #       break;
                if checkpoint_manager:
                    checkpoint_manager.increment_and_save({
                        'activeVoxelCount': activeVoxelCount,
                        'priorCost': priorCost,
                        'currentCost': currentCost,
                        'costChange': costChange,
                        'perVoxelDecrease': perVoxelDecrease,
                        'perVoxelDecreaseThreshold': perVoxelDecreaseThreshold,
                        'minLogLikelihoodTimesDeformationPrior': minLogLikelihoodTimesDeformationPrior,
                        'intensityModelParameterCost': intensityModelParameterCost,
                    }, 'optimizerPerVoxelExit')
                break
                #     end
            #
            #
            #   end % End looping over global iterations for this multiresolution level
        #   historyOfCost = historyOfCost( 2 : end );
        #
        #   % Get the final node positions
        #   finalNodePositions = kvlGetMeshNodePositions( mesh );
        finalNodePositions = mesh.points
        #
        #   % Transform back in template space (i.e., undoing the affine registration
        #   % that we applied), and save for later usage
        #   tmp = ( totalTransformationMatrix \ [ finalNodePositions ones( numberOfNodes, 1 ) ]' )';

        tmp = np.linalg.solve(totalTransformationMatrix,
                              np.pad(finalNodePositions, ((0, 0), (0, 1)), 'constant', constant_values=1).T).T
        #   finalNodePositionsInTemplateSpace = tmp( :, 1 : 3 );
        finalNodePositionsInTemplateSpace = tmp[:, 0: 3]
        #
        #
        #   % Save something about how the estimation proceeded
        #   historyWithinEachMultiResolutionLevel( multiResolutionLevel ).downSamplingFactors = downSamplingFactors;
        #   historyWithinEachMultiResolutionLevel( multiResolutionLevel ).downSampledImageBuffers = downSampledImageBuffers;
        #   historyWithinEachMultiResolutionLevel( multiResolutionLevel ).downSampledMask = downSampledMask;
        #   historyWithinEachMultiResolutionLevel( multiResolutionLevel ).initialNodePositions = initialNodePositions;
        #   historyWithinEachMultiResolutionLevel( multiResolutionLevel ).finalNodePositions = finalNodePositions;
        #   historyWithinEachMultiResolutionLevel( multiResolutionLevel ).initialNodePositionsInTemplateSpace = ...
        #                                                                              initialNodePositionsInTemplateSpace;
        #   historyWithinEachMultiResolutionLevel( multiResolutionLevel ).finalNodePositionsInTemplateSpace = ...
        #                                                                              finalNodePositionsInTemplateSpace;
        #   historyWithinEachMultiResolutionLevel( multiResolutionLevel ).historyWithinEachIteration = ...
        #                                                                       historyWithinEachIteration;
        #   historyWithinEachMultiResolutionLevel( multiResolutionLevel ).historyOfCost = historyOfCost;
        #   historyWithinEachMultiResolutionLevel( multiResolutionLevel ).historyOfMaximalDeformationApplied = ...
        #                                                                       historyOfMaximalDeformationApplied;
        #   historyWithinEachMultiResolutionLevel( multiResolutionLevel ).historyOfTimeTakenIntensityParameterUpdating = ...
        #                                                                       historyOfTimeTakenIntensityParameterUpdating;
        #   historyWithinEachMultiResolutionLevel( multiResolutionLevel ).historyOfTimeTakenDeformationUpdating = ...
        #                                                                       historyOfTimeTakenDeformationUpdating;
        #   historyWithinEachMultiResolutionLevel( multiResolutionLevel ).priorsAtEnd = priors;
        #   historyWithinEachMultiResolutionLevel( multiResolutionLevel ).posteriorsAtEnd = posteriors;
        ## Record deformation delta here in lieu of maintaining history
        nodeDeformationInTemplateSpaceAtPreviousMultiResolutionLevel = \
            finalNodePositionsInTemplateSpace - initialNodePositionsInTemplateSpace
        #
        # end % End loop over multiresolution levels
        #
    return {
        'biasFieldCoefficients': biasFieldCoefficients,
        'imageBuffers': imageBuffers,
        'means': means,
        'mixtureWeights': mixtureWeights,
        'nodeDeformationInTemplateSpaceAtPreviousMultiResolutionLevel':
            nodeDeformationInTemplateSpaceAtPreviousMultiResolutionLevel,
        'transformMatrix': transform.as_numpy_array,
        'variances': variances,
    }

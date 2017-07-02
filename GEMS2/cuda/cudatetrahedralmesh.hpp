#pragma once

#include "kvlAtlasMesh.h"

#include "cudaimage.hpp"

namespace kvl {
  namespace cuda {
    template<typename CoordinateType, typename MeshIndexType>
    class CudaTetrahedralMesh {
    public:
      const unsigned char nDims = 3;
      const unsigned char nVertices = 4;

      void Send( kvl::AtlasMesh::ConstPointer mesh ) {
	auto tetIds = this->GetTetrahedronIds( mesh );

	// Sanity check things
	std::set<size_t> activePointIndices;
	for( size_t iTet=0; iTet<tetIds.size(); iTet++ ) {
	  kvl::AtlasMesh::CellAutoPointer cell;
	  mesh->GetCell( tetIds.at(iTet), cell );
	  
	  size_t pointCount = 0;
	  for( auto pit = cell->PointIdsBegin();
	       pit != cell->PointIdsEnd();
	       ++pit ) {
	    activePointIndices.insert(*pit);
	    pointCount++;
	  }
    
	  if( pointCount != nVertices ) {
	    throw std::runtime_error("Found tetrahedron with wrong vertex count");
	  }
	}

	if( activePointIndices.size() != mesh->GetPoints()->size() ) {
	  throw std::runtime_error("Point index remapping not supported!");
	}


	this->SendVertices(mesh);
	this->SendVertexMap(mesh, tetIds);
      }

    private:
      CudaImage<CoordinateType,2,MeshIndexType> d_vertices;
      CudaImage<MeshIndexType,2,MeshIndexType> d_vertexMap;

      std::vector<kvl::AtlasMesh::CellIdentifier> GetTetrahedronIds( kvl::AtlasMesh::ConstPointer mesh ) const {
	std::vector<kvl::AtlasMesh::CellIdentifier> ids;
	for( auto cellIt = mesh->GetCells()->Begin();
	     cellIt != mesh->GetCells()->End();
	     ++cellIt ) {
	  if( cellIt.Value()->GetType() == kvl::AtlasMesh::CellType::TETRAHEDRON_CELL ) {
	    ids.push_back( cellIt.Index() );
	  }
	}

	return ids;
      }

      void SendVertices( kvl::AtlasMesh::ConstPointer mesh ) {
	Dimension<2,MeshIndexType> vertexDims;
	vertexDims[0] = mesh->GetPoints()->size();
	vertexDims[1] = nDims;

	std::vector<CoordinateType> vertices;
	vertices.resize(vertexDims.ElementCount());

	for( auto pointIt = mesh->GetPoints()->Begin();
	     pointIt != mesh->GetPoints()->End();
	     ++pointIt ) {
	  for( MeshIndexType i=0; i<nDims; i++ ) {
	    size_t idx = vertexDims.GetLinearIndex(pointIt.Index(),i);
	    vertices.at(idx) = pointIt.Value()[i];
	  }
	}

	this->d_vertices.Send( vertices, vertexDims );
      }

      void SendVertexMap( kvl::AtlasMesh::ConstPointer mesh, const std::vector<kvl::AtlasMesh::CellIdentifier>& ids ) {
	Dimension<2,MeshIndexType> mapDims;
	mapDims[0] = ids.size();
	mapDims[1] = nVertices;

	std::vector<MeshIndexType> vertexMap;
	vertexMap.resize(mapDims.ElementCount());

	for( MeshIndexType iTet=0; iTet<ids.size(); iTet++ ) {
	  AtlasMesh::CellAutoPointer cell;
	  mesh->GetCell( ids.at(iTet), cell );

	  MeshIndexType pIdx = 0;
	  for( auto pit = cell->PointIdsBegin();
	       pit != cell->PointIdsEnd();
	       ++pit ) {
	     size_t idx = mapDims.GetLinearIndex(iTet,pIdx);
	     vertexMap.at(idx) = *pit;
	     pIdx++;
	  }
	}

	this->d_vertexMap.Send( vertexMap, mapDims );
      }
    };
  }
}

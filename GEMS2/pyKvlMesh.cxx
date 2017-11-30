#include <pybind11/pybind11.h>
#include "itkMacro.h"
#include "pyKvlMesh.h"
#include "pyKvlNumpy.h"

KvlMesh::KvlMesh() {

}

KvlMesh::KvlMesh(MeshPointer &aMesh) {
    mesh = aMesh;
}

int KvlMesh::PointCount() const {
    return mesh->GetPoints()->Size();
}

py::array_t<double> KvlMesh::GetPointSet() const {
    return PointSetToNumpy(mesh->GetPoints());
}

KvlMeshCollection::KvlMeshCollection() {
    meshCollection = kvl::AtlasMeshCollection::New();
}

void KvlMeshCollection::Read(const std::string &meshCollectionFileName) {
    if (!meshCollection->Read(meshCollectionFileName.c_str())) {
        itkExceptionMacro("Couldn't read mesh collection from file " << meshCollectionFileName);
    }
    std::cout << "Read mesh collection: " << meshCollectionFileName << std::endl;
}

void KvlMeshCollection::Write(const std::string &meshCollectionFileName) {
    if (!meshCollection->Write(meshCollectionFileName.c_str())) {
        itkExceptionMacro("Couldn't write mesh collection to file " << meshCollectionFileName);
    }
    std::cout << "Wrote mesh collection: " << meshCollectionFileName << std::endl;
}

void KvlMeshCollection::SetK(double k) {
    meshCollection->SetK(k);
}

double KvlMeshCollection::GetK() const {
    return meshCollection->GetK();
}


KvlMesh *KvlMeshCollection::GetMesh(int meshNumber) {
    if (meshNumber < -1) {
        itkExceptionMacro("meshNumber " << meshNumber << " too negative");
    } else if (meshNumber == -1) {
        return this->GetReferenceMesh();
    } else {
        unsigned int meshCount = this->MeshCount();
        unsigned int meshIndex = (unsigned int) meshNumber;
        if (meshIndex >= meshCount) {
            itkExceptionMacro("meshNumber " << meshNumber << " exceeds mesh count of " << meshCount);
        } else {
            MeshPointer mesh = meshCollection->GetMesh(meshIndex);
            return new KvlMesh(mesh);
        }
    }
}

KvlMesh *KvlMeshCollection::GetReferenceMesh() {
    MeshPointer mesh = meshCollection->GetReferenceMesh();
    return new KvlMesh(mesh);
}

unsigned int KvlMeshCollection::MeshCount() const {
    return meshCollection->GetNumberOfMeshes();
}

#define XYZ_DIMENSIONS 3
py::array_t<double> PointSetToNumpy(PointSetConstPointer points) {
    const unsigned int numberOfNodes = points->Size();
    auto *data = new double[numberOfNodes * XYZ_DIMENSIONS];
    auto dataIterator = data;
    for (auto pointsIterator = points->Begin(); pointsIterator != points->End(); ++pointsIterator) {
        for (int xyzAxisSelector = 0; xyzAxisSelector < XYZ_DIMENSIONS; xyzAxisSelector++) {
            *dataIterator++ = pointsIterator.Value()[xyzAxisSelector];
        }
    }
    return createNumpyArrayCStyle({numberOfNodes, XYZ_DIMENSIONS}, data);
}
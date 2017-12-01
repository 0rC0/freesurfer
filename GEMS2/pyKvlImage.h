#ifndef GEMS_KVLIMAGE_H
#define GEMS_KVLIMAGE_H

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include "itkImage.h"
#include "pyKvlTransform.h"

namespace py = pybind11;

typedef itk::Image< float, 3 >  ImageType;
typedef ImageType::Pointer ImagePointer;

class KvlImage {
public:
    ImagePointer imageHandle;
    TransformPointer transform;
    KvlImage(const std::string &imageFileName);
    std::unique_ptr<KvlTransform> GetTransformMatrix();
    py::array_t<float> GetImageBuffer();
};



#endif //GEMS_KVLIMAGE_H

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
    ImagePointer imageHandle;
    TransformPointer transform;

public:
    KvlImage(const std::string &imageFileName);
    void Greet();
    py::array_t<double> GetTransformMatrix();
};

void UseImage(KvlImage* image);



#endif //GEMS_KVLIMAGE_H

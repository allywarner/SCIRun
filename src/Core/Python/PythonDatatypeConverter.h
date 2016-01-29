/*
 For more information, please see: http://software.sci.utah.edu

 The MIT License

 Copyright (c) 2015 Scientific Computing and Imaging Institute,
 University of Utah.


 Permission is hereby granted, free of charge, to any person obtaining a
 copy of this software and associated documentation files (the "Software"),
 to deal in the Software without restriction, including without limitation
 the rights to use, copy, modify, merge, publish, distribute, sublicense,
 and/or sell copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included
 in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 DEALINGS IN THE SOFTWARE.
 */

#ifdef BUILD_WITH_PYTHON
#ifndef CORE_PYTHON_PYTHONDATATYPECONVERTER_H
#define CORE_PYTHON_PYTHONDATATYPECONVERTER_H

#include <boost/python.hpp>
#include <boost/python/stl_iterator.hpp>
#include <boost/shared_ptr.hpp>
#include <vector>
#include <Core/Datatypes/DatatypeFwd.h>


#include <Core/Python/share.h>

namespace SCIRun
{
  namespace Core
  {
    namespace Python
    {
      template <class T>
      boost::python::list toPythonList(const std::vector<T>& vec)
      {
        boost::python::list list;
        for (const auto& v : vec)
        {
          list.append(v);
        }
        return list;
      }

      template <class T>
      boost::python::list toPythonListOfLists(const std::vector<T>& vec, int dim1, int dim2)
      {
        boost::python::list list;
        auto iter = vec.begin();
        for (int i = 0; i < dim1; ++i)
        {
          boost::python::list row;
          for (int j = 0; j < dim2; ++j)
            row.append(*iter++);
          list.append(row);
        }
        return list;
      }

      template< typename T >
      std::vector< T > to_std_vector(const boost::python::object& iterable)
      {
        return std::vector< T >(boost::python::stl_input_iterator< T >(iterable),
          boost::python::stl_input_iterator< T >());
      }


      SCISHARE boost::python::object convertFieldToPython(FieldHandle field);
      SCISHARE boost::python::object convertMatrixToPython(Datatypes::DenseMatrixHandle matrix);
      SCISHARE boost::python::object convertMatrixToPython(Datatypes::SparseRowMatrixHandle matrix);
      SCISHARE boost::python::object convertStringToPython(Datatypes::StringHandle str);
    }
  }
}

#endif
#endif
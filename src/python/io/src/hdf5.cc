/**
 * @author <a href="mailto:andre.dos.anjos@gmail.com">Andre Anjos</a> 
 * @date Thu 14 Apr 09:41:47 2011 
 *
 * @brief Binds our C++ HDF5 interface to python 
 */

#include <boost/python.hpp>
#include <boost/make_shared.hpp>
#include <boost/format.hpp>

#include "core/python/exception.h"
#include "core/python/pycore.h"

#include "io/HDF5File.h"

using namespace boost::python;
namespace io = Torch::io;
namespace tp = Torch::python;

/**
 * Allows us to write HDF5File("filename.hdf5", "rb")
 */
static boost::shared_ptr<io::HDF5File>
hdf5file_make_fromstr(const std::string& filename, const std::string& opmode) {
  static const char* help = "Supported flags are 'r' (read-only), 'w' (read/append), 't' (read/write/truncate) or 'x' (exclusive)";
  if (opmode.size() > 1) {
    PyErr_SetString(PyExc_RuntimeError, help);
    throw_error_already_set();
  }
  io::HDF5File::mode_t mode = io::HDF5File::inout;
  if (opmode[0] == 'r') mode = io::HDF5File::in;
  else if (opmode[0] == 'w') mode = io::HDF5File::inout;
  else if (opmode[0] == 't') mode = io::HDF5File::trunc;
  else if (opmode[0] == 'x') mode = io::HDF5File::excl;
  else { //anything else is just unsupported for the time being
    PyErr_SetString(PyExc_RuntimeError, help);
    throw_error_already_set();
  }
  return boost::make_shared<io::HDF5File>(filename, mode);
}

/**
 * Allows us to write HDF5File("filename.hdf5") and open that file for input
 * and output (without truncation)
 */
static boost::shared_ptr<io::HDF5File>
hdf5file_make_readwrite(const std::string& filename) {
  return boost::make_shared<io::HDF5File>(filename, io::HDF5File::inout);
}

/**
 * Returns a list of all paths inside a HDF5File
 */
static list hdf5file_paths(const io::HDF5File& f) {
  list retval;
  std::vector<std::string> values;
  f.paths(values);
  for (size_t i=0; i<values.size(); ++i) retval.append(str(values[i]));
  return retval;
}

/**
 * Returns tuples for the description of all possible ways to read a certain
 * path.
 */
static tuple hdf5file_describe(const io::HDF5File& f, const std::string& p) {
  const std::vector<io::HDF5Descriptor>& dv = f.describe(p);
  list retval;
  for (size_t k=0; k<dv.size(); ++k) retval.append(dv[k]);
  return tuple(retval);
}

/**
 * Functionality to read from HDF5File's
 */
static object hdf5file_xread(io::HDF5File& f, const std::string& p,
    int descriptor, int pos) {

  const std::vector<io::HDF5Descriptor>& D = f.describe(p);

  //last descriptor always contains the full readout.
  const io::HDF5Type& type = D[descriptor].type;
  const io::HDF5Shape& shape = type.shape();
  
  if (shape.n() == 1 && shape[0] == 1) { //read as scalar
    switch(type.type()) {
      case io::i8: 
        return object(f.read<int8_t>(p, pos));
      case io::i16: 
        return object(f.read<int16_t>(p, pos));
      case io::i32: 
        return object(f.read<int32_t>(p, pos));
      case io::i64: 
        return object(f.read<int64_t>(p, pos));
      case io::u8:
        return object(f.read<uint8_t>(p, pos));
      case io::u16: 
        return object(f.read<uint16_t>(p, pos));
      case io::u32: 
        return object(f.read<uint32_t>(p, pos));
      case io::u64: 
        return object(f.read<uint64_t>(p, pos));
      case io::f32: 
        return object(f.read<float>(p, pos));
      case io::f64: 
        return object(f.read<double>(p, pos));
      case io::f128:
        return object(f.read<long double>(p, pos));
      case io::c64: 
        return object(f.read<std::complex<float> >(p, pos));
      case io::c128:
        return object(f.read<std::complex<double> >(p, pos));
      case io::c256:
        return object(f.read<std::complex<long double> >(p, pos));
      default:
        boost::format s("unsupported HDF5 type: %s");
        s % type.str();
        PYTHON_ERROR(TypeError, s.str().c_str());
    }
  }

  //read as an numpy array
  npy_intp dims[NPY_MAXDIMS];
  for (uint64_t k=0; k<shape.n(); ++k) dims[k] = shape[k];
  PyArrayObject* arr = tp::make_ndarray(shape.n(), dims, tp::eltype_to_num(type.element_type()));
  f.read_buffer(p, pos, type, arr->data);
  handle<> retval(borrowed<>((PyObject*)arr)); //?
  return object(retval);
}

static object hdf5file_lread(io::HDF5File& f, const std::string& p,
    int64_t pos=-1) {
  if (pos >= 0) return hdf5file_xread(f, p, 0, pos);

  //otherwise returns as a list
  const std::vector<io::HDF5Descriptor>& D = f.describe(p);
  list retval;
  for (uint64_t k=0; k<D[0].size; ++k) 
    retval.append(hdf5file_xread(f, p, 0, k));
  return retval;
}
BOOST_PYTHON_FUNCTION_OVERLOADS(hdf5file_lread_overloads, hdf5file_lread, 2, 3)

static inline object hdf5file_read(io::HDF5File& f, const std::string& p) {
  return hdf5file_xread(f, p, 1, 0);
}

template <typename T> static void hdf5file_replace_scalar(io::HDF5File& f, const std::string& p, size_t pos, const T& value) {
  f.replace(p, pos, value);
}

static void hdf5file_replace_array(io::HDF5File& f, const std::string& p, 
    size_t pos, numeric::array& arrobj) {

  PyArrayObject* arr = (PyArrayObject*)arrobj.ptr();
    
  tp::assert_ndarray_byteorder(arr);
  tp::assert_ndarray_behaved(arr);

  io::HDF5Type type(tp::num_to_eltype(arr->descr->type_num), 
      io::HDF5Shape(arr->nd, arr->dimensions));

  f.write_buffer(p, pos, type, arr->data);
}

static void hdf5file_append_array(io::HDF5File& f, 
    const std::string& path, numeric::array& arrobj, size_t compression) {

  PyArrayObject* arr = (PyArrayObject*)arrobj.ptr();

  tp::assert_ndarray_byteorder(arr);
  tp::assert_ndarray_behaved(arr);

  io::HDF5Type type(tp::num_to_eltype(arr->descr->type_num), 
      io::HDF5Shape(arr->nd, arr->dimensions));

  if (!f.contains(path)) f.create(path, type, true, compression);

  f.extend_buffer(path, type, arr->data);
}

static void hdf5file_set_array(io::HDF5File& f, 
    const std::string& path, numeric::array& arrobj, size_t compression) {
  
  PyArrayObject* arr = (PyArrayObject*)arrobj.ptr();

  tp::assert_ndarray_byteorder(arr);
  tp::assert_ndarray_behaved(arr);

  io::HDF5Type type(tp::num_to_eltype(arr->descr->type_num), 
      io::HDF5Shape(arr->nd, arr->dimensions));

  if (!f.contains(path)) f.create(path, type, false, compression);

  f.write_buffer(path, 0, type, arr->data);
}

void bind_io_hdf5() {
  class_<io::HDF5File, boost::shared_ptr<io::HDF5File>, boost::noncopyable>("HDF5File", "A HDF5File allows users to read and write data from and to files containing standard Torch binary coded data in HDF5 format. For an introduction to HDF5, please visit http://www.hdfgroup.org/HDF5.", no_init)
    .def("__init__", make_constructor(hdf5file_make_fromstr, default_call_policies(), (arg("filename"), arg("openmode_string"))), "Opens a new file in one of these supported modes: 'r' (read-only), 'w' (read/write/append), 't' (read/write/truncate) or 'x' (read/write/exclusive)")
    .def("__init__", make_constructor(hdf5file_make_readwrite, default_call_policies(), (arg("filename"))), "Opens a new HDF5File for reading and writing.")
    .def("cd", &io::HDF5File::cd, (arg("self"), arg("path")), "Changes the current prefix path. When this object is started, the prefix path is empty, which means all following paths to data objects should be given using the full path. If you set this to a different value, it will be used as a prefix to any subsequent operation until you reset it. If path starts with '/', it is treated as an absolute path. '..' and '.' are supported. This object should be a std::string. If the value is relative, it is added to the current path. If it is absolute, it causes the prefix to be reset. Note all operations taking a relative path, following a cd(), will be considered relative to the value defined by the 'cwd' property of this object.")
    .add_property("cwd", make_function(&io::HDF5File::cwd, return_value_policy<copy_const_reference>()), &io::HDF5File::cd)
    .def("__contains__", &io::HDF5File::contains, (arg("self"), arg("key")), "Returns True if the file contains an HDF5 dataset with a given path")
    .def("has_key", &io::HDF5File::contains, (arg("self"), arg("key")), "Returns True if the file contains an HDF5 dataset with a given path")
    .def("describe", &hdf5file_describe, (arg("self"), arg("key")), "If a given path to an HDF5 dataset exists inside the file, return a type description of objects recorded in such a dataset, otherwise, raises an exception. The returned value type is a tuple of tuples (HDF5Type, number-of-objects, expandible) describing the capabilities if the file is read using theses formats.")
    .def("unlink", &io::HDF5File::unlink, (arg("self"), arg("key")), "If a given path to an HDF5 dataset exists inside the file, unlinks it. Please note this will note remove the data from the file, just make it inaccessible. If you wish to cleanup, save the reacheable objects from this file to another HDF5File object using copy(), for example.")
    .def("rename", &io::HDF5File::rename, (arg("self"), arg("from"), arg("to")), "If a given path to an HDF5 dataset exists in the file, rename it")
    .def("keys", &hdf5file_paths, (arg("self")), "Returns all paths to datasets available inside this file")
    .def("paths", &hdf5file_paths, (arg("self")), "Returns all paths to datasets available inside this file")
    .def("copy", &io::HDF5File::copy, (arg("self"), arg("file")), "Copies all accessible content to another HDF5 file")
    .def("read", &hdf5file_read, (arg("self"), arg("key")), "Reads the whole dataset in a single shot. Returns a single object with all contents.") 
    .def("lread", (object(*)(io::HDF5File&, const std::string&, int64_t))0, hdf5file_lread_overloads((arg("self"), arg("key"), arg("pos")=-1), "Reads a given position from the dataset. Returns a single object if 'pos' >= 0, otherwise a list by reading all objects in sequence."))
#   define DECLARE_SUPPORT(T,E) \
    .def(BOOST_PP_STRINGIZE(__replace_ ## E ## __), &hdf5file_replace_scalar<T>, (arg("self"), arg("key"), arg("pos"), arg("value")), "Modifies the value of a scalar inside the file.") \
    .def(BOOST_PP_STRINGIZE(__append_ ## E ## __), &io::HDF5File::append<T>, (arg("self"), arg("key"), arg("value")), "Appends a scalar to a dataset. If the dataset does not yet exist, one is created with the type characteristics.") \
    .def(BOOST_PP_STRINGIZE(__set_ ## E ## __), &io::HDF5File::set<T>, (arg("self"), arg("key"), arg("value")), "Sets the scalar at position 0 to the given value. This method is equivalent to checking if the scalar at position 0 exists and then replacing it. If the path does not exist, we append the new scalar.") 
    DECLARE_SUPPORT(bool, bool)
    DECLARE_SUPPORT(int8_t, int8)
    DECLARE_SUPPORT(int16_t, int16)
    DECLARE_SUPPORT(int32_t, int32)
    DECLARE_SUPPORT(int64_t, int64)
    DECLARE_SUPPORT(uint8_t, uint8)
    DECLARE_SUPPORT(uint16_t, uint16)
    DECLARE_SUPPORT(uint32_t, uint32)
    DECLARE_SUPPORT(uint64_t, uint64)
    DECLARE_SUPPORT(float, float32)
    DECLARE_SUPPORT(double, float64)
    //DECLARE_SUPPORT(long double, float128)
    DECLARE_SUPPORT(std::complex<float>, complex64)
    DECLARE_SUPPORT(std::complex<double>, complex128)
    //DECLARE_SUPPORT(std::complex<long double>, complex256)
    DECLARE_SUPPORT(std::string, string)
#   undef DECLARE_SUPPORT
    .def("__append_array__", &hdf5file_append_array, (arg("self"), arg("key"), arg("array"), arg("compression")), "Appends a array to a dataset. If the dataset does not yet exist, one is created with the type characteristics.\n\nIf a new Dataset is to be created you can set its compression level. Note these settings have no effect if the Dataset already exists on file, in which case the current settings for that dataset are respected. The maximum value for the gzip compression is 9. The value of zero turns compression off (the default).")
    .def("__replace_array__", &hdf5file_replace_array, (arg("self"), arg("key"), arg("pos"), arg("array")), "Modifies the value of a array inside the file.")
    .def("__set_array__", &hdf5file_set_array, (arg("self"), arg("key"), arg("array"), arg("compression")), "Sets the array at position 0 to the given value. This method is equivalent to checking if the array at position 0 exists and then replacing it. If the path does not exist, you can set the compression level. Note these settings have no effect if the Dataset already exists on file, in which case the current settings for that dataset are respected. The maximum value for the gzip compression is 9. The value of zero turns compression off (the default).") 
    ;
}

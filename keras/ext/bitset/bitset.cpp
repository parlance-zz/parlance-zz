#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <intrin.h>
#include <stdint.h>
#include <random>
#define NPY_NO_DEPRECATED_API 1
#include <numpy/arrayobject.h>

static std::mt19937_64 _rng;

inline uint64_t BitSet_Count(uint64_t *a, size_t len)
{
	uint64_t c = 0;
	for (int i = 0; i < len; i++) c += __popcnt64(a[i]);
	return c;
}

inline void BitSet_And(uint64_t *a, uint64_t *b, size_t len)
{
	for (int i = 0; i < len; i++) a[i] &= b[i];
}

inline uint64_t BitSet_And_Count(uint64_t *a, uint64_t *b, size_t len)
{
	uint64_t c = 0;
	for (int i = 0; i < len; i++)
	{
		a[i] &= b[i];
		c += __popcnt64(a[i]);
	}
	return c;
}

inline void BitSet_Or(uint64_t *a, uint64_t *b, size_t len)
{
	for (int i = 0; i < len; i++) a[i] |= b[i];
}

inline uint64_t BitSet_Or_Count(uint64_t *a, uint64_t *b, size_t len)
{
	uint64_t c = 0;
	for (int i = 0; i < len; i++)
	{
		a[i] |= b[i];
		c += __popcnt64(a[i]);
	}
	return c;
}

inline void BitSet_Xor(uint64_t *a, uint64_t *b, size_t len)
{
	for (int i = 0; i < len; i++) a[i] ^= b[i];
}

inline uint64_t BitSet_Xor_Count(uint64_t *a, uint64_t *b, size_t len)
{
	uint64_t c = 0;
	for (int i = 0; i < len; i++)
	{
		a[i] ^= b[i];
		c += __popcnt64(a[i]);
	}
	return c;
}

inline void BitSet_SelectMerge(uint64_t *merged, uint64_t *lib, uint64_t *index, size_t indexLen, size_t mergedLen)
{
	for (int i = 0; i < indexLen; i++)
	{
		uint64_t tmp = index[i];
		while (tmp)
		{
			unsigned long bit;
			_BitScanForward64(&bit, tmp);
			tmp ^= uint64_t(1) << uint64_t(bit);

			int selected = (i << 6) + bit;
			BitSet_Or(merged, &lib[selected * mergedLen], mergedLen);
		}
	}
}

inline bool BitSet_GetBit(uint64_t *a, uint64_t b)
{
	return bool(a[b >> 6] & (uint64_t(1) << uint64_t(b & 0x3F)));
}

inline void BitSet_SetBit(uint64_t *a, uint64_t b)
{
	a[b >> 6] |= uint64_t(1) << uint64_t(b & 0x3F);
}

inline void BitSet_UnsetBit(uint64_t *a, uint64_t b)
{
	a[b >> 6] &= ~(uint64_t(1) << uint64_t(b & 0x3F));
}

inline void BitSet_FlipBit(uint64_t *a, uint64_t b)
{
	a[b >> 6] ^= uint64_t(1) << uint64_t(b & 0x3F);
}

inline void BitSet_SetAll(uint64_t *a, size_t len)
{
	memset(a, 0xFFFFFFFF, len << 3);
}

inline void BitSet_SetNone(uint64_t *a, size_t len)
{
	memset(a, 0, len << 3);
}

inline void BitSet_SetRandom(uint64_t *a, size_t len, int density)
{
	for (int i = 0; i < len; i++)
	{
		a[i] = _rng();
		for (int ii = 0; ii < density; ii++) a[i] &= _rng();
	}
}

static PyObject* bitset_count(PyObject* self, PyObject* args)
{
	PyArrayObject *a = NULL;
	if (!PyArg_ParseTuple(args, "O!", &PyArray_Type, &a)) return NULL;

	size_t a_len = PyArray_NBYTES(a) >> 3;
	uint64_t *a_data = (uint64_t*)a->data;
	size_t c = size_t(BitSet_Count(a_data, a_len));
	return PyLong_FromSize_t(c);
}

static PyObject* bitset_and(PyObject* self, PyObject* args)
{
	PyArrayObject *a = NULL, *b = NULL;
	if (!PyArg_ParseTuple(args, "O!O!", &PyArray_Type, &a, &PyArray_Type, &b)) return NULL;
	
	if (!PyArray_CHKFLAGS(a, NPY_ARRAY_CARRAY))
	{
		PyErr_SetString(PyExc_Exception, "Numpy array in a is not a c-array");
		return NULL;
	}

	if (!PyArray_CHKFLAGS(b, NPY_ARRAY_CARRAY))
	{
		PyErr_SetString(PyExc_Exception, "Numpy array in b is not a c-array");
		return NULL;
	}

	size_t a_len = PyArray_NBYTES(a) >> 3;
	size_t b_len = PyArray_NBYTES(b) >> 3;
	uint64_t *a_data = (uint64_t*)a->data;
	uint64_t *b_data = (uint64_t*)b->data;

	size_t len = (a_len < b_len) ? a_len : b_len;
	BitSet_And(a_data, b_data, len);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject* bitset_or(PyObject* self, PyObject* args)
{
	PyArrayObject *a = NULL, *b = NULL;
	if (!PyArg_ParseTuple(args, "O!O!", &PyArray_Type, &a, &PyArray_Type, &b)) return NULL;

	if (!PyArray_CHKFLAGS(a, NPY_ARRAY_CARRAY))
	{
		PyErr_SetString(PyExc_Exception, "Numpy array in a is not a c-array");
		return NULL;
	}

	if (!PyArray_CHKFLAGS(b, NPY_ARRAY_CARRAY))
	{
		PyErr_SetString(PyExc_Exception, "Numpy array in b is not a c-array");
		return NULL;
	}

	size_t a_len = PyArray_NBYTES(a) >> 3;
	size_t b_len = PyArray_NBYTES(b) >> 3;
	uint64_t *a_data = (uint64_t*)a->data;
	uint64_t *b_data = (uint64_t*)b->data;

	size_t len = (a_len < b_len) ? a_len : b_len;
	BitSet_Or(a_data, b_data, len);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject* bitset_xor(PyObject* self, PyObject* args)
{
	PyArrayObject *a = NULL, *b = NULL;
	if (!PyArg_ParseTuple(args, "O!O!", &PyArray_Type, &a, &PyArray_Type, &b)) return NULL;

	if (!PyArray_CHKFLAGS(a, NPY_ARRAY_CARRAY))
	{
		PyErr_SetString(PyExc_Exception, "Numpy array in a is not a c-array");
		return NULL;
	}

	if (!PyArray_CHKFLAGS(b, NPY_ARRAY_CARRAY))
	{
		PyErr_SetString(PyExc_Exception, "Numpy array in b is not a c-array");
		return NULL;
	}

	size_t a_len = PyArray_NBYTES(a) >> 3;
	size_t b_len = PyArray_NBYTES(b) >> 3;
	uint64_t *a_data = (uint64_t*)a->data;
	uint64_t *b_data = (uint64_t*)b->data;

	size_t len = (a_len < b_len) ? a_len : b_len;
	BitSet_Xor(a_data, b_data, len);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject* bitset_and_count(PyObject* self, PyObject* args)
{
	PyArrayObject *a = NULL, *b = NULL;
	if (!PyArg_ParseTuple(args, "O!O!", &PyArray_Type, &a, &PyArray_Type, &b)) return NULL;

	if (!PyArray_CHKFLAGS(a, NPY_ARRAY_CARRAY))
	{
		PyErr_SetString(PyExc_Exception, "Numpy array in a is not a c-array");
		return NULL;
	}

	if (!PyArray_CHKFLAGS(b, NPY_ARRAY_CARRAY))
	{
		PyErr_SetString(PyExc_Exception, "Numpy array in b is not a c-array");
		return NULL;
	}

	size_t a_len = PyArray_NBYTES(a) >> 3;
	size_t b_len = PyArray_NBYTES(b) >> 3;
	uint64_t *a_data = (uint64_t*)a->data;
	uint64_t *b_data = (uint64_t*)b->data;

	size_t len = (a_len < b_len) ? a_len : b_len;
	size_t c = size_t(BitSet_And_Count(a_data, b_data, len));

	return PyLong_FromSize_t(c);
}

static PyObject* bitset_or_count(PyObject* self, PyObject* args)
{
	PyArrayObject *a = NULL, *b = NULL;
	if (!PyArg_ParseTuple(args, "O!O!", &PyArray_Type, &a, &PyArray_Type, &b)) return NULL;

	if (!PyArray_CHKFLAGS(a, NPY_ARRAY_CARRAY))
	{
		PyErr_SetString(PyExc_Exception, "Numpy array in a is not a c-array");
		return NULL;
	}

	if (!PyArray_CHKFLAGS(b, NPY_ARRAY_CARRAY))
	{
		PyErr_SetString(PyExc_Exception, "Numpy array in b is not a c-array");
		return NULL;
	}

	size_t a_len = PyArray_NBYTES(a) >> 3;
	size_t b_len = PyArray_NBYTES(b) >> 3;
	uint64_t *a_data = (uint64_t*)a->data;
	uint64_t *b_data = (uint64_t*)b->data;

	size_t len = (a_len < b_len) ? a_len : b_len;
	size_t c = size_t(BitSet_Or_Count(a_data, b_data, len));

	return PyLong_FromSize_t(c);
}

static PyObject* bitset_xor_count(PyObject* self, PyObject* args)
{
	PyArrayObject *a = NULL, *b = NULL;
	if (!PyArg_ParseTuple(args, "O!O!", &PyArray_Type, &a, &PyArray_Type, &b)) return NULL;

	if (!PyArray_CHKFLAGS(a, NPY_ARRAY_CARRAY))
	{
		PyErr_SetString(PyExc_Exception, "Numpy array in a is not a c-array");
		return NULL;
	}

	if (!PyArray_CHKFLAGS(b, NPY_ARRAY_CARRAY))
	{
		PyErr_SetString(PyExc_Exception, "Numpy array in b is not a c-array");
		return NULL;
	}

	size_t a_len = PyArray_NBYTES(a) >> 3;
	size_t b_len = PyArray_NBYTES(b) >> 3;
	uint64_t *a_data = (uint64_t*)a->data;
	uint64_t *b_data = (uint64_t*)b->data;

	size_t len = (a_len < b_len) ? a_len : b_len;
	size_t c = size_t(BitSet_Xor_Count(a_data, b_data, len));

	return PyLong_FromSize_t(c);
}

static PyObject* bitset_getbit(PyObject* self, PyObject* args)
{
	PyArrayObject *a = NULL; long b;
	if (!PyArg_ParseTuple(args, "O!l", &PyArray_Type, &a, &b)) return NULL;

	if (!PyArray_CHKFLAGS(a, NPY_ARRAY_CARRAY))
	{
		PyErr_SetString(PyExc_Exception, "Numpy array in a is not a c-array");
		return NULL;
	}

	size_t a_len = PyArray_NBYTES(a) >> 3;
	uint64_t *a_data = (uint64_t*)a->data;
	
	if ((b >> 6) >= a_len)
	{
		PyErr_SetString(PyExc_Exception, "Index in b is out of range for numpy array in a");
		return NULL;
	}

	size_t c = size_t(BitSet_GetBit(a_data, b));
	return PyLong_FromSize_t(c);
}

static PyObject* bitset_setbit(PyObject* self, PyObject* args)
{
	PyArrayObject *a = NULL; long b;
	if (!PyArg_ParseTuple(args, "O!l", &PyArray_Type, &a, &b)) return NULL;

	if (!PyArray_CHKFLAGS(a, NPY_ARRAY_CARRAY))
	{
		PyErr_SetString(PyExc_Exception, "Numpy array in a is not a c-array");
		return NULL;
	}

	size_t a_len = PyArray_NBYTES(a) >> 3;
	uint64_t *a_data = (uint64_t*)a->data;

	if ((b >> 6) >= a_len)
	{
		PyErr_SetString(PyExc_Exception, "Index in b is out of range for numpy array in a");
		return NULL;
	}

	BitSet_SetBit(a_data, b);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject* bitset_unsetbit(PyObject* self, PyObject* args)
{
	PyArrayObject *a = NULL; long b;
	if (!PyArg_ParseTuple(args, "O!l", &PyArray_Type, &a, &b)) return NULL;

	if (!PyArray_CHKFLAGS(a, NPY_ARRAY_CARRAY))
	{
		PyErr_SetString(PyExc_Exception, "Numpy array in a is not a c-array");
		return NULL;
	}

	size_t a_len = PyArray_NBYTES(a) >> 3;
	uint64_t *a_data = (uint64_t*)a->data;

	if ((b >> 6) >= a_len)
	{
		PyErr_SetString(PyExc_Exception, "Index in b is out of range for numpy array in a");
		return NULL;
	}

	BitSet_UnsetBit(a_data, b);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject* bitset_flipbit(PyObject* self, PyObject* args)
{
	PyArrayObject *a = NULL; long b;
	if (!PyArg_ParseTuple(args, "O!l", &PyArray_Type, &a, &b)) return NULL;

	if (!PyArray_CHKFLAGS(a, NPY_ARRAY_CARRAY))
	{
		PyErr_SetString(PyExc_Exception, "Numpy array in a is not a c-array");
		return NULL;
	}

	size_t a_len = PyArray_NBYTES(a) >> 3;
	uint64_t *a_data = (uint64_t*)a->data;

	if ((b >> 6) >= a_len)
	{
		PyErr_SetString(PyExc_Exception, "Index in b is out of range for numpy array in a");
		return NULL;
	}

	BitSet_FlipBit(a_data, b);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject* bitset_setall(PyObject* self, PyObject* args)
{
	PyArrayObject *a = NULL;
	if (!PyArg_ParseTuple(args, "O!", &PyArray_Type, &a)) return NULL;

	if (!PyArray_CHKFLAGS(a, NPY_ARRAY_CARRAY))
	{
		PyErr_SetString(PyExc_Exception, "Numpy array in a is not a c-array");
		return NULL;
	}

	size_t a_len = PyArray_NBYTES(a) >> 3;
	uint64_t *a_data = (uint64_t*)a->data;

	BitSet_SetAll(a_data, a_len);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject* bitset_setnone(PyObject* self, PyObject* args)
{
	PyArrayObject *a = NULL;
	if (!PyArg_ParseTuple(args, "O!", &PyArray_Type, &a)) return NULL;

	if (!PyArray_CHKFLAGS(a, NPY_ARRAY_CARRAY))
	{
		PyErr_SetString(PyExc_Exception, "Numpy array in a is not a c-array");
		return NULL;
	}

	size_t a_len = PyArray_NBYTES(a) >> 3;
	uint64_t *a_data = (uint64_t*)a->data;

	BitSet_SetNone(a_data, a_len);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject* bitset_setrandom(PyObject* self, PyObject* args)
{
	PyArrayObject *a = NULL; long density = 0;
	if (!PyArg_ParseTuple(args, "O!|l", &PyArray_Type, &a, &density)) return NULL;

	if (!PyArray_CHKFLAGS(a, NPY_ARRAY_CARRAY))
	{
		PyErr_SetString(PyExc_Exception, "Numpy array in a is not a c-array");
		return NULL;
	}

	size_t a_len = PyArray_NBYTES(a) >> 3;
	uint64_t *a_data = (uint64_t*)a->data;

	if ((density < 0) || (density > 64)) density = 0;
	BitSet_SetRandom(a_data, a_len, density);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject* bitset_selectmerge(PyObject* self, PyObject* args)
{
	PyArrayObject *merged = NULL, *lib = NULL, *index = NULL;
	if (!PyArg_ParseTuple(args, "O!O!O!", &PyArray_Type, &merged, &PyArray_Type, &lib, &PyArray_Type, &index)) return NULL;

	if (!PyArray_CHKFLAGS(merged, NPY_ARRAY_CARRAY))
	{
		PyErr_SetString(PyExc_Exception, "Numpy array in a is not a c-array");
		return NULL;
	}

	if (!PyArray_CHKFLAGS(lib, NPY_ARRAY_CARRAY))
	{
		PyErr_SetString(PyExc_Exception, "Numpy array in b is not a c-array");
		return NULL;
	}

	if (!PyArray_CHKFLAGS(index, NPY_ARRAY_CARRAY))
	{
		PyErr_SetString(PyExc_Exception, "Numpy array in c is not a c-array");
		return NULL;
	}

	size_t mergedLen = PyArray_NBYTES(merged) >> 3;
	size_t libLen = PyArray_NBYTES(lib) >> 3;
	size_t indexLen = PyArray_NBYTES(index) >> 3;

	if (libLen < ((indexLen << 6) * mergedLen))
	{
		PyErr_SetString(PyExc_Exception, "Lib array is not large enough for specified index and merge dimensions");
		return NULL;
	}

	uint64_t *merged_data = (uint64_t*)merged->data;
	uint64_t *lib_data = (uint64_t*)lib->data;
	uint64_t *index_data = (uint64_t*)index->data;

	BitSet_SelectMerge(merged_data, lib_data, index_data, indexLen, mergedLen);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject* bitset_sumlist(PyObject* self, PyObject* args)
{
	PyArrayObject *a = NULL; PyObject *b;
	if (!PyArg_ParseTuple(args, "O!O", &PyArray_Type, &a, &b)) return NULL;

	if (!PyArray_CHKFLAGS(a, NPY_ARRAY_CARRAY))
	{
		PyErr_SetString(PyExc_Exception, "Numpy array in a is not a c-array");
		return NULL;
	}

	if (!PyList_Check(b))
	{
		PyErr_SetString(PyExc_Exception, "Parameter b is not a Python list");
		return NULL;
	}

	size_t a_len = PyArray_NBYTES(a) >> 3;
	uint64_t *a_data = (uint64_t*)a->data;

	size_t listSize = PyList_Size(b);
	for (int i = 0; i < listSize; i++)
	{
		PyObject *obj = PyList_GET_ITEM(b, i);
		if (!PyObject_IsInstance(obj, (PyObject*)&PyArray_Type))
		{
			PyErr_SetString(PyExc_Exception, "Python list in b contains objects that are not numpy arrays");
			return NULL;
		}

		PyArrayObject *arr = (PyArrayObject*)obj;
		if (!PyArray_CHKFLAGS(arr, NPY_ARRAY_CARRAY))
		{
			PyErr_SetString(PyExc_Exception, "Numpy array in Python list b is not a c-array");
			return NULL;
		}

		size_t arrLen = PyArray_NBYTES(arr) >> 3;
		uint64_t *data = (uint64_t*)arr->data;

		size_t len = ((arrLen << 6) < a_len) ? (arrLen << 6) : a_len;
		for (int b = 0; b < len; b++) a_data[b] += BitSet_GetBit(data, b);
	}
	
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject* bitset_seedrandom(PyObject* self, PyObject* args)
{
	long seed = NULL;
	if (!PyArg_ParseTuple(args, "|l", &seed)) return NULL;

	if (seed)
	{
		_rng.seed(seed);
	}
	else
	{
		std::random_device _seeder;
		_rng.seed(_seeder());
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject* bitset_samplerandom(PyObject* self, PyObject* args)
{
	PyArrayObject *a = NULL;
	if (!PyArg_ParseTuple(args, "O!", &PyArray_Type, &a)) return NULL;

	if (!PyArray_CHKFLAGS(a, NPY_ARRAY_CARRAY))
	{
		PyErr_SetString(PyExc_Exception, "Numpy array in a is not a c-array");
		return NULL;
	}
	
	size_t a_len = PyArray_NBYTES(a) >> 3;
	uint64_t *a_data = (uint64_t*)a->data;
	
	uint64_t total = 0; for (int i = 0; i < a_len; i++) total += a_data[i];
	if (!total)
	{
		PyErr_SetString(PyExc_Exception, "Error - Cannot sample empty distribution");
		return NULL;		
	}
	
	uint64_t rnd = _rng() % total;
	uint64_t count = 0;
	size_t c;
	for (c = 0; c < a_len; c++)
	{
		if ((rnd >= count) && (rnd < (count + a_data[c]))) break;
		count += a_data[c];
	}

	return PyLong_FromSize_t(c);
}

static PyObject* bitset_thresholdmask(PyObject* self, PyObject* args)
{
	PyArrayObject *mask = NULL, *counts = NULL; long threshold;
	if (!PyArg_ParseTuple(args, "O!O!l", &PyArray_Type, &mask, &PyArray_Type, &counts, &threshold)) return NULL;

	if (!PyArray_CHKFLAGS(mask, NPY_ARRAY_CARRAY))
	{
		PyErr_SetString(PyExc_Exception, "Numpy array in a is not a c-array");
		return NULL;
	}

	if (!PyArray_CHKFLAGS(counts, NPY_ARRAY_CARRAY))
	{
		PyErr_SetString(PyExc_Exception, "Numpy array in b is not a c-array");
		return NULL;
	}

	size_t maskLen = PyArray_NBYTES(mask) >> 3;
	size_t countsLen = PyArray_NBYTES(counts) >> 3;

	if (maskLen < (countsLen >> 6))
	{
		PyErr_SetString(PyExc_Exception, "Mask array is not large enough for specified count array");
		return NULL;
	}

	uint64_t *mask_data = (uint64_t*)mask->data;
	uint64_t *counts_data = (uint64_t*)counts->data;

	uint64_t offset = 0, bit = 1; memset(mask_data, 0, maskLen >> 3);
	for (int i = 0; i < countsLen; i++)
	{
		if (counts_data[i] < threshold) mask_data[offset] |= bit;
		
		const uint64_t overflow = bit >> 0x3F;
		offset += overflow;
		bit = (bit << 1) | overflow;
	}
	
	Py_INCREF(Py_None);
	return Py_None;
}

static PyMethodDef methods[] = {
	{ "count", bitset_count, METH_VARARGS, "Returns the population count of the specified bitset" },
	{ "bitwise_and", bitset_and, METH_VARARGS, "Binary AND a and b into a" },
	{ "bitwise_or", bitset_or, METH_VARARGS, "Binary OR a and b into a" },
	{ "bitwise_xor", bitset_xor, METH_VARARGS, "Binary XOR a and b into a" },
	{ "bitwise_and_count", bitset_and_count, METH_VARARGS, "Binary AND a and b into a, returns set pop count of a" },
	{ "bitwise_or_count", bitset_or_count, METH_VARARGS, "Binary OR a and b into a, returns set pop count of a" },
	{ "bitwise_xor_count", bitset_xor_count, METH_VARARGS, "Binary XOR and and b into a, returns set pop count" },
	{ "get_bit", bitset_getbit, METH_VARARGS, "Get the value of the bit in a specified by index b" },
	{ "set_bit", bitset_setbit, METH_VARARGS, "Set the value of the bit in a specified by index b" },
	{ "unset_bit", bitset_unsetbit, METH_VARARGS, "Unset the value of the bit in a specified by index b" },
	{ "flip_bit", bitset_flipbit, METH_VARARGS, "Flip the value of the bit in a specified by index b" },
	{ "set_all", bitset_setall, METH_VARARGS, "Set all bits in a" },
	{ "set_none", bitset_setnone, METH_VARARGS, "Zero all bits in a" },
	{ "set_random", bitset_setrandom, METH_VARARGS, "Reset a to random bits, optionally specify the -log2 density as a (positive) integer in b" },
	{ "select_merge", bitset_selectmerge, METH_VARARGS, "Selects bitsets of the same length as a from the 2d array b specified by the mask c, and binary ORs the results into a" },
	{ "sum_list", bitset_sumlist, METH_VARARGS, "Adds the Python list of numpy arrays in b into np.uint64 counts for each bit in a" },
	{ "seed_random", bitset_seedrandom, METH_VARARGS, "Seeds the internal random number generator for the module. If no argument is provided a random seed is used" },
	{ "sample_random", bitset_samplerandom, METH_VARARGS, "Samples the distribution in the numpy np.uint64 array in a and returns the winning index" },
	{ "threshold_mask", bitset_thresholdmask, METH_VARARGS, "Set bits in a if the np.uint64 values in numpy array b exceed the threshold count in c" },
	{ NULL, NULL, 0, NULL }
};

static struct PyModuleDef bitset_module = {
	PyModuleDef_HEAD_INIT,
	"bitset",
	"Fast Bitfield Sets",
	-1,
	methods
};

PyMODINIT_FUNC PyInit_bitset(void)
{
	import_array();

	PyObject *mod = PyModule_Create(&bitset_module);
	if (!mod) return NULL;

	return mod;
}
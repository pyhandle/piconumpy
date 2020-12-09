# PicoNumpy

[![Build Status](https://travis-ci.org/paugier/piconumpy.svg?branch=master)](https://travis-ci.org/paugier/piconumpy)

**An experiment about Numpy and HPy**

The C API of CPython is one of the cause of the success of Python in scientific
computing. In particular, Numpy (and all the Python scientific stack) is built
on top of this API. However, some characteristics of this API start to be an
issue for the future of scientific Python (see [1], [2], [HPy]).

[1]: https://faster-cpython.readthedocs.io/
[2]: https://morepypy.blogspot.com/2019/12/hpy-kick-off-sprint-report.html
[HPy]: https://github.com/hpyproject/hpy

[HPy] is a very ambitious and promissing project to design a new and better C
API for interacting with Python interpreters. It should allow people to write
Python extensions efficient on different interpreters (CPython, PyPy, Jython,
IronPython, GraalPython, RustPython, etc.).

PyPy would be especially useful for some scientific applications. For example
for Integration and ODEs
([scipy.integrate](https://docs.scipy.org/doc/scipy/reference/integrate.html)),
for which there are a lot of callbacks of very small functions. This repository
contains [a tiny benchmark](bench/without_numpy) showing that as long as Numpy
is not used, PyPy is very efficient for such task. Unfortunately, as soon as
Numpy is used, PyPy becomes very slow!

[bench/without_numpy]: https://github.com/paugier/piconumpy/blob/master/bench/without_numpy/

With PicoNumpy, I'd like to study if [HPy] could help for codes using Numpy and
callbacks of small Python functions.

We start by a [simple but realistic benchmark](bench/bench_array1d.py) (the
slow loops only involve pure-Python and very simple Numpy). We then wrote a
tiny ("pico") implementation of a Numpy like object (just sufficient to run the
benchmark).

The next task is to reimplement PicoNumpy using [HPy] and to check if PyPy
could efficiently accelerate [our main benchmark](bench/bench_array1d.py).

PicoNumpy is really tiny. It just provides an `array` class (one-dimensional)
supporting:

- Instantiation from a list of floats
- Elementwise multiplication and division by a float
- Elementwise addition (of 2 arrays)
- Indexing
- `len`

A good acceleration by PyPy of our example would be a great proof that the
scientific Python community has to invest time and energy on [HPy].

In the script [bench_array1d.py](bench/bench_array1d.py), Transonic is used for
the benchmark and comparison. With Transonic-Pythran, we typically get a 50
speedup compared to CPython (and ~400 versus PyPy, which is still very slow for
such codes using Numpy).

[bench/bench_array1d.py]: https://github.com/paugier/piconumpy/blob/master/bench/bench_array1d.py

## Install and run the benchmarks

**Warning:** PicoNumpy now depends on HPy, which still has to be installed from
the [Git repository](https://github.com/hpyproject/hpy). For now, the
installation is a bit more complex that what is described here (more about this
[here](#more-precise-notes-on-how-to-install-and-run-the-benchmarks-with-PyPy)).

`make` should install the package in editable mode. `cd bench; make` should run
the benchmarks. For the benchmarks, Julia is used for a good comparison point
so the command `julia` has to be available.

For PyPy, the Makefiles are sensible to the environment variable `PYTHON`, so
you could do:

```bash
export PYTHON=pypy3
make
cd bench
make
```

The benchmark code can be profiled for the different implementations with the
commands (you need gprof2dot and graphviz):

```bash
cd bench
make profile METHOD="cpython-c-api"
make profile METHOD="purepy_array"
make profile METHOD="purepy"
make profile METHOD="cython"
```

### More precise notes on how to install and run the benchmarks with PyPy

Download and extract a nightly PyPy build
<https://buildbot.pypy.org/nightly/>. Add to the `PATH` environment variable
the path of the directory containing the `pypy` executable (something like
`~/opt/pypy-c-jit-101190-b661dc329618-linux64/bin`). Then, you should be able
to run:

```bash
pypy -m ensurepip
pypy -m pip install pip -U
pypy -m pip install numpy cython pytest transonic pythran
```

We need to install the correct version of HPy for the version of PyPy we are using:

```bash
pypy -c "import hpy.universal as u; print(u.get_version()"
```

gives `('0.1.dev875+g7c832a2', '7c832a2')`.

```bash
cd ~/Dev/hpy
# update to the correct commit
pypy setup.py develop
```

Now we can build-install PicoNumpy:

```bash
cd ~/Dev/piconumpy
pypy setup.py --hpy-abi=universal develop
```

And run the benchmarks with:

```bash
export PYTHON="pypy"
make clean
make bench_hpy
make
```

## Few results

As of today (8 Dec 2020), HPy is not yet ready for high performance, but at
least (with HPy git revision 7c832a2f) it runs !

### At home (Intel(R) Core(TM) i5-8400 CPU @ 2.80GHz)

- With CPython

```
Julia                      :     1 * norm = 0.00194 s
PicoNumpy (CPython C-API)  :  9.03 * norm
PicoNumpy (HPy CPy ABI)    :  9.87 * norm
PicoNumpy (HPy Universal)  :  12.5 * norm
Transonic-Pythran          : 0.546 * norm
Numpy                      :  37.5 * norm
PicoNumpy (purepy)         :  39.2 * norm
PicoNumpy (purepy_array)   :  37.1 * norm
PicoNumpy (Cython)         :  28.5 * norm
```

- With PyPy3

```
Julia                      :     1 * norm = 0.00194 s
PicoNumpy (CPython C-API)  :  33.3 * norm
PicoNumpy (HPy Universal)  :    15 * norm
Transonic-Pythran          : 0.629 * norm
Numpy                      :   332 * norm
PicoNumpy (purepy)         :  4.36 * norm
PicoNumpy (purepy_array)   :  7.69 * norm
PicoNumpy (Cython)         :   247 * norm
```

#### Simpler benchmarks (bench_cpy_vs_hpy.py)

- With CPython

```
CPython C-API:   1.83 seconds
HPy [CPy ABI]:   2.20 seconds
HPy [Universal]: 2.73 seconds
```

- With PyPy3

```
CPython C-API:   5.88 seconds
HPy [Universal]: 3.17 seconds
```

### At work (meige8pcpa79, Intel(R) Xeon(R) CPU E5-1603 v3 @ 2.80GHz)

- With CPython

```raw
Julia                      :     1 * norm = 0.00534 s
Transonic-Pythran          : 0.564 * norm
Numpy                      :  18.2 * norm
PicoNumpy (purepy)         :  23.7 * norm
PicoNumpy (purepy_array)   :  21.7 * norm
PicoNumpy (Cython)         :  15.6 * norm
PicoNumpy (CPython C-API)  :   4.7 * norm
```

- With PyPy3

```raw
Julia                      :     1 * norm = 0.00534 s
Transonic-Pythran          : 0.603 * norm
Numpy                      :   183 * norm
PicoNumpy (purepy)         :   2.4 * norm
PicoNumpy (purepy_array)   :  3.73 * norm
PicoNumpy (Cython)         :   135 * norm
PicoNumpy (CPython C-API)  :  17.8 * norm
```

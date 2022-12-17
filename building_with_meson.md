# Building with Meson

_Note: this is for early adopters. It has been tested on Linux and macOS, and
with Python 3.9-3.12. Windows will be tested soon. There is one CI job to keep
the build stable. This may have rough edges, please open an issue if you run
into a problem._

### Developer build

**Install build tools:** Use one of:

- `mamba create -f environment.yml
- `pip install -r build_requirements.txt  # also make sure you have pkg-config and the usual system dependencies for NumPy`

**Compile and install:** `./dev.py build`

This builds in the `build/` directory, and installs into the `build-install` directory.

Then run the test suite or a shell via `dev.py`:
```
./dev.py test
./dev.py ipython
```

Alternatively, to use the package, add it to your `PYTHONPATH`:
```
export PYTHONPATH=${PWD}/build/lib64/python3.10/site-packages  # may vary
pytest --pyargs numpy
```


### pip install

Note that `pip` will use the default build system, which is (as of now) still
`numpy.distutils`. In order to switch that default to Meson, uncomment the
`build-backend = "mesonpy"` line at the top of `pyproject.toml`.

After that is done, `pip install .` or `pip install --no-build-isolation .`
will work as expected. As does building an sdist or wheel with `python -m build`.
Note, however, that `pip install -e .` (in-place developer install) does not!
Use `dev.py` instead (see above).



### Workaround for a hiccup on Fedora

- Fedora does not distribute `openblas.pc`. Install the following file in `~/lib/pkgconfig/openblas.pc`:

```
prefix=/usr
includedir=${prefix}/include
libdir=${prefix}/lib64

Name: openblas
Description: OpenBLAS is an optimized BLAS library based on GotoBLAS2 1.13 BSD version
Version: 0.3.19
Cflags: -I${includedir}/openblas
Libs: -L${libdir} -lopenblas
```

Then build with:

```
./dev.py build -- -Dpkg_config_path=${HOME}/lib/pkgconfig
```

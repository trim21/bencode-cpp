import sys
import os
from glob import glob

from setuptools import setup, find_packages
from pybind11.setup_helpers import Pybind11Extension, build_ext

macro = [("FMT_HEADER_ONLY", "")]

extra_compile_args = None
# if os.environ.get("BENCODE_CPP_DEBUG") == "1":
# macro.append(("BENCODE_CPP_DEBUG", "1"))
if sys.platform == "win32":
    extra_compile_args = ["/utf-8"]

module = Pybind11Extension(
    "bencode_cpp._bencode",
    sources=sorted(glob("./src/bencode_cpp/*.cpp")),
    include_dirs=["./src/bencode_cpp", "./vendor/fmt/include"],
    define_macros=macro,
    extra_compile_args=extra_compile_args,
)

setup(
    ext_modules=[module],
    packages=find_packages("src"),
    package_dir={"": "src"},
    package_data={"": ["*.h", "*.cpp", "*.pyi", "py.typed"]},
    include_package_data=True,
    cmdclass={"build_ext": build_ext},
    zip_safe=False,
)

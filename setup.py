import platform
import sys
import os
from glob import glob

import os

from setuptools import setup, find_packages
from pybind11.setup_helpers import Pybind11Extension, build_ext

macro = [("FMT_HEADER_ONLY", "")]

extra_compile_args = None
# if os.environ.get("BENCODE_CPP_DEBUG") == "1":
# macro.append(("BENCODE_CPP_DEBUG", "1"))
if sys.platform == "win32":
    extra_compile_args = ["/utf-8"]
    if platform.machine() == "AMD64":
        TRIPLET = "x64-windows"
    else:
        raise AssertionError("only x64 are supported on win32")
else:
    if platform.machine() in ("aarch64", "arm64"):
        TRIPLET = "arm64"
    elif platform.machine() in ("x86_64", "x64"):
        TRIPLET = "x64"
    else:
        raise AssertionError(f"unknown arch {platform.machine()=!r}")

    if sys.platform == "linux":
        TRIPLET = TRIPLET + "-Linux"
    elif sys.platform == "darwin":
        TRIPLET = TRIPLET + "-osx"
    else:
        raise AssertionError(f"unknown platform {sys.platform=!r}")

module = Pybind11Extension(
    "bencode_cpp._bencode",
    sources=sorted(glob("./src/bencode_cpp/*.cpp")),
    include_dirs=[
        "./src/bencode_cpp",
        os.path.join(os.environ["VCPKG_ROOT"], f"installed/{TRIPLET}/include"),
    ],
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

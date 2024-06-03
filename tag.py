import sys

import subprocess

glibc_version = subprocess.getoutput("ldd --version").splitlines()[0].split()[-1]

print("manylinux_" + glibc_version.replace(".", "_") + "_x86_64")

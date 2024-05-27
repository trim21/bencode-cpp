# bencode-c

bencode serialize/deserialize written in native c extension.

```shell
pip install bencode-c
```

```python
import bencode_c

# NOTICE: we decode bencode bytes to bytes, not str.
assert bencode_c.bdecode(b'd5:hello5:worlde') == {b'hello': b'world'}

assert bencode_c.bencode(...) == b'...'
```

## Benchmark

this maybe the fastest bencode library in python.

compared packages:

- `abi3`: native c extension (this package) [bencode-c](https://pypi.org/project/bencode-c)
- `py`: pure python implement [bencode-py](https://pypi.org/project/bencode-py)
- `cy`: cython implement [fast-bencode](https://pypi.org/project/fast-bencode)
- `mypy`: pure python implement with mypyc [bencode2](https://pypi.org/project/bencode2)

test cases are 40 torrents from real world.

(windows, python3.10 AMD R7 5800X)

### Decode

```
------------------------------------------------------------ benchmark 'case=decode': 4 tests ------------------------------------------------------------
Name (time in us)                      Mean                    Min                    Max                 Median              StdDev            Iterations
----------------------------------------------------------------------------------------------------------------------------------------------------------
test_benchmark[decode-abi3]      1,024.6516 (1.0)         987.3901 (1.0)       1,097.5088 (1.0)       1,010.7185 (1.0)       42.6630 (1.0)            1013
test_benchmark[decode-cy]        2,471.1399 (2.41)      2,269.6842 (2.30)      2,592.6601 (2.36)      2,484.0398 (2.46)     121.1285 (2.84)           1000
test_benchmark[decode-mypy]      7,451.1057 (7.27)      7,319.7134 (7.41)      7,697.5504 (7.01)      7,399.4402 (7.32)     149.9720 (3.52)            127
test_benchmark[decode-py]       21,020.7260 (20.51)    20,832.5390 (21.10)    21,180.5220 (19.30)    21,047.4730 (20.82)    139.9841 (3.28)            100
----------------------------------------------------------------------------------------------------------------------------------------------------------
```

### Encode

```
--------------------------------------------------- benchmark 'case=encode': 4 tests ---------------------------------------------------
Name (time in ms)                  Mean                Min                Max             Median            StdDev            Iterations
----------------------------------------------------------------------------------------------------------------------------------------
test_benchmark[encode-abi3]      4.2812 (1.0)       4.1505 (1.0)       4.3833 (1.0)       4.2897 (1.0)      0.0843 (1.56)           1000
test_benchmark[encode-mypy]      5.0277 (1.17)      4.9340 (1.19)      5.0887 (1.16)      5.0244 (1.17)     0.0606 (1.12)           1000
test_benchmark[encode-cy]        5.7779 (1.35)      5.7304 (1.38)      5.8682 (1.34)      5.7562 (1.34)     0.0541 (1.0)             178
test_benchmark[encode-py]       13.7456 (3.21)     13.5073 (3.25)     14.0211 (3.20)     13.5974 (3.17)     0.2524 (4.67)            100
----------------------------------------------------------------------------------------------------------------------------------------
```

(linux, python3.10, Intel G6405)

### Decode

```
--------------------------------------------------- benchmark 'case=decode': 4 tests ---------------------------------------------------
Name (time in ms)                  Mean                Min                Max             Median            StdDev            Iterations
----------------------------------------------------------------------------------------------------------------------------------------
test_benchmark[decode-abi3]      1.9905 (1.0)       1.6988 (1.0)       2.4797 (1.0)       1.9208 (1.0)      0.3053 (1.0)             121
test_benchmark[decode-cy]        3.1906 (1.60)      2.6953 (1.59)      3.6342 (1.47)      3.2118 (1.67)     0.3971 (1.30)            100
test_benchmark[decode-mypy]     12.7124 (6.39)     11.4128 (6.72)     14.8851 (6.00)     12.4180 (6.46)     1.3009 (4.26)             18
test_benchmark[decode-py]       36.5785 (18.38)    33.5474 (19.75)    44.7655 (18.05)    34.5241 (17.97)    4.7173 (15.45)            10
----------------------------------------------------------------------------------------------------------------------------------------
```

### Encode

```
--------------------------------------------------- benchmark 'case=encode': 4 tests ---------------------------------------------------
Name (time in ms)                  Mean                Min                Max             Median            StdDev            Iterations
----------------------------------------------------------------------------------------------------------------------------------------
test_benchmark[encode-abi3]      4.5476 (1.0)       4.2847 (1.0)       4.9841 (1.0)       4.4301 (1.0)      0.2711 (1.0)             100
test_benchmark[encode-cy]        6.3922 (1.41)      5.9589 (1.39)      6.8652 (1.38)      6.1731 (1.39)     0.4102 (1.51)            100
test_benchmark[encode-mypy]      7.8197 (1.72)      7.2574 (1.69)      8.3987 (1.69)      7.8091 (1.76)     0.4947 (1.82)            100
test_benchmark[encode-py]       23.9431 (5.26)     23.4045 (5.46)     24.2830 (4.87)     24.0749 (5.43)     0.3818 (1.41)             10
----------------------------------------------------------------------------------------------------------------------------------------
```

# development

```shell
git clone -r https://github.com/trim21/bencode-c bencode-c
cd bencode-c
python -m venv .venv
# enable venv
source .venv/bin/activate

pip install -e .

pytest -sv
```

`CMakeLists.txt` is for IDE to find includes, not for building files.

use `setup.py` to build python extension.

# see https://github.com/trim21/bencode-py 

# bencode-cpp

bencode serialize/deserialize written with c++, with pybind11.

```shell
pip install bencode-cpp
```

```python
import bencode_cpp

# NOTICE: we decode bencode bytes to bytes, not str.
assert bencode_cpp.bdecode(b'd5:hello5:worlde') == {b'hello': b'world'}

assert bencode_cpp.bencode(...) == b'...'
```

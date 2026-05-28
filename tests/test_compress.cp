// test_compress.cp - test compression round-trip
打印("=== Deflate ===")
data = "Hello World Hello World Hello World"
comp = compress(data)
打印(comp)
decomp = decompress(comp)
打印(decomp)

打印("=== Gzip ===")
gz = gzipCompress(data)
打印(gz)
ungz = gzipDecompress(gz)
打印(ungz)

// Test empty/minimal
打印("=== Edge cases ===")
small = compress("A")
打印(small)
back = decompress(small)
打印(back)

// Gzip roundtrip with longer text
long = "The quick brown fox jumps over the lazy dog. "
long = long + long + long + long
gz2 = gzipCompress(long)
ungz2 = gzipDecompress(gz2)
打印(ungz2)

打印("COMPRESS_OK")

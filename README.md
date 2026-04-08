# ZIP301

**Compression** — Kanaan & Jared  
**Decompression** — Jackson

---

## Compression

Huffman coding using byte-frequency analysis over the input file. A min-heap builds the Huffman tree, and each byte is replaced by its variable-length code. The `.zip301` format stores a plain-text code table in the header, followed by a bit count and the raw binary-encoded payload.

**Usage:**
```
./Compress <file.txt>
```
Output is written to `{filename}.zip301` alongside the input.

### Benchmark Results

Tested on macOS ARM64.

| File | Original Size | Compressed Size | Ratio | Runtime |
|------|:-------------:|:---------------:|:-----:|:-------:|
| `palindrome.txt` | 20 B | 100 B | 500% | 0.005s |
| `gulliver.txt` | 281 B | 422 B | 150% | 0.003s |
| `jacob5.txt` | 20 KB | 11 KB | 58% | 0.006s |
| `pgp.txt` | 157 KB | 91 KB | 58% | 0.031s |
| `dc.txt` | 713 KB | 412 KB | 58% | 0.074s |
| `bom.txt` | 1.5 MB | 840 KB | 57% | 0.149s |
| `kjvbible.txt` | 4.2 MB | 2.4 MB | 57% | 0.427s |
| `canon.txt` | 6.5 MB | 3.7 MB | 57% | 0.665s |
| `minilog.txt` | 12 MB | 7.4 MB | 63% | 1.194s |
| `smallog.txt` | 18 MB | 11.4 MB | 63% | 1.864s |
| `log.txt` | 26 MB | 16.5 MB | 63% | 2.732s |
| `biglog.txt` | 52 MB | 33.0 MB | 63% | 5.539s |

> Note: Ratio > 100% for very small files (palindrome, gulliver) — the Huffman code table header exceeds any savings at that scale.

---

## Decompression

Huffman coding with a binary trie for O(1)-per-bit decoding. The compressed `.zip301` format stores a Huffman code table in a plain-text header, followed by a bit count and the raw binary-encoded payload.

**Usage:**
```
./Decompress <file.zip301>
```
Output is written to `{filename}2.txt` alongside the input.

### Benchmark Results

Tested on macOS ARM64. Each file was decompressed and the output was verified against the original plaintext via `diff`.

| File | Original Size | Runtime | Verified |
|------|:-------------:|:-------:|:--------:|
| `palindrome.zip301` | 20 B | 0.028s | ✓ |
| `gulliver.zip301` | 281 B | 0.020s | ✓ |
| `jacob5.zip301` | 20 KB | 0.019s | ✓ |
| `pgp.zip301` | 157 KB | 0.022s | ✓ |
| `dc.zip301` | 713 KB | 0.027s | ✓ |
| `bom.zip301` | 1.5 MB | 0.034s | ✓ |
| `kjvbible.zip301` | 4.2 MB | 0.067s | ✓ |
| `canon.zip301` | 6.5 MB | 0.094s | ✓ |
| `minilog.zip301` | 12 MB | 0.110s | ✓ |
| `smallog.zip301` | 18 MB | 0.163s | ✓ |
| `log.zip301` | 26 MB | 0.237s | ✓ |
| `biglog.zip301` | 52 MB | 0.419s | ✓ |

All 12 files decompressed identically to their originals (`diff` produced no output).

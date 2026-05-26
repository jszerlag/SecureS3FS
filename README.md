# Secure S3FS
 
A modified implementation of [s3fs-fuse](https://github.com/s3fs-fuse/s3fs-fuse) that adds transparent client-side AES-256-CBC encryption, ensuring files are encrypted **before** they leave your machine and are uploaded to Amazon S3.
 
## Overview
 
Standard s3fs delegates data security to Amazon. This project intercepts file I/O at the filesystem level so that only ciphertext ever reaches S3 — you retain full control over your encryption keys and data privacy. Files can be decrypted locally through the mounted bucket, the included standalone AES tool, or OpenSSL directly.
 
## Features
 
- **Transparent encryption/decryption** — applications read and write files normally; encryption is handled entirely by the filesystem
- **AES-256-CBC with salting** — compatible with OpenSSL's `aes-256-cbc` implementation
- **In-place file-level encryption** — optimized read/write strategy avoids redundant temporary files
- **Standalone AES tool** — encrypt or decrypt any file independently of the filesystem, with full OpenSSL interoperability
- **S3 console compatibility** — files downloaded directly from the S3 console are in encrypted form and can be decrypted with either the standalone tool or OpenSSL
## Architecture
 
Encryption and decryption are implemented at the **file level** inside `fdcache_entity.cpp`:
 
- **Decrypt** — called inside `FdEntity::Load()` immediately after a file is downloaded from S3, before it is exposed to any application
- **Encrypt** — called inside `FdEntity::RowFlushHasLock()` just before upload logic runs, covering all upload types (multipart, stream, etc.)
Both functions operate on a single file descriptor and use an optimized in-place read/write strategy:
 
- **Decryption** reads 16 bytes ahead of the write cursor, ensuring plaintext is never written over unread ciphertext
- **Encryption** pre-reads one buffer before the write loop begins, preventing the `Salted__(bytes)` header and growing ciphertext from overwriting unread plaintext
## Standalone AES Tool
 
A separate command-line utility for encrypting and decrypting files using AES-256-CBC.
 
### Usage
 
```bash
./aes <-e|-d> -p <password> -i <infile> -o <outfile> [-salt|-nosalt]
```
 
| Flag | Description |
|------|-------------|
| `-e` | Encrypt |
| `-d` | Decrypt |
| `-p <password>` | Encryption/decryption password |
| `-i <infile>` | Input file path |
| `-o <outfile>` | Output file path (created if it doesn't exist) |
| `-salt` | Use a random 8-byte salt (default, recommended) |
| `-nosalt` | Disable salting |
 
### OpenSSL Interoperability
 
Files encrypted with this tool can be decrypted with OpenSSL and vice versa:
 
```bash
# Decrypt with OpenSSL
openssl enc -d -aes-256-cbc -in encrypted.bin -out decrypted.txt -pass pass:<password>
 
# Encrypt with OpenSSL
openssl enc -e -aes-256-cbc -in plaintext.txt -out encrypted.bin -pass pass:<password>
```
 
> **Note:** OpenSSL may display a deprecation warning about key derivation (`EVP_BytesToKey`). This does not affect functionality; updating to PBKDF2 key derivation is a planned improvement.
 
## Dependencies
 
| Dependency | Version | Purpose |
|------------|---------|---------|
| s3fs-fuse | v1.95 | Base filesystem |
| OpenSSL | v3.0.13+ | EVP API (`evp.h`, `rand.h`, `err.h`) |
| FUSE | 2.9.9 | Userspace filesystem interface |
| GCC | 13.3.0+ | Compilation (`-lcrypto -lssl`) |
 
## Build
 
Follow the standard s3fs build process after cloning this repository:
 
```bash
./autogen.sh
./configure
make
sudo make install
```
 
Compile the standalone AES tool separately:
 
```bash
gcc aes.c -o aes -lcrypto -lssl
```
 
## Mounting
 
```bash
s3fs <bucket-name> <mountpoint> -o passwd_file=<path-to-credentials>
```
 
Once mounted, all reads and writes are transparently encrypted/decrypted. No application-level changes are needed.
 
## Development Environment
 
Developed and tested on **Ubuntu 24.04.3 LTS** via WSL2 on Windows 11 (AMD Ryzen 7 7800X3D).
 
## Known Limitations
 
- File-level (not block-level) encryption introduces a small performance overhead on large files, as data is read twice during an upload cycle
- Small files created with `nano` inside the mounted bucket may encounter decryption issues in some cases
- Key derivation uses `EVP_BytesToKey` (SHA-256); upgrading to PBKDF2 (`-pbkdf2`) would improve brute-force resistance
## Future Work
 
- Block-level encryption via `curl.cpp` buffer interception for improved performance
- PBKDF2 key derivation to eliminate the OpenSSL deprecation warning
- Multithreaded buffer pipeline (circular queue) to overlap encryption with I/O
## References
 
- [s3fs-fuse on GitHub](https://github.com/s3fs-fuse/s3fs-fuse)
- [OpenSSL EVP API](https://wiki.openssl.org/index.php/EVP_Symmetric_Encryption_and_Decryption)
- [EVP_BytesToKey documentation](https://docs.openssl.org/3.5/man3/EVP_BytesToKey/)

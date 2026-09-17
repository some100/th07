# Contributing

Contributions are welcome. Before anything can be done, `reccmp` must be installed. This is already done for you if you're using `uv`.

First, copy the original game binary `th07.exe` into the resources directory of the repository. This is required so that `reccmp` has some kind of base to compare against.

Then, simply run the command:

```sh
uv run scripts/build.py reccmp --init
```

Now, you can finally start diffing. After each (re)build, run `uv run reccmp-reccmp --target TH07 --html index.html --nolib` to get a matching summary of all files in the program, and output a webpage showing the diff of every function in the program. Or, alternatively, run `uv run reccmp-reccmp --target TH07 --verbose 0x00FNADDR` on a particular function to diff that function in specific. You'll get a lot of "\[ERROR\] Failed to match xyz" errors in the console. These can be ignored.

For convenience purposes, you can also use `uv run scripts/build.py reccmp` to rebuild and run reccmp at the same time, or `uv run scripts/build.py reccmp 0x00FNADDR` to rebuild and diff a function at the same time.

If you also have `custom.exe`, or the configuration tool bundled in, you can also copy that into the resources directory. After that, reinit `reccmp` with `uv run scripts/build.py reccmp --init`, then run any command with reccmp with `--with-custom`, like `uv run scripts/build.py --with-custom reccmp`. Technically since the program is already 100% matched this would be kind of pointless but it could be helpful for if you make a change to it and want to verify if its still matching.

# Renaming

Most of the functions have poorly named variables that definitely require some renaming. Take for instance, this function in `Pbg4Archive::ReadDecompressEntry`.

```c++
#pragma var_order(pPVar2, dstLen, pbVar5, local_14, dwBytes)
// FUNCTION: TH07 0x0045f960
u8 *Pbg4Archive::ReadDecompressEntry(const char *filename, u8 *buf) {
  SIZE_T dstLen;
  Pbg4Entry *pPVar2;
  SIZE_T dwBytes;
  u8 *pbVar5;
  u8 *local_14;

  local_14 = NULL;
  if (this->fileAbstraction == NULL)
    return NULL;

  pPVar2 = FindEntry(filename);
  if (pPVar2 == NULL)
    goto err;

  if (this->fileAbstraction->Open(this->filename, g_AccessModes[0]) == 0)
    goto err;

  dwBytes = pPVar2[1].dataOffset - pPVar2->dataOffset;
  dstLen = pPVar2->decompressedSize;
  local_14 = (u8 *)GlobalAlloc(0, dwBytes);
  if (local_14 == NULL)
    goto err;

  if (!this->fileAbstraction->Seek(pPVar2->dataOffset, g_SeekModes[0]))
    goto err;
  if (this->fileAbstraction->Read(local_14, dwBytes) == 0)
    goto err;

  pbVar5 = Lzss::Decompress(local_14, dwBytes, buf, dstLen);
  if (local_14) {
    GlobalFree(local_14);
    local_14 = NULL;
  }

  return pbVar5;
err:
  // STRING: TH07 0x004950b8
  utils::DebugPrint("info : %s error\r\n", this->filename);
  if (local_14) {
    GlobalFree(local_14);
    local_14 = NULL;
  }
  return NULL;
}
```

We can spot three unnamed or functionally unnamed variables, `pPVar2`, `pbVar5`, and `local_14`. These names were basically kept as-is from Ghidra, and sadly, they are quite poorly named. To find out what they were probably named, try looking for where they are defined and used.

```c++
Pbg4Entry *pPVar2;
// ...
pPVar2 = FindEntry(filename);
// ...
dstLen = pPVar2->decompressedSize;
// ...
```

Simply based off the type name and how it's being assigned and used, it can be pretty easily renamed as `entry`.

```c++
local_14 = NULL;
// ...
dwBytes = entry[1].dataOffset - entry->dataOffset;
dstLen = entry->decompressedSize;
local_14 = (u8 *)GlobalAlloc(0, dwBytes);
// ...
if (this->fileAbstraction->Read(local_14, dwBytes) == 0)
// ...
pbVar5 = Lzss::Decompress(local_14, dwBytes, buf, dstLen);
```

We can see here that it's allocating a certain amount of bytes, based off the length of the entries, then reading the file into that allocation, then putting it as the `src` argument into `Lzss::Decompress`. Therefore we can conclude that a good name for it would probably be like `src` or `srcBuf`.

```c++
pbVar5 = Lzss::Decompress(srcBuf, dwBytes, buf, dstLen);
```

This one is just dumping the decompressed LZSS output into the variable. Therefore we can rename it to something like `dst`, `decompressed` or `dstBuf`.

The final function comes out to a slightly more reasonable looking:

```c++
#pragma var_order(entry, dstLen, dstBuf, srcBuf, dwBytes)
// FUNCTION: TH07 0x0045f960
u8 *Pbg4Archive::ReadDecompressEntry(const char *filename, u8 *buf) {
  SIZE_T dstLen;
  Pbg4Entry *entry;
  SIZE_T dwBytes;
  u8 *dstBuf;
  u8 *srcBuf;

  srcBuf = NULL;
  if (this->fileAbstraction == NULL)
    return NULL;

  entry = FindEntry(filename);
  if (entry == NULL)
    goto err;

  if (this->fileAbstraction->Open(this->filename, g_AccessModes[0]) == 0)
    goto err;

  dwBytes = entry[1].dataOffset - entry->dataOffset;
  dstLen = entry->decompressedSize;
  srcBuf = (u8 *)GlobalAlloc(0, dwBytes);
  if (srcBuf == NULL)
    goto err;

  if (!this->fileAbstraction->Seek(entry->dataOffset, g_SeekModes[0]))
    goto err;
  if (this->fileAbstraction->Read(srcBuf, dwBytes) == 0)
    goto err;

  dstBuf = Lzss::Decompress(srcBuf, dwBytes, buf, dstLen);
  if (srcBuf) {
    GlobalFree(srcBuf);
    srcBuf = NULL;
  }
  return dstBuf;
err:
  // STRING: TH07 0x004950b8
  utils::DebugPrint("info : %s error\r\n", this->filename);
  if (srcBuf) {
    GlobalFree(srcBuf);
    srcBuf = NULL;
  }
  return NULL;
}
```

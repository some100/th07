# th07

<img src="resources/progress.svg" alt="th07.exe: Implemented: 100%. Accuracy: 99.78%" width="50%">
<img src="resources/custom/progress.svg" alt="custom.exe: Implemented: 100%. Accuracy: 100%" width="50%">

A work-in-progress reimplementation/decompilation of 東方妖々夢　～ Perfect Cherry Blossom 1.00b (md5: 0126afce1e805370d36c3482445e98da) by Team Shanghai Alice.

This repository builds two executables, th07.exe (the main game) and custom.exe (the configuration tool).

The game should be fully playable, since it is 100% implemented. The vast majority of functions are either functionally or completely matched with the original, but there are still a few (mostly due to stack nonsense) that are not currently matching. The behavior of the program should be functionally identical to the original binary, but there may be bugs or differences not present within the original. Perfect functional accuracy is an eventual goal.

This branch is for a matching decompilation only. It will not compile on any platform other than 32-bit Windows with the MSVC 2002 toolchain. For a desktop cross-platform port of the game, you can use the [portable branch](https://github.com/some100/th07/tree/portable) instead. Also, see the [reallyportable branch](https://github.com/some100/th07/tree/reallyportable) for an even more portable version (including iOS/Android support, macOS support, and web builds).

## Building

This project requires the original th07.exe 1.00b executable for extracting the icon. Copy it to the resources directory of the repository. If building custom.exe, you will also need to place the original executable in resources/custom.exe to extract icons as well.

### Dependencies

- uv
- ninja
- wine (Linux only)
    - Note: extracting the MSVC msi is completely broken on older versions of wine. If you face an issue with extracting, try using the latest devel version of wine.

Run the python script in the root directory of the repo with uv:

```
uv run scripts/build.py
```

The resulting build can be found at `build/th07.exe`.

This executable _will_ crash after some time (specifically after 3999 Supervisor cycles). It'll fail the integrity check due to the executable not (yet?) being completely byte accurate (including checksum) to the original. In that case, you can try building a nonmatching build instead, which will disable this integrity check:

```
uv run scripts/build.py --no-matching
```

If you don't have the original executable, you can still build the program without the icon.

```
uv run scripts/build.py --no-icon
```

You can also build the custom.exe configuration tool that comes with the original game.

```
uv run scripts/build.py --with-custom
```

This will build both PCB and its configuration tool.

## Todo

- Improve accuracy and documentation
- Get a better build system than whatever this is

## Contributing

See the [CONTRIBUTING.md](./CONTRIBUTING.md).

## Credits

- The earlier [decompilation for th06](https://github.com/GensokyoClub/th06), used as a source of shared types, function names, file names, source organization, basically everything. Because EoSD and PCB are so similar architecturally, the pre-existing th06 decompilation could be used as a direct reference for reverse engineering th07.

- The [decompilation for th08](https://github.com/GensokyoClub/th08) for:
    - ZunMemory class
    - Entirety of LZSS
    - Float3 and Float2
    - A ton of other stuff that would have been basically impossible to determine in th07 thanks to inlining

- EstexNT for porting the [var_order pragma](https://gist.github.com/EstexNT/e98a1384b906a3eedaaa3eeb7e58cd9d) to MSVC 7, which is used extensively throughout this project.

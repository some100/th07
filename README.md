# th07

A work-in-progress reimplementation/decompilation of 東方妖々夢　～ Perfect Cherry Blossom 1.00b (md5: 0126afce1e805370d36c3482445e98da) by Team Shanghai Alice.

This project is still in early stages. It's playable-ish, and every system appears to work fine, but the original game replays are not fully compatible, and there are a few bugs and problems. Perfect byte accuracy is an eventual goal.

## Building

This project requires the original th07.exe 1.00b executable for extracting the icon. Copy it to the root directory of the repository.

### Dependencies

* uv
* msitools (for msiextract) (Linux only)
* wine (Linux only)

Run the python script in the root directory of the repo with uv:

```
uv run build.py
```

The resulting build can be found at `build/th07.exe`.

If you don't have the original executable, you can still build the program without the icon.

```
uv run build.py --no-icon
```

Note that this build script was not tested on Windows.

## Todo

* Fix the replay desync issue.
* Clean up this complete mess of code.
* Start matching.

## Contributing

See the [CONTRIBUTING.md](./CONTRIBUTING.md).

## Credits

* The earlier [decompilation for th06](https://github.com/GensokyoClub), used as a source of shared types, function names, file names, source organization, basically everything. Because EoSD and PCB are so similar architecturally, the pre-existing th06 decompilation could be used as a direct reference for reverse engineering th07.

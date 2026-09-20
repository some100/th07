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

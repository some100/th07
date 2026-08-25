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

# Matching

Matching functions can range from being nearly trivial to extremely difficult, but `reccmp` makes this process slightly easier.

Let's take `EclManager::Unload`, for example:

```c++
// FUNCTION: TH07 0x0040e4f0
void EclManager::Unload() {
  if (this->eclFile != NULL) {
    free(this->eclFile);
  }
  this->eclFile = NULL;
}
```

This function is still mostly unprocessed Ghidra decompiler output. While it looks reasonable, it may have different output than the original assembly. Call `uv run scripts/build.py reccmp 0x0040e4f0` to get a detailed diff. Note that the original executable is the "older" version in the diff, so it'll be marked with `-`.

```
---
+++
@@ -0x40e4f0,15 +0x40e270,17 @@
0x40e4f0 : push ebp     (EclManager.cpp:78)
0x40e4f1 : mov ebp, esp
0x40e4f3 : -sub esp, 8
0x40e4f6 : -mov dword ptr [ebp - 8], ecx
0x40e4f9 : -mov eax, dword ptr [ebp - 8]
         : +push ecx
         : +mov dword ptr [ebp - 4], ecx
         : +mov eax, dword ptr [ebp - 4]        (EclManager.cpp:79)
0x40e4fc : cmp dword ptr [eax], 0
0x40e4ff : -je 0x14
0x40e501 : -mov ecx, dword ptr [ebp - 8]
         : +je 0xe
         : +mov ecx, dword ptr [ebp - 4]        (EclManager.cpp:81)
0x40e504 : mov edx, dword ptr [ecx]
0x40e506 : -mov dword ptr [ebp - 4], edx
0x40e509 : -mov eax, dword ptr [ebp - 4]
0x40e50c : -push eax
         : +push edx
0x40e50d : call free (FUNCTION)
0x40e512 : add esp, 4
0x40e515 : -mov ecx, dword ptr [ebp - 8]
         : +mov eax, dword ptr [ebp - 4]        (EclManager.cpp:83)
         : +mov dword ptr [eax], 0
         : +mov esp, ebp        (EclManager.cpp:84)
         : +pop ebp
         : +ret 


EclManager::Unload is only 37.50% similar to the original, diff above
```

Immediately from the assembly diff, you can determine a few things just from this section:

```
0x40e4f3 : -sub esp, 8
0x40e4f6 : -mov dword ptr [ebp - 8], ecx
0x40e4f9 : -mov eax, dword ptr [ebp - 8]
         : +push ecx
         : +mov dword ptr [ebp - 4], ecx
         : +mov eax, dword ptr [ebp - 4] 	(EclManager.cpp:67)
```

* Firstly, PCB was compiled with debug settings (for most files), so you can see that the frame pointer here was not omitted. This means we can determine the amount of stack "space" we need. In this case, the very first instruction `sub esp, 8` means we should be subtracting 8 bytes from the frame pointer, `esp`. That means we need to have 8 bytes on the stack.

* Secondly, since this is a _member function_ of EclManager, `this` is located at `ecx`, which is then immediately moved to the end of the stack at `[ebp - 8]` in the original binary. Thus, since `this` already occupies a stack "slot," it means that we are only missing 4 bytes of stack space from our version. Just to make sure, though, we can use a tool that comes with `reccmp`, called `stackcmp`. Call `uv run scripts/build.py stackcmp 0x0040e4f0` to get:

```
[ERROR] Structural mismatch at orig=0x40e506:
-mov dword ptr [ebp - 4], edx
-mov eax, dword ptr [ebp - 4]
-push eax
+push edx

[ERROR] Structural mismatch at orig=0x40e515:
-mov ecx, dword ptr [ebp - 8]
+mov eax, dword ptr [ebp - 4]   (EclManager.cpp:83)
+mov dword ptr [eax], 0
+mov esp, ebp   (EclManager.cpp:84)
+pop ebp
+ret 


Ordered by original stack (left=orig, right=recomp):
⇄  ebp - 0x08: ebp - 0x04  this

Ordered by recomp stack (left=orig, right=recomp):
⇄  ebp - 0x08: ebp - 0x04  this

Legend:
⇄ : This stack variable matches 1:1, but the order of variables is not correct.
✗ : This stack variable matches multiple variables in the other binary.
? : This stack variable did not appear in the diff. It either matches or only appears in structural mismatches.

WARNING: Original and recomp have at least one structural discrepancy, so the comparison of stack variables might be incomplete. The structural mismatches above need to be checked manually.
```

This confirms that we are missing a variable, and more importantly, we are using `this` in place of where it should be. Also, note that the second structural mismatch isn't really a structural mismatch, it'll resolve itself once the actual structural mismatch is fixed. So we have to see where exactly `[ebp - 4]` is used in the first structural mismatch, to determine its type and use.

```
0x40e506 : -mov dword ptr [ebp - 4], edx
0x40e509 : -mov eax, dword ptr [ebp - 4]
0x40e50c : -push eax
         : +push edx
0x40e50d : call free (FUNCTION)
```

Looking at the assembly, it's clear what the actual issue here is. Our new version simply pushes edx (which, prior to this part, stored `this` located at the recompiled binary's `[ebp - 4]`) to the `free` function. However, the original binary moves `edx` into `[ebp - 4]` first, before pushing that variable to `free`.

```c++
free(this->eclFile);
```

So, judging from this line from our recompiled binary, we can pretty safely conclude that the variable at `[ebp - 4]` likely stored `this->eclFile`. If you can recall, we were missing 4 bytes of stack space. This is the missing variable that was present in the original binary. So simply define the variable as such, to get our new function:

```c++
// FUNCTION: TH07 0x0040e4f0
void EclManager::Unload() {
  if (this->eclFile) {
    EclRawHeader *file = this->eclFile;
    free(file);
  }
  this->eclFile = NULL;
}
```

Now, rerun the diff command from earlier to get this result:

```
0x40e4f0: EclManager::Unload 100% match.

✨ OK! ✨
```

Congrats, you've matched a function! 

There's also an alternative way to match this function. For at least this compiler in particular, MSVC 2002, inline functions create compiler temporaries, which can be used for matching. These compiler temporaries are placed _below_ user-defined stack variables on the stack, (so, for example, if I had two stack variables at `[ebp - 4]` and `[ebp - 8]`, and a simple getter inline function, it would move its result to `[ebp - 0xc]`), but _above_ the stack variable for the `this` pointer (if there is any). This is particularly necessary for if stack variables are mismatched, and `stackcmp` indicates that a user-defined variable and temporary variable (the unnamed ones) are swapped.

Generally speaking if it seems like the compiler unnecessarily spilt a variable right before it was used (instead of just using that variable directly), as in this case:

```c++
EclRawHeader *file = this->eclFile;
free(file);
```

Then it probably was using an inline function. One common instance of inline functions
being used is in malloc/free calls. These can be found in ZunMemory.hpp.

This means that you can also have this

```c++
// FUNCTION: TH07 0x0040e4f0
void EclManager::Unload()
{
    if (this->eclFile)
    {
        ZunMemory::Free(this->eclFile);
    }
    this->eclFile = NULL;
}
```

Which has basically the same result to reccmp, and probably matches the original program better.

```
0x4547b0: AnmManager::LoadSurface 100% match.

✨ OK! ✨
```

Ternary operators create their temporaries _below_ the `this` pointer. If the `this` pointer is located at, for example, `[ebp - 0xc]`, then a temporary that stores the result of the operation will be located at `[ebp - 0x10]`. If there is more stack space than the offset of the `this` pointer variable indicates, then that usually indicates that the function uses at least one ternary operator.

Let's look at another example, this time `AnmManager::LoadSurface`.

```c++
// FUNCTION: TH07 0x004547b0
ZunResult AnmManager::LoadSurface(i32 surfaceIdx, const char *path) {
  IDirect3DSurface8 *surface;

  if (this->surfaces[surfaceIdx] != NULL) {
    ReleaseSurface(surfaceIdx);
  }
  u8 *data = FileSystem::OpenFile(path, 0);
  if (data == NULL) {
    // STRING: TH07 0x00495b30
    g_GameErrorContext.Fatal("%sが読み込めないです。\r\n", path);
    return ZUN_ERROR;
  } else {
    if (g_Supervisor.d3dDevice->CreateImageSurface(
            640, 1024, g_Supervisor.presentParameters.BackBufferFormat,
            &surface) == 0) {
      if (((D3DXLoadSurfaceFromFileInMemory(
                surface, NULL, NULL, data, g_LastFileSize, NULL, 1, 0,
                (D3DXIMAGE_INFO *)&this->surfaceSourceInfo[surfaceIdx]) == 0) &&
           (((g_Supervisor.d3dDevice->CreateRenderTarget(
                  this->surfaceSourceInfo[surfaceIdx].width,
                  this->surfaceSourceInfo[surfaceIdx].height,
                  g_Supervisor.presentParameters.BackBufferFormat,
                  D3DMULTISAMPLE_NONE, 1, this->surfaces + surfaceIdx) == 0 ||
              (g_Supervisor.d3dDevice->CreateImageSurface(
                   this->surfaceSourceInfo[surfaceIdx].width,
                   this->surfaceSourceInfo[surfaceIdx].height,
                   g_Supervisor.presentParameters.BackBufferFormat,
                   this->surfaces + surfaceIdx) == 0)) &&
             (g_Supervisor.d3dDevice->CreateImageSurface(
                  this->surfaceSourceInfo[surfaceIdx].width,
                  this->surfaceSourceInfo[surfaceIdx].height,
                  g_Supervisor.presentParameters.BackBufferFormat,
                  this->surfacesBis + surfaceIdx) == 0)))) &&
          ((D3DXLoadSurfaceFromSurface(this->surfaces[surfaceIdx], 0, NULL,
                                       surface, 0, NULL, 1, 0) == 0 &&
            (D3DXLoadSurfaceFromSurface(this->surfacesBis[surfaceIdx], 0, NULL,
                                        surface, 0, NULL, 1, 0) == 0)))) {
        SAFE_RELEASE(surface);
        free(data);
        return ZUN_SUCCESS;
      } else {
        SAFE_RELEASE(surface);
        free(data);
        return ZUN_ERROR;
      }
    } else {
      return ZUN_ERROR;
    }
  }
}
```

```
---
+++
@@ -0x4547df,56 +0x4555ef,53 @@
0x4547df : mov dword ptr [ebp - 8], eax
0x4547e2 : cmp dword ptr [ebp - 8], 0   (AnmManager.cpp:2472)
0x4547e6 : jne 0x1e
0x4547e8 : mov eax, dword ptr [ebp + 0xc]       (AnmManager.cpp:2475)
0x4547eb : push eax
0x4547ec : push "%s\u304c\u8aad\u307f\u8fbc\u3081\u306a\u3044\u3067\u3059\u3002\r\n" (STRING)
0x4547f1 : push g_GameErrorContext (DATA)
0x4547f6 : call GameErrorContext::Fatal (FUNCTION)
0x4547fb : add esp, 0xc
0x4547fe : or eax, 0xffffffff   (AnmManager.cpp:2476)
0x454801 : -jmp 0x1fb
         : +jmp 0x1f4
0x454806 : lea ecx, [ebp - 4]   (AnmManager.cpp:2482)
0x454809 : push ecx
0x45480a : mov edx, dword ptr [g_Supervisor+232 (OFFSET)]
0x454810 : push edx
0x454811 : push 0x400
0x454816 : push 0x280
0x45481b : mov eax, dword ptr [g_Supervisor+8 (OFFSET)]
0x454820 : mov ecx, dword ptr [eax]
0x454822 : mov edx, dword ptr [g_Supervisor+8 (OFFSET)]
0x454828 : push edx
0x454829 : call dword ptr [ecx + 0x6c]
0x45482c : test eax, eax
0x45482e : -je 0x8
0x454830 : -or eax, 0xffffffff
0x454833 : -jmp 0x1c9
         : +jne 0x1c3
0x454838 : mov eax, dword ptr [ebp + 8]         (AnmManager.cpp:2505)
0x45483b : imul eax, eax, 0x14
0x45483e : mov ecx, dword ptr [ebp - 0xc]
0x454841 : lea edx, [ecx + eax + 0x2e248]
0x454848 : push edx
0x454849 : push 0
0x45484b : push 1
0x45484d : push 0
0x45484f : mov eax, dword ptr [g_LastFileSize (DATA)]
0x454854 : push eax
0x454855 : mov ecx, dword ptr [ebp - 8]
0x454858 : push ecx
0x454859 : push 0
0x45485b : push 0
0x45485d : mov edx, dword ptr [ebp - 4]
0x454860 : push edx
0x454861 : call _D3DXLoadSurfaceFromFileInMemory@36 (FUNCTION)
0x454866 : test eax, eax
0x454868 : -je 0x5
0x45486a : -jmp 0x16a
         : +jne 0x163
0x45486f : mov eax, dword ptr [ebp + 8]
0x454872 : mov ecx, dword ptr [ebp - 0xc]
0x454875 : lea edx, [ecx + eax*4 + 0x2e148]
0x45487c : push edx
0x45487d : push 1
0x45487f : push 0
0x454881 : mov eax, dword ptr [g_Supervisor+232 (OFFSET)]
0x454886 : push eax
0x454887 : mov ecx, dword ptr [ebp + 8]
0x45488a : imul ecx, ecx, 0x14

---
+++
@@ -0x45489b,21 +0x4556a6,21 @@
0x45489b : imul ecx, ecx, 0x14
0x45489e : mov edx, dword ptr [ebp - 0xc]
0x4548a1 : mov eax, dword ptr [edx + ecx + 0x2e248]
0x4548a8 : push eax
0x4548a9 : mov ecx, dword ptr [g_Supervisor+8 (OFFSET)]
0x4548af : mov edx, dword ptr [ecx]
0x4548b1 : mov eax, dword ptr [g_Supervisor+8 (OFFSET)]
0x4548b6 : push eax
0x4548b7 : call dword ptr [edx + 0x64]
0x4548ba : test eax, eax
0x4548bc : -je 0x52
         : +je 0x51
0x4548be : mov ecx, dword ptr [ebp + 8]
0x4548c1 : mov edx, dword ptr [ebp - 0xc]
0x4548c4 : lea eax, [edx + ecx*4 + 0x2e148]
0x4548cb : push eax
0x4548cc : mov ecx, dword ptr [g_Supervisor+232 (OFFSET)]
0x4548d2 : push ecx
0x4548d3 : mov edx, dword ptr [ebp + 8]
0x4548d6 : imul edx, edx, 0x14
0x4548d9 : mov eax, dword ptr [ebp - 0xc]
0x4548dc : mov ecx, dword ptr [eax + edx + 0x2e24c]

---
+++
@@ -0x4548e7,22 +0x4556f2,21 @@
0x4548e7 : imul edx, edx, 0x14
0x4548ea : mov eax, dword ptr [ebp - 0xc]
0x4548ed : mov ecx, dword ptr [eax + edx + 0x2e248]
0x4548f4 : push ecx
0x4548f5 : mov edx, dword ptr [g_Supervisor+8 (OFFSET)]
0x4548fb : mov eax, dword ptr [edx]
0x4548fd : mov ecx, dword ptr [g_Supervisor+8 (OFFSET)]
0x454903 : push ecx
0x454904 : call dword ptr [eax + 0x6c]
0x454907 : test eax, eax
0x454909 : -je 0x5
0x45490b : -jmp 0xc9
         : +jne 0xc3
0x454910 : mov edx, dword ptr [ebp + 8]
0x454913 : mov eax, dword ptr [ebp - 0xc]
0x454916 : lea ecx, [eax + edx*4 + 0x2e1c8]
0x45491d : push ecx
0x45491e : mov edx, dword ptr [g_Supervisor+232 (OFFSET)]
0x454924 : push edx
0x454925 : mov eax, dword ptr [ebp + 8]
0x454928 : imul eax, eax, 0x14
0x45492b : mov ecx, dword ptr [ebp - 0xc]
0x45492e : mov edx, dword ptr [ecx + eax + 0x2e24c]

---
+++
@@ -0x454939,70 +0x455743,73 @@
0x454939 : imul eax, eax, 0x14
0x45493c : mov ecx, dword ptr [ebp - 0xc]
0x45493f : mov edx, dword ptr [ecx + eax + 0x2e248]
0x454946 : push edx
0x454947 : mov eax, dword ptr [g_Supervisor+8 (OFFSET)]
0x45494c : mov ecx, dword ptr [eax]
0x45494e : mov edx, dword ptr [g_Supervisor+8 (OFFSET)]
0x454954 : push edx
0x454955 : call dword ptr [ecx + 0x6c]
0x454958 : test eax, eax
0x45495a : -je 0x2
0x45495c : -jmp 0x7b
         : +jne 0x77
0x45495e : push 0
0x454960 : push 1
0x454962 : push 0
0x454964 : push 0
0x454966 : mov eax, dword ptr [ebp - 4]
0x454969 : push eax
0x45496a : push 0
0x45496c : push 0
0x45496e : mov ecx, dword ptr [ebp + 8]
0x454971 : mov edx, dword ptr [ebp - 0xc]
0x454974 : mov eax, dword ptr [edx + ecx*4 + 0x2e148]
0x45497b : push eax
0x45497c : call _D3DXLoadSurfaceFromSurface@32 (FUNCTION)
0x454981 : test eax, eax
0x454983 : -je 0x2
0x454985 : -jmp 0x52
         : +jne 0x50
0x454987 : push 0
0x454989 : push 1
0x45498b : push 0
0x45498d : push 0
0x45498f : mov ecx, dword ptr [ebp - 4]
0x454992 : push ecx
0x454993 : push 0
0x454995 : push 0
0x454997 : mov edx, dword ptr [ebp + 8]
0x45499a : mov eax, dword ptr [ebp - 0xc]
0x45499d : mov ecx, dword ptr [eax + edx*4 + 0x2e1c8]
0x4549a4 : push ecx
0x4549a5 : call _D3DXLoadSurfaceFromSurface@32 (FUNCTION)
0x4549aa : test eax, eax
0x4549ac : -je 0x2
0x4549ae : -jmp 0x29
         : +jne 0x29
0x4549b0 : cmp dword ptr [ebp - 4], 0   (AnmManager.cpp:2507)
0x4549b4 : je 0x13
0x4549b6 : mov edx, dword ptr [ebp - 4]
0x4549b9 : mov eax, dword ptr [edx]
0x4549bb : mov ecx, dword ptr [ebp - 4]
0x4549be : push ecx
0x4549bf : call dword ptr [eax + 8]
0x4549c2 : mov dword ptr [ebp - 4], 0
0x4549c9 : mov edx, dword ptr [ebp - 8]         (AnmManager.cpp:2508)
0x4549cc : push edx
0x4549cd : call free (FUNCTION)
0x4549d2 : add esp, 4
0x4549d5 : xor eax, eax         (AnmManager.cpp:2509)
0x4549d7 : -jmp 0x28
         : +jmp 0x2d
0x4549d9 : cmp dword ptr [ebp - 4], 0   (AnmManager.cpp:2513)
0x4549dd : je 0x13
0x4549df : mov eax, dword ptr [ebp - 4]
0x4549e2 : mov ecx, dword ptr [eax]
0x4549e4 : mov edx, dword ptr [ebp - 4]
0x4549e7 : push edx
0x4549e8 : call dword ptr [ecx + 8]
0x4549eb : mov dword ptr [ebp - 4], 0
0x4549f2 : mov eax, dword ptr [ebp - 8]         (AnmManager.cpp:2514)
0x4549f5 : push eax
0x4549f6 : call free (FUNCTION)
0x4549fb : add esp, 4
         : +or eax, 0xffffffff  (AnmManager.cpp:2515)
         : +jmp 0x3
         : +or eax, 0xffffffff  (AnmManager.cpp:2520)
         : +mov esp, ebp        (AnmManager.cpp:2523)
         : +pop ebp
         : +ret 8


AnmManager::LoadSurface is only 91.91% similar to the original, diff above
```

This is a function that's already very close to matching, and there's a few things going on here. But we only need to care about a few things:

```
0x45481b : mov eax, dword ptr [g_Supervisor+8 (OFFSET)]
0x454820 : mov ecx, dword ptr [eax]
0x454822 : mov edx, dword ptr [g_Supervisor+8 (OFFSET)]
0x454828 : push edx
0x454829 : call dword ptr [ecx + 0x6c]
0x45482c : test eax, eax
0x45482e : -je 0x8
0x454830 : -or eax, 0xffffffff
0x454833 : -jmp 0x1c9
         : +jne 0x1c3
```

Firstly it's important to note that non-void functions usually return their result in `eax`. Secondly, `je` means to skip if the if statement is NOT true, so the condition should be the opposite of the jump conditional following it.

This assembly indicates that we should be calling a member function of `g_Supervisor+8` in `ecx`, checking if `eax` (the result) is zero, and, if it is NOT zero, we OR the result with `0xffffffff` (which is `-1` from unsigned to signed) and `jmp` to the end of the function. So it's doing something similar to this:

```c++
if (g_Supervisor.something->func() != 0)
{
  return -1;
}
```

We also see this kind of pattern all around:

```
0x4549a5 : call _D3DXLoadSurfaceFromSurface@32 (FUNCTION)
0x4549aa : test eax, eax
0x4549ac : -je 0x2
0x4549ae : -jmp 0x29
         : +jne 0x29
```

This means that instead of comparing a `D3DXLoadSurfaceFromSurface()` to zero, and early returning (or using a goto) if it is NOT zero, we are instead doing a nested if-statement as indicated by the `jne`.

Going back to C++:

```c++
if (((D3DXLoadSurfaceFromFileInMemory(
          surface, NULL, NULL, data, g_LastFileSize, NULL, 1, 0,
          (D3DXIMAGE_INFO *)&this->surfaceSourceInfo[surfaceIdx]) == 0) &&
(((g_Supervisor.d3dDevice->CreateRenderTarget(
       this->surfaceSourceInfo[surfaceIdx].width,
       this->surfaceSourceInfo[surfaceIdx].height,
       g_Supervisor.presentParameters.BackBufferFormat,
       D3DMULTISAMPLE_NONE, 1, this->surfaces + surfaceIdx) == 0 ||
   (g_Supervisor.d3dDevice->CreateImageSurface(
        this->surfaceSourceInfo[surfaceIdx].width,
        this->surfaceSourceInfo[surfaceIdx].height,
        g_Supervisor.presentParameters.BackBufferFormat,
        this->surfaces + surfaceIdx) == 0)) &&
  (g_Supervisor.d3dDevice->CreateImageSurface(
       this->surfaceSourceInfo[surfaceIdx].width,
       this->surfaceSourceInfo[surfaceIdx].height,
       g_Supervisor.presentParameters.BackBufferFormat,
       this->surfacesBis + surfaceIdx) == 0)))) &&
((D3DXLoadSurfaceFromSurface(this->surfaces[surfaceIdx], 0, NULL,
                            surface, 0, NULL, 1, 0) == 0 &&
 (D3DXLoadSurfaceFromSurface(this->surfacesBis[surfaceIdx], 0, NULL,
                             surface, 0, NULL, 1, 0) == 0))))
```

No real human person writes like this, so it's very likely that there's some kind of reason why all these early return if-statements have the exact same error path. The else branch of this extremely convoluted if-statement is:

```c++
} else {
  SAFE_RELEASE(surface);
  free(data);
  return ZUN_ERROR;
}
```

This is probably meant to be a goto label. It's very likely that Ghidra, the decompiler used to dump all this code, decided to merge together if-statements that had the same error path as though they were one if-else statement. This is supported by the earlier finding that the if-statements should really be using early returns/gotos, not nested if-statements. Rewrite it to use a goto:

```c++
if (D3DXLoadSurfaceFromFileInMemory(
          surface, NULL, NULL, data, g_LastFileSize, NULL, 1, 0,
          (D3DXIMAGE_INFO *)&this->surfaceSourceInfo[surfaceIdx]))
    goto err;

err:
  SAFE_RELEASE(surface);
  free(data);
  return ZUN_ERROR;
```

After applying this same pattern to all of the conditions, you get this much more reasonable looking:

```c++
// FUNCTION: TH07 0x004547b0
ZunResult AnmManager::LoadSurface(i32 surfaceIdx, const char *path) {
  IDirect3DSurface8 *surface;

  if (this->surfaces[surfaceIdx]) {
    ReleaseSurface(surfaceIdx);
  }
  u8 *data = FileSystem::OpenFile(path, 0);
  if (data == NULL) {
    // STRING: TH07 0x00495b30
    g_GameErrorContext.Fatal("%sが読み込めないです。\r\n", path);
    return ZUN_ERROR;
  }
  if (g_Supervisor.d3dDevice->CreateImageSurface(
          640, 1024, g_Supervisor.presentParameters.BackBufferFormat,
          &surface) != 0)
    return ZUN_ERROR;

  if (D3DXLoadSurfaceFromFileInMemory(
          surface, NULL, NULL, data, g_LastFileSize, NULL, 1, 0,
          (D3DXIMAGE_INFO *)&this->surfaceSourceInfo[surfaceIdx])
    goto err;

  if (g_Supervisor.d3dDevice->CreateRenderTarget(
          this->surfaceSourceInfo[surfaceIdx].width,
          this->surfaceSourceInfo[surfaceIdx].height,
          g_Supervisor.presentParameters.BackBufferFormat, D3DMULTISAMPLE_NONE,
          1, this->surfaces + surfaceIdx) != 0)
    if (g_Supervisor.d3dDevice->CreateImageSurface(
          this->surfaceSourceInfo[surfaceIdx].width,
          this->surfaceSourceInfo[surfaceIdx].height,
          g_Supervisor.presentParameters.BackBufferFormat,
          this->surfaces + surfaceIdx) != 0)
    goto err;

  if (g_Supervisor.d3dDevice->CreateImageSurface(
          this->surfaceSourceInfo[surfaceIdx].width,
          this->surfaceSourceInfo[surfaceIdx].height,
          g_Supervisor.presentParameters.BackBufferFormat,
          this->surfacesBis + surfaceIdx) != 0)
    goto err;

  if (D3DXLoadSurfaceFromSurface(this->surfaces[surfaceIdx], 0, NULL, surface,
                                 0, NULL, 1, 0) != 0)
    goto err;
  if ((D3DXLoadSurfaceFromSurface(this->surfacesBis[surfaceIdx], 0, NULL,
                                  surface, 0, NULL, 1, 0) != 0))
    goto err;

  SAFE_RELEASE(surface);
  free(data);
  return ZUN_SUCCESS;

err:
  SAFE_RELEASE(surface);
  free(data);
  return ZUN_ERROR;
}
```

Now, after rerunning the diff command:

```
0x4547b0: AnmManager::LoadSurface 100% match.

✨ OK! ✨
```

This function is now 100% matching.

## Using the var_order pragma

Sometimes you'll have functions like these:

```c++
// FUNCTION: TH07 0x00409990
void BombData::BombReimuADrawFocus(Player *player)
{
    i32 i;
    AnmVm *vm;

    DarkenViewport(player);
    for (i = 0; i < 8; i++)
    {
        if (player->bombInfo.subInfo[i].state == 0)
        {
            continue;
        }

        vm = player->bombInfo.subInfo[i].vms;
        vm->pos = player->bombInfo.subInfo[i].pos + vm->offset;
        player->SetToTopLeftPos(vm);
        g_AnmManager->DrawNoRotation(vm);
        vm++;
        vm->pos =
            player->bombInfo.subInfo[i].pos + vm->offset;
        player->SetToTopLeftPos(vm);
        g_AnmManager->DrawNoRotation(vm);
        vm++;
        vm->pos =
            player->bombInfo.subInfo[i].pos + vm->offset;
        player->SetToTopLeftPos(vm);
        g_AnmManager->DrawNoRotation(vm);
        vm++;
        vm->pos =
            player->bombInfo.subInfo[i].pos + vm->offset;
        player->SetToTopLeftPos(vm);
        g_AnmManager->DrawNoRotation(vm);
        vm++;
    }
}
```
```
---
+++
@@ -0x409990,38 +0x409710,38 @@
0x409990 : push ebp 	(BombData.cpp:479)
0x409991 : mov ebp, esp
0x409993 : sub esp, 0xac
0x409999 : mov dword ptr [ebp - 0x8c], ecx
0x40999f : mov ecx, dword ptr [ebp - 0x8c] 	(BombData.cpp:483)
0x4099a5 : call BombData::DarkenViewport (FUNCTION)
0x4099aa : -mov dword ptr [ebp - 8], 0
         : +mov dword ptr [ebp - 4], 0 	(BombData.cpp:484)
0x4099b1 : jmp 0x9
0x4099b3 : -mov eax, dword ptr [ebp - 8]
         : +mov eax, dword ptr [ebp - 4]
0x4099b6 : add eax, 1
0x4099b9 : -mov dword ptr [ebp - 8], eax
0x4099bc : -cmp dword ptr [ebp - 8], 8
         : +mov dword ptr [ebp - 4], eax
         : +cmp dword ptr [ebp - 4], 8
0x4099c0 : jge 0x3fd
0x4099c6 : -mov ecx, dword ptr [ebp - 8]
         : +mov ecx, dword ptr [ebp - 4] 	(BombData.cpp:486)
0x4099c9 : imul ecx, ecx, 0x1428
0x4099cf : mov edx, dword ptr [ebp - 0x8c]
0x4099d5 : cmp dword ptr [edx + ecx + 0x16a4c], 0
0x4099dd : jne 0x2
0x4099df : jmp -0x2e 	(BombData.cpp:488)
0x4099e1 : -mov eax, dword ptr [ebp - 8]
         : +mov eax, dword ptr [ebp - 4] 	(BombData.cpp:491)
0x4099e4 : imul eax, eax, 0x1428
0x4099ea : mov ecx, dword ptr [ebp - 0x8c]
0x4099f0 : lea edx, [ecx + eax + 0x16c04]
0x4099f7 : -mov dword ptr [ebp - 4], edx
0x4099fa : -mov eax, dword ptr [ebp - 4]
         : +mov dword ptr [ebp - 8], edx
         : +mov eax, dword ptr [ebp - 8] 	(BombData.cpp:492)
0x4099fd : add eax, 0x230
0x409a02 : mov dword ptr [ebp - 0x48], eax
0x409a05 : -mov ecx, dword ptr [ebp - 8]
         : +mov ecx, dword ptr [ebp - 4]
0x409a08 : imul ecx, ecx, 0x1428
0x409a0e : mov edx, dword ptr [ebp - 0x8c]
0x409a14 : lea eax, [edx + ecx + 0x16a60]
0x409a1b : mov dword ptr [ebp - 0x4c], eax
0x409a1e : mov ecx, dword ptr [ebp - 0x4c]
0x409a21 : mov edx, dword ptr [ebp - 0x48]
0x409a24 : fld dword ptr [ecx + 8]
0x409a27 : fadd dword ptr [edx + 8]
0x409a2a : fstp dword ptr [ebp - 0x3c]
0x409a2d : mov eax, dword ptr [ebp - 0x4c]

---
+++
@@ -0x409a3f,57 +0x4097bf,57 @@
0x409a3f : mov eax, dword ptr [ebp - 0x48]
0x409a42 : fld dword ptr [edx]
0x409a44 : fadd dword ptr [eax]
0x409a46 : fstp dword ptr [ebp - 0x44]
0x409a49 : mov ecx, dword ptr [ebp - 0x44]
0x409a4c : mov dword ptr [ebp - 0x14], ecx
0x409a4f : mov edx, dword ptr [ebp - 0x40]
0x409a52 : mov dword ptr [ebp - 0x10], edx
0x409a55 : mov eax, dword ptr [ebp - 0x3c]
0x409a58 : mov dword ptr [ebp - 0xc], eax
0x409a5b : -mov ecx, dword ptr [ebp - 4]
         : +mov ecx, dword ptr [ebp - 8]
0x409a5e : add ecx, 0x1c8
0x409a64 : mov edx, dword ptr [ebp - 0x14]
0x409a67 : mov dword ptr [ecx], edx
0x409a69 : mov eax, dword ptr [ebp - 0x10]
0x409a6c : mov dword ptr [ecx + 4], eax
0x409a6f : mov edx, dword ptr [ebp - 0xc]
0x409a72 : mov dword ptr [ecx + 8], edx
0x409a75 : -mov eax, dword ptr [ebp - 4]
         : +mov eax, dword ptr [ebp - 8] 	(BombData.cpp:493)
0x409a78 : add eax, 0x1c8
0x409a7d : mov dword ptr [ebp - 0x90], eax
0x409a83 : mov ecx, dword ptr [ebp - 0x90]
0x409a89 : fld dword ptr [g_GameManager+38388 (OFFSET)]
0x409a8f : fadd dword ptr [ecx]
0x409a91 : mov edx, dword ptr [ebp - 0x90]
0x409a97 : fstp dword ptr [edx]
0x409a99 : -mov eax, dword ptr [ebp - 4]
         : +mov eax, dword ptr [ebp - 8]
0x409a9c : add eax, 0x1cc
0x409aa1 : mov dword ptr [ebp - 0x94], eax
0x409aa7 : mov ecx, dword ptr [ebp - 0x94]
0x409aad : fld dword ptr [g_GameManager+38392 (OFFSET)]
0x409ab3 : fadd dword ptr [ecx]
0x409ab5 : mov edx, dword ptr [ebp - 0x94]
0x409abb : fstp dword ptr [edx]
0x409abd : -mov eax, dword ptr [ebp - 4]
         : +mov eax, dword ptr [ebp - 8]
0x409ac0 : mov dword ptr [eax + 0x1d0], 0
0x409aca : -mov ecx, dword ptr [ebp - 4]
         : +mov ecx, dword ptr [ebp - 8] 	(BombData.cpp:494)
0x409acd : push ecx
0x409ace : mov ecx, dword ptr [g_AnmManager (DATA)]
0x409ad4 : call AnmManager::DrawNoRotation (FUNCTION)
0x409ad9 : -mov edx, dword ptr [ebp - 4]
         : +mov edx, dword ptr [ebp - 8] 	(BombData.cpp:495)
0x409adc : add edx, 0x24c
0x409ae2 : -mov dword ptr [ebp - 4], edx
0x409ae5 : -mov eax, dword ptr [ebp - 4]
         : +mov dword ptr [ebp - 8], edx
         : +mov eax, dword ptr [ebp - 8] 	(BombData.cpp:497)
0x409ae8 : add eax, 0x230
0x409aed : mov dword ptr [ebp - 0x5c], eax
0x409af0 : -mov ecx, dword ptr [ebp - 8]
         : +mov ecx, dword ptr [ebp - 4]
0x409af3 : imul ecx, ecx, 0x1428
0x409af9 : mov edx, dword ptr [ebp - 0x8c]
0x409aff : lea eax, [edx + ecx + 0x16a60]
0x409b06 : mov dword ptr [ebp - 0x60], eax
0x409b09 : mov ecx, dword ptr [ebp - 0x60]
0x409b0c : mov edx, dword ptr [ebp - 0x5c]
0x409b0f : fld dword ptr [ecx + 8]
0x409b12 : fadd dword ptr [edx + 8]
0x409b15 : fstp dword ptr [ebp - 0x50]
0x409b18 : mov eax, dword ptr [ebp - 0x60]

---
+++
@@ -0x409b2a,57 +0x4098aa,57 @@
0x409b2a : mov eax, dword ptr [ebp - 0x5c]
0x409b2d : fld dword ptr [edx]
0x409b2f : fadd dword ptr [eax]
0x409b31 : fstp dword ptr [ebp - 0x58]
0x409b34 : mov ecx, dword ptr [ebp - 0x58]
0x409b37 : mov dword ptr [ebp - 0x20], ecx
0x409b3a : mov edx, dword ptr [ebp - 0x54]
0x409b3d : mov dword ptr [ebp - 0x1c], edx
0x409b40 : mov eax, dword ptr [ebp - 0x50]
0x409b43 : mov dword ptr [ebp - 0x18], eax
0x409b46 : -mov ecx, dword ptr [ebp - 4]
         : +mov ecx, dword ptr [ebp - 8]
0x409b49 : add ecx, 0x1c8
0x409b4f : mov edx, dword ptr [ebp - 0x20]
0x409b52 : mov dword ptr [ecx], edx
0x409b54 : mov eax, dword ptr [ebp - 0x1c]
0x409b57 : mov dword ptr [ecx + 4], eax
0x409b5a : mov edx, dword ptr [ebp - 0x18]
0x409b5d : mov dword ptr [ecx + 8], edx
0x409b60 : -mov eax, dword ptr [ebp - 4]
         : +mov eax, dword ptr [ebp - 8] 	(BombData.cpp:498)
0x409b63 : add eax, 0x1c8
0x409b68 : mov dword ptr [ebp - 0x98], eax
0x409b6e : mov ecx, dword ptr [ebp - 0x98]
0x409b74 : fld dword ptr [g_GameManager+38388 (OFFSET)]
0x409b7a : fadd dword ptr [ecx]
0x409b7c : mov edx, dword ptr [ebp - 0x98]
0x409b82 : fstp dword ptr [edx]
0x409b84 : -mov eax, dword ptr [ebp - 4]
         : +mov eax, dword ptr [ebp - 8]
0x409b87 : add eax, 0x1cc
0x409b8c : mov dword ptr [ebp - 0x9c], eax
0x409b92 : mov ecx, dword ptr [ebp - 0x9c]
0x409b98 : fld dword ptr [g_GameManager+38392 (OFFSET)]
0x409b9e : fadd dword ptr [ecx]
0x409ba0 : mov edx, dword ptr [ebp - 0x9c]
0x409ba6 : fstp dword ptr [edx]
0x409ba8 : -mov eax, dword ptr [ebp - 4]
         : +mov eax, dword ptr [ebp - 8]
0x409bab : mov dword ptr [eax + 0x1d0], 0
0x409bb5 : -mov ecx, dword ptr [ebp - 4]
         : +mov ecx, dword ptr [ebp - 8] 	(BombData.cpp:499)
0x409bb8 : push ecx
0x409bb9 : mov ecx, dword ptr [g_AnmManager (DATA)]
0x409bbf : call AnmManager::DrawNoRotation (FUNCTION)
0x409bc4 : -mov edx, dword ptr [ebp - 4]
         : +mov edx, dword ptr [ebp - 8] 	(BombData.cpp:500)
0x409bc7 : add edx, 0x24c
0x409bcd : -mov dword ptr [ebp - 4], edx
0x409bd0 : -mov eax, dword ptr [ebp - 4]
         : +mov dword ptr [ebp - 8], edx
         : +mov eax, dword ptr [ebp - 8] 	(BombData.cpp:502)
0x409bd3 : add eax, 0x230
0x409bd8 : mov dword ptr [ebp - 0x70], eax
0x409bdb : -mov ecx, dword ptr [ebp - 8]
         : +mov ecx, dword ptr [ebp - 4]
0x409bde : imul ecx, ecx, 0x1428
0x409be4 : mov edx, dword ptr [ebp - 0x8c]
0x409bea : lea eax, [edx + ecx + 0x16a60]
0x409bf1 : mov dword ptr [ebp - 0x74], eax
0x409bf4 : mov ecx, dword ptr [ebp - 0x74]
0x409bf7 : mov edx, dword ptr [ebp - 0x70]
0x409bfa : fld dword ptr [ecx + 8]
0x409bfd : fadd dword ptr [edx + 8]
0x409c00 : fstp dword ptr [ebp - 0x64]
0x409c03 : mov eax, dword ptr [ebp - 0x74]

---
+++
@@ -0x409c15,57 +0x409995,57 @@
0x409c15 : mov eax, dword ptr [ebp - 0x70]
0x409c18 : fld dword ptr [edx]
0x409c1a : fadd dword ptr [eax]
0x409c1c : fstp dword ptr [ebp - 0x6c]
0x409c1f : mov ecx, dword ptr [ebp - 0x6c]
0x409c22 : mov dword ptr [ebp - 0x2c], ecx
0x409c25 : mov edx, dword ptr [ebp - 0x68]
0x409c28 : mov dword ptr [ebp - 0x28], edx
0x409c2b : mov eax, dword ptr [ebp - 0x64]
0x409c2e : mov dword ptr [ebp - 0x24], eax
0x409c31 : -mov ecx, dword ptr [ebp - 4]
         : +mov ecx, dword ptr [ebp - 8]
0x409c34 : add ecx, 0x1c8
0x409c3a : mov edx, dword ptr [ebp - 0x2c]
0x409c3d : mov dword ptr [ecx], edx
0x409c3f : mov eax, dword ptr [ebp - 0x28]
0x409c42 : mov dword ptr [ecx + 4], eax
0x409c45 : mov edx, dword ptr [ebp - 0x24]
0x409c48 : mov dword ptr [ecx + 8], edx
0x409c4b : -mov eax, dword ptr [ebp - 4]
         : +mov eax, dword ptr [ebp - 8] 	(BombData.cpp:503)
0x409c4e : add eax, 0x1c8
0x409c53 : mov dword ptr [ebp - 0xa0], eax
0x409c59 : mov ecx, dword ptr [ebp - 0xa0]
0x409c5f : fld dword ptr [g_GameManager+38388 (OFFSET)]
0x409c65 : fadd dword ptr [ecx]
0x409c67 : mov edx, dword ptr [ebp - 0xa0]
0x409c6d : fstp dword ptr [edx]
0x409c6f : -mov eax, dword ptr [ebp - 4]
         : +mov eax, dword ptr [ebp - 8]
0x409c72 : add eax, 0x1cc
0x409c77 : mov dword ptr [ebp - 0xa4], eax
0x409c7d : mov ecx, dword ptr [ebp - 0xa4]
0x409c83 : fld dword ptr [g_GameManager+38392 (OFFSET)]
0x409c89 : fadd dword ptr [ecx]
0x409c8b : mov edx, dword ptr [ebp - 0xa4]
0x409c91 : fstp dword ptr [edx]
0x409c93 : -mov eax, dword ptr [ebp - 4]
         : +mov eax, dword ptr [ebp - 8]
0x409c96 : mov dword ptr [eax + 0x1d0], 0
0x409ca0 : -mov ecx, dword ptr [ebp - 4]
         : +mov ecx, dword ptr [ebp - 8] 	(BombData.cpp:504)
0x409ca3 : push ecx
0x409ca4 : mov ecx, dword ptr [g_AnmManager (DATA)]
0x409caa : call AnmManager::DrawNoRotation (FUNCTION)
0x409caf : -mov edx, dword ptr [ebp - 4]
         : +mov edx, dword ptr [ebp - 8] 	(BombData.cpp:505)
0x409cb2 : add edx, 0x24c
0x409cb8 : -mov dword ptr [ebp - 4], edx
0x409cbb : -mov eax, dword ptr [ebp - 4]
         : +mov dword ptr [ebp - 8], edx
         : +mov eax, dword ptr [ebp - 8] 	(BombData.cpp:507)
0x409cbe : add eax, 0x230
0x409cc3 : mov dword ptr [ebp - 0x84], eax
0x409cc9 : -mov ecx, dword ptr [ebp - 8]
         : +mov ecx, dword ptr [ebp - 4]
0x409ccc : imul ecx, ecx, 0x1428
0x409cd2 : mov edx, dword ptr [ebp - 0x8c]
0x409cd8 : lea eax, [edx + ecx + 0x16a60]
0x409cdf : mov dword ptr [ebp - 0x88], eax
0x409ce5 : mov ecx, dword ptr [ebp - 0x88]
0x409ceb : mov edx, dword ptr [ebp - 0x84]
0x409cf1 : fld dword ptr [ecx + 8]
0x409cf4 : fadd dword ptr [edx + 8]
0x409cf7 : fstp dword ptr [ebp - 0x78]
0x409cfa : mov eax, dword ptr [ebp - 0x88]

---
+++
@@ -0x409d15,47 +0x409a95,47 @@
0x409d15 : mov eax, dword ptr [ebp - 0x84]
0x409d1b : fld dword ptr [edx]
0x409d1d : fadd dword ptr [eax]
0x409d1f : fstp dword ptr [ebp - 0x80]
0x409d22 : mov ecx, dword ptr [ebp - 0x80]
0x409d25 : mov dword ptr [ebp - 0x38], ecx
0x409d28 : mov edx, dword ptr [ebp - 0x7c]
0x409d2b : mov dword ptr [ebp - 0x34], edx
0x409d2e : mov eax, dword ptr [ebp - 0x78]
0x409d31 : mov dword ptr [ebp - 0x30], eax
0x409d34 : -mov ecx, dword ptr [ebp - 4]
         : +mov ecx, dword ptr [ebp - 8]
0x409d37 : add ecx, 0x1c8
0x409d3d : mov edx, dword ptr [ebp - 0x38]
0x409d40 : mov dword ptr [ecx], edx
0x409d42 : mov eax, dword ptr [ebp - 0x34]
0x409d45 : mov dword ptr [ecx + 4], eax
0x409d48 : mov edx, dword ptr [ebp - 0x30]
0x409d4b : mov dword ptr [ecx + 8], edx
0x409d4e : -mov eax, dword ptr [ebp - 4]
         : +mov eax, dword ptr [ebp - 8] 	(BombData.cpp:508)
0x409d51 : add eax, 0x1c8
0x409d56 : mov dword ptr [ebp - 0xa8], eax
0x409d5c : mov ecx, dword ptr [ebp - 0xa8]
0x409d62 : fld dword ptr [g_GameManager+38388 (OFFSET)]
0x409d68 : fadd dword ptr [ecx]
0x409d6a : mov edx, dword ptr [ebp - 0xa8]
0x409d70 : fstp dword ptr [edx]
0x409d72 : -mov eax, dword ptr [ebp - 4]
         : +mov eax, dword ptr [ebp - 8]
0x409d75 : add eax, 0x1cc
0x409d7a : mov dword ptr [ebp - 0xac], eax
0x409d80 : mov ecx, dword ptr [ebp - 0xac]
0x409d86 : fld dword ptr [g_GameManager+38392 (OFFSET)]
0x409d8c : fadd dword ptr [ecx]
0x409d8e : mov edx, dword ptr [ebp - 0xac]
0x409d94 : fstp dword ptr [edx]
0x409d96 : -mov eax, dword ptr [ebp - 4]
         : +mov eax, dword ptr [ebp - 8]
0x409d99 : mov dword ptr [eax + 0x1d0], 0
0x409da3 : -mov ecx, dword ptr [ebp - 4]
         : +mov ecx, dword ptr [ebp - 8] 	(BombData.cpp:509)
0x409da6 : push ecx
0x409da7 : mov ecx, dword ptr [g_AnmManager (DATA)]
0x409dad : call AnmManager::DrawNoRotation (FUNCTION)
0x409db2 : -mov edx, dword ptr [ebp - 4]
         : +mov edx, dword ptr [ebp - 8] 	(BombData.cpp:510)
0x409db5 : add edx, 0x24c
0x409dbb : -mov dword ptr [ebp - 4], edx
         : +mov dword ptr [ebp - 8], edx
0x409dbe : jmp -0x410 	(BombData.cpp:511)
0x409dc3 : mov esp, ebp 	(BombData.cpp:512)
0x409dc5 : pop ebp
0x409dc6 : ret


BombData::BombReimuADrawFocus is only 84.42% similar to the original, diff above
```

The diff appears large, and it may look as though the function isn't very close to matching. However, if you more closely inspect the diff:

```
0x409dbb : -mov dword ptr [ebp - 4], edx
         : +mov dword ptr [ebp - 8], edx
```

The real issue here is that the stack slots are wrongly ordered. The variable that should be in `[ebp - 4]` is located in `[ebp - 8]`, and vice versa. In other words, we need to swap the positions of these variables as they appear in the stack. Just to confirm though, the `reccmp` suite already has exactly the tool to verify this, `stackcmp`:

```
Ordered by original stack (left=orig, right=recomp):
✓  ebp - 0xac: ebp - 0xac
✓  ebp - 0xa8: ebp - 0xa8
✓  ebp - 0xa4: ebp - 0xa4
✓  ebp - 0xa0: ebp - 0xa0
✓  ebp - 0x9c: ebp - 0x9c
✓  ebp - 0x98: ebp - 0x98
✓  ebp - 0x94: ebp - 0x94
✓  ebp - 0x90: ebp - 0x90
✓  ebp - 0x8c: ebp - 0x8c  player
✓  ebp - 0x88: ebp - 0x88
✓  ebp - 0x84: ebp - 0x84
✓  ebp - 0x80: ebp - 0x80
✓  ebp - 0x7c: ebp - 0x7c
✓  ebp - 0x78: ebp - 0x78
✓  ebp - 0x74: ebp - 0x74
✓  ebp - 0x70: ebp - 0x70
✓  ebp - 0x6c: ebp - 0x6c
✓  ebp - 0x68: ebp - 0x68
✓  ebp - 0x64: ebp - 0x64
✓  ebp - 0x60: ebp - 0x60
✓  ebp - 0x5c: ebp - 0x5c
✓  ebp - 0x58: ebp - 0x58
✓  ebp - 0x54: ebp - 0x54
✓  ebp - 0x50: ebp - 0x50
✓  ebp - 0x4c: ebp - 0x4c
✓  ebp - 0x48: ebp - 0x48
✓  ebp - 0x44: ebp - 0x44
✓  ebp - 0x40: ebp - 0x40
✓  ebp - 0x3c: ebp - 0x3c
✓  ebp - 0x38: ebp - 0x38
✓  ebp - 0x34: ebp - 0x34
✓  ebp - 0x30: ebp - 0x30
✓  ebp - 0x2c: ebp - 0x2c
✓  ebp - 0x28: ebp - 0x28
✓  ebp - 0x24: ebp - 0x24
✓  ebp - 0x20: ebp - 0x20
✓  ebp - 0x1c: ebp - 0x1c
✓  ebp - 0x18: ebp - 0x18
✓  ebp - 0x14: ebp - 0x14
✓  ebp - 0x10: ebp - 0x10
✓  ebp - 0x0c: ebp - 0x0c
⇄  ebp - 0x08: ebp - 0x04  i
⇄  ebp - 0x04: ebp - 0x08  vm

Ordered by recomp stack (left=orig, right=recomp):
✓  ebp - 0xac: ebp - 0xac
✓  ebp - 0xa8: ebp - 0xa8
✓  ebp - 0xa4: ebp - 0xa4
✓  ebp - 0xa0: ebp - 0xa0
✓  ebp - 0x9c: ebp - 0x9c
✓  ebp - 0x98: ebp - 0x98
✓  ebp - 0x94: ebp - 0x94
✓  ebp - 0x90: ebp - 0x90
✓  ebp - 0x8c: ebp - 0x8c  player
✓  ebp - 0x88: ebp - 0x88
✓  ebp - 0x84: ebp - 0x84
✓  ebp - 0x80: ebp - 0x80
✓  ebp - 0x7c: ebp - 0x7c
✓  ebp - 0x78: ebp - 0x78
✓  ebp - 0x74: ebp - 0x74
✓  ebp - 0x70: ebp - 0x70
✓  ebp - 0x6c: ebp - 0x6c
✓  ebp - 0x68: ebp - 0x68
✓  ebp - 0x64: ebp - 0x64
✓  ebp - 0x60: ebp - 0x60
✓  ebp - 0x5c: ebp - 0x5c
✓  ebp - 0x58: ebp - 0x58
✓  ebp - 0x54: ebp - 0x54
✓  ebp - 0x50: ebp - 0x50
✓  ebp - 0x4c: ebp - 0x4c
✓  ebp - 0x48: ebp - 0x48
✓  ebp - 0x44: ebp - 0x44
✓  ebp - 0x40: ebp - 0x40
✓  ebp - 0x3c: ebp - 0x3c
✓  ebp - 0x38: ebp - 0x38
✓  ebp - 0x34: ebp - 0x34
✓  ebp - 0x30: ebp - 0x30
✓  ebp - 0x2c: ebp - 0x2c
✓  ebp - 0x28: ebp - 0x28
✓  ebp - 0x24: ebp - 0x24
✓  ebp - 0x20: ebp - 0x20
✓  ebp - 0x1c: ebp - 0x1c
✓  ebp - 0x18: ebp - 0x18
✓  ebp - 0x14: ebp - 0x14
✓  ebp - 0x10: ebp - 0x10
✓  ebp - 0x0c: ebp - 0x0c
⇄  ebp - 0x04: ebp - 0x08  vm
⇄  ebp - 0x08: ebp - 0x04  i

Legend:
⇄ : This stack variable matches 1:1, but the order of variables is not correct.
✗ : This stack variable matches multiple variables in the other binary.
? : This stack variable did not appear in the diff. It either matches or only appears in structural mismatches.
```

The output of `stackcmp` may be misleading if there are severe structural mismatches, so you can't rely on it too early. In this case however, there are no structural mismatches.

Inside of `stackcmp`, at least in this situation, you can basically ignore the bottom half that says "Ordered by recomp stack" and focus on the top half. This tells us the order that these variables need to be in, in order to match the original.

For whatever reason it seems like variables in MSVC 2002 are sorted by their hashed names, making it quite unreliable to force a match this way. It is also possible to simply break the variables into various block scopes, which may be necessary in some cases though it is (in my opinion) super clunky and ugly.

Instead of playing around with the variables, the preferred way to force a match is to use the `var_order` pragma that comes with this repo. Simply put, just add `#pragma var_order(foo, bar)` in the order recommended by `stackcmp`. Applied to this case:

```c++
#pragma var_order(vm, i)
// FUNCTION: TH07 0x00409990
void BombData::BombReimuADrawFocus(Player *player)
{
    i32 i;
    AnmVm *vm;
    // ...
}
```

Make sure that the pragma is above the reccmp annotation and that the variables you declared are ordered in stack layout order _starting from the bottom_, as such: 
```c++
i32 i;
AnmVm *vm;
```
This is not for any reason in particular, it's just for consistency and to align with the stackcmp output.

Now, if you run reccmp again on the function:

```
0x409990: BombData::BombReimuADrawFocus 100% match.

✨ OK!
```

It'll appear 100% matching.

The `var_order` pragma is not only usable atop functions, it can be used in any scope where a variable is declared inside. For example:

```c++
#pragma var_order(fovDiff, t)
if (arg->timersMax[camIdx] != 0)
{
    f32 t;
    f32 fovDiff;

    if (arg->timers[camIdx] < arg->timersMax[camIdx])
    {
        arg->timers[camIdx]++;
        t = arg->timers[camIdx].AsFloat() /
                        (f32)arg->timersMax[camIdx];
    }
    else
    {
        arg->timers[camIdx] = arg->timersMax[camIdx];
        t = 1.0f;
        arg->timersMax[camIdx] = 0;
    }
    switch (arg->easeModes[camIdx])
    {
    case 1:
        t = 1.0f - t;
        t = 1.0f - t * t;
        break;
    case 2:
        t = 1.0f - t;
        t = 1.0f - t * t * t;
        break;
    case 3:
        t = 1.0f - t;
        t = 1.0f - t * t * t * t;
        break;
    case 4:
        t = t * t;
        break;
    case 5:
        t = t * t * t;
        break;
    case 6:
        t = t * t * t * t;
    }
    fovDiff = arg->camEnd.fov - arg->camStart.fov;
    arg->cam.fov = fovDiff * t + arg->camStart.fov;
}
```

Doing this will reorder the variables inside of that scope.

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
  DebugPrint("info : %s error\r\n", this->filename);
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
  DebugPrint("info : %s error\r\n", this->filename);
  if (srcBuf) {
    GlobalFree(srcBuf);
    srcBuf = NULL;
  }
  return NULL;
}
```

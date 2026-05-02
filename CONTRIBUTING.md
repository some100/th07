# Contributing

Contributions are welcome. Before anything can be done, `reccmp` must be installed from git. This is already done for you if you're using `uv`. 

First, copy the original game binary `th07.exe` into the root of the repository. This is required so that `reccmp` has some kind of base to compare against.

Then, simply run the command:
```sh
uv run build.py reccmp --init
```

Now, you can finally start diffing. After each (re)build, run `uv run reccmp-reccmp --target TH07 --html index.html --nolib` to get a matching summary of all files in the program, and output a webpage showing the diff of every function in the program. Or, alternatively, run `uv run reccmp-reccmp --target TH07 --verbose 0x00FNADDR` on a particular function to diff that function in specific. You'll get a lot of "\[ERROR\] Failed to match xyz" errors in the console. These can be ignored.

For convenience purposes, you can also use `uv run build.py reccmp` to rebuild and run reccmp at the same time, or `uv run build.py reccmp 0x00FNADDR` to rebuild and diff a function at the same time.

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

This function is still mostly unprocessed Ghidra decompiler output. While it looks reasonable, it may have different output than the original assembly. Call `uv run build.py reccmp 0x0040e4f0` to get a detailed diff.

```
---
+++
@@ -0x40e4f0,15 +0x40d490,17 @@
0x40e4f0 : push ebp 	(EclManager.cpp:66)
0x40e4f1 : mov ebp, esp
0x40e4f3 : -sub esp, 8
0x40e4f6 : -mov dword ptr [ebp - 8], ecx
0x40e4f9 : -mov eax, dword ptr [ebp - 8]
         : +push ecx
         : +mov dword ptr [ebp - 4], ecx
         : +mov eax, dword ptr [ebp - 4] 	(EclManager.cpp:67)
0x40e4fc : cmp dword ptr [eax], 0
0x40e4ff : -je 0x14
0x40e501 : -mov ecx, dword ptr [ebp - 8]
         : +je 0xe
         : +mov ecx, dword ptr [ebp - 4] 	(EclManager.cpp:68)
0x40e504 : mov edx, dword ptr [ecx]
0x40e506 : -mov dword ptr [ebp - 4], edx
         : +push edx
         : +call free (FUNCTION)
         : +add esp, 4
0x40e509 : mov eax, dword ptr [ebp - 4] 	(EclManager.cpp:70)
0x40e50c : -push eax
0x40e50d : -call <OFFSET1>
0x40e512 : -add esp, 4
0x40e515 : -mov ecx, dword ptr [ebp - 8]
         : +mov dword ptr [eax], 0
         : +mov esp, ebp 	(EclManager.cpp:71)
         : +pop ebp
         : +ret
```

Immediately from the assembly diff, you can determine a few things just from this diff:

```
0x40e4f3 : -sub esp, 8
0x40e4f6 : -mov dword ptr [ebp - 8], ecx
0x40e4f9 : -mov eax, dword ptr [ebp - 8]
         : +push ecx
         : +mov dword ptr [ebp - 4], ecx
         : +mov eax, dword ptr [ebp - 4] 	(EclManager.cpp:67)
```

Firstly, PCB was compiled with debug settings, so you can see that the frame pointer here was not omitted. Secondly, it shows that 8 bytes were allocated on the stack, evidenced by `sub esp, 8`, which was not allocated in the recompiled version. However, since this is a member function of EclManager, `this` is located at `ecx`, which is then immediately moved to the end of the stack at `[ebp - 8]` in the original binary, or `[ebp - 4]`. Thus, since `this` already occupies a stack "slot," it means that we are only missing 4 bytes of stack space from our version. Just to make sure, though, we can use a tool that comes with `reccmp`, called `stackcmp`. Call `reccmp-stackcmp --target TH07 0x0040e4f0` to get:

```
[ERROR] Structural mismatch at orig=0x40e506:
-mov dword ptr [ebp - 4], edx
+push edx
+call free (FUNCTION)
+add esp, 4

[ERROR] Mismatching line structure at orig=0x40e515:
-push eax
-call <OFFSET1>
-add esp, 4
-mov ecx, dword ptr [ebp - 8]
+mov dword ptr [eax], 0
+mov esp, ebp 	(EclManager.cpp:77)
+pop ebp
+ret


Ordered by original stack (left=orig, right=recomp):
&#8644;  ebp - 0x08: ebp - 0x04  this
&#10003;  ebp - 0x04: ebp - 0x04  this

Ordered by recomp stack (left=orig, right=recomp):
&#10007;  ['ebp - 0x08', 'ebp - 0x04']: ebp - 0x04  this

Legend:
&#8644; : This stack variable matches 1:1, but the order of variables is not correct.
&#10007; : This stack variable matches multiple variables in the other binary.
? : This stack variable did not appear in the diff. It either matches or only appears in structural mismatches.
```

This confirms that we are missing an a variable, and more importantly, we are using `this` in place of where it should be. So we have to see where exactly `[ebp - 4]` is used or assigned to in the _original_ version in the diff, to determine its type and use.

```
0x40e506 : -mov dword ptr [ebp - 4], edx
         : +push edx
         : +call free (FUNCTION)
         : +add esp, 4
```

This corresponds to the `free` call in our function. It essentially pushes an argument, which is stored in edx, into the stack, which is then used by the free function afterwards. Recall beforehand in our C++ function that this was

```c++
free(this->eclFile);
```

Therefore, at this moment in the program, edx is storing `this->eclFile`. In the original version in the diff, we see the call `mov dword ptr [ebp - 4], edx`. The `mov` call essentially moves the right hand operand to the left hand operand, meaning that it assigns `edx`, which is storing `this->eclFile`, into `[ebp - 4]`, which is a variable on the stack. If you can recall, we were missing 4 bytes of stack space. This is the missing variable that was present in the original binary. So simply define the variable as such, to get our new function:

```c++
// FUNCTION: TH07 0x0040e4f0
void EclManager::Unload() {
  if (this->eclFile != NULL) {
    EclRawHeader *file = this->eclFile;
    free(file);
  }
  this->eclFile = NULL;
}
```

Now, rerun the diff command from earlier to get this result:

```
0x40e4f0: EclManager::Unload 100% match.

OK! 
```

Congrats, you've matched a function!

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
    g_GameErrorContext.Fatal("%s‚ª“Ç‚Ýž‚ß‚È‚¢‚Å‚·B\r\n", path);
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
@@ -0x4547df,56 +0x453cbf,53 @@
0x4547df : mov dword ptr [ebp - 8], eax
0x4547e2 : cmp dword ptr [ebp - 8], 0 	(AnmManager.cpp:2489)
0x4547e6 : jne 0x1e
0x4547e8 : mov eax, dword ptr [ebp + 0xc] 	(AnmManager.cpp:2491)
0x4547eb : push eax
0x4547ec : push "%s\u304c\u8aad\u307f\u8fbc\u3081\u306a\u3044\u3067\u3059\u3002\r\n" (STRING)
0x4547f1 : push g_GameErrorContext (DATA)
0x4547f6 : call GameErrorContext::Fatal (FUNCTION)
0x4547fb : add esp, 0xc
0x4547fe : or eax, 0xffffffff 	(AnmManager.cpp:2492)
0x454801 : -jmp 0x1fb
         : +jmp 0x1f4
0x454806 : lea ecx, [ebp - 4] 	(AnmManager.cpp:2496)
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
0x454838 : mov eax, dword ptr [ebp + 8] 	(AnmManager.cpp:2518)
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
0x454861 : -call <OFFSET9>
         : +call _D3DXLoadSurfaceFromFileInMemory@36 (FUNCTION)
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
@@ -0x45489b,21 +0x453d76,21 @@
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
@@ -0x4548e7,22 +0x453dc2,21 @@
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
@@ -0x454939,70 +0x453e13,73 @@
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
0x45497c : -call <OFFSET10>
         : +call _D3DXLoadSurfaceFromSurface@32 (FUNCTION)
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
0x4549a5 : -call <OFFSET10>
         : +call _D3DXLoadSurfaceFromSurface@32 (FUNCTION)
0x4549aa : test eax, eax
0x4549ac : -je 0x2
0x4549ae : -jmp 0x29
         : +jne 0x29
0x4549b0 : cmp dword ptr [ebp - 4], 0 	(AnmManager.cpp:2519)
0x4549b4 : je 0x13
0x4549b6 : mov edx, dword ptr [ebp - 4]
0x4549b9 : mov eax, dword ptr [edx]
0x4549bb : mov ecx, dword ptr [ebp - 4]
0x4549be : push ecx
0x4549bf : call dword ptr [eax + 8]
0x4549c2 : mov dword ptr [ebp - 4], 0
0x4549c9 : mov edx, dword ptr [ebp - 8] 	(AnmManager.cpp:2520)
0x4549cc : push edx
0x4549cd : -call <OFFSET11>
         : +call free (FUNCTION)
0x4549d2 : add esp, 4
0x4549d5 : xor eax, eax 	(AnmManager.cpp:2521)
0x4549d7 : -jmp 0x28
         : +jmp 0x2d
0x4549d9 : cmp dword ptr [ebp - 4], 0 	(AnmManager.cpp:2523)
0x4549dd : je 0x13
0x4549df : mov eax, dword ptr [ebp - 4]
0x4549e2 : mov ecx, dword ptr [eax]
0x4549e4 : mov edx, dword ptr [ebp - 4]
0x4549e7 : push edx
0x4549e8 : call dword ptr [ecx + 8]
0x4549eb : mov dword ptr [ebp - 4], 0
0x4549f2 : mov eax, dword ptr [ebp - 8] 	(AnmManager.cpp:2524)
0x4549f5 : push eax
0x4549f6 : -call <OFFSET11>
         : +call free (FUNCTION)
0x4549fb : add esp, 4
         : +or eax, 0xffffffff 	(AnmManager.cpp:2525)
         : +jmp 0x3
         : +or eax, 0xffffffff 	(AnmManager.cpp:2528)
         : +mov esp, ebp 	(AnmManager.cpp:2531)
         : +pop ebp
         : +ret 8
```

A lot of things going on here, but we only need to care about a few things:

```
0x454861 : -call <OFFSET9>
         : +call _D3DXLoadSurfaceFromFileInMemory@36 (FUNCTION)
0x454866 : test eax, eax
0x454868 : -je 0x5
0x45486a : -jmp 0x16a
         : +jne 0x163
```

When seeing something like `test eax, eax` (or alternatively `cmp xyz, a`), followed by a `je` type instruction, that usually indicates an if statement. `je` basically means "if equal, jump relative to the current offset." In these cases, it's emitted to skip over an if-statement is the condition is not true. Afterwards is an unconditional `jmp` call, indicating an early return, which can be either a goto or a return. However, our version instead chooses to do `jne`, which is meant to skip to the else statement. Immediately before the `je` instruction is this:

```
0x454861 : -call <OFFSET9>
         : +call _D3DXLoadSurfaceFromFileInMemory@36 (FUNCTION)
0x454866 : test eax, eax
```

Functions return their result in the `eax` register, and `test eax, eax` simply checks if the result is equal to zero. Therefore, the condition in C++ code should be if the result is NOT zero. So going back to C++:

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

This is probably meant to be a goto label. It's very likely that Ghidra, the decompiler used to dump all this code, decided to merge together if-statements that had the same error path as though they were one if-else statement. Rewrite it to use a goto:

```c++
if (D3DXLoadSurfaceFromFileInMemory(
          surface, NULL, NULL, data, g_LastFileSize, NULL, 1, 0,
          (D3DXIMAGE_INFO *)&this->surfaceSourceInfo[surfaceIdx]) != 0)
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

  if (this->surfaces[surfaceIdx] != NULL) {
    ReleaseSurface(surfaceIdx);
  }
  u8 *data = FileSystem::OpenFile(path, 0);
  if (data == NULL) {
    // STRING: TH07 0x00495b30
    g_GameErrorContext.Fatal("%s‚ª“Ç‚Ýž‚ß‚È‚¢‚Å‚·B\r\n", path);
    return ZUN_ERROR;
  }
  if (g_Supervisor.d3dDevice->CreateImageSurface(
          640, 1024, g_Supervisor.presentParameters.BackBufferFormat,
          &surface) != 0)
    return ZUN_ERROR;

  if (D3DXLoadSurfaceFromFileInMemory(
          surface, NULL, NULL, data, g_LastFileSize, NULL, 1, 0,
          (D3DXIMAGE_INFO *)&this->surfaceSourceInfo[surfaceIdx]) != 0)
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

OK!
```

This function is now 100% matching.

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
  if (local_14 != NULL) {
    GlobalFree(local_14);
    local_14 = NULL;
  }

  return pbVar5;
err:
  // STRING: TH07 0x004950b8
  DebugPrint("info : %s error\r\n", this->filename);
  if (local_14 != NULL) {
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
  if (srcBuf != NULL) {
    GlobalFree(srcBuf);
    srcBuf = NULL;
  }
  return dstBuf;
err:
  // STRING: TH07 0x004950b8
  DebugPrint("info : %s error\r\n", this->filename);
  if (srcBuf != NULL) {
    GlobalFree(srcBuf);
    srcBuf = NULL;
  }
  return NULL;
}
```

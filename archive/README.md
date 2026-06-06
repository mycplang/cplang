# Archive

Historical source files preserved for reference. These are NOT compiled or linked
by the current build system.

- `vm_switch.cpp` — Original switch-dispatch VM implementation (1521 lines).
  Replaced by the threaded-code VM in `src/vm/vm_exec.cpp`.
  Builds with `#error` unless `-DCPLANG_ALLOW_VM_SWITCH` is explicitly set.

- `vm_opt_stub.cpp` — Former empty stub for MSVC builds (3 lines).
  Replaced by a documented stub at `src/vm/vm_opt_stub.cpp`.

- `main_minimal.cpp` — Earlier minimal CLI entry point (66 lines).
  Uses old APIs (e.g., `lexer.tokenize()`) that no longer exist.

- `all_stubs.cpp` — Obsolete combined stub file for LLVM/JIT/AOT symbols.
  Replaced by the individual stub files:
  - `src/jit/hybrid_jit_stub.cpp` (HybridJIT)
  - `src/codegen/aot_stub.cpp` (AOTCompiler)

# Web boss background call signature

The Web linker reported PortSetBgWorld returning i32 in the boss caller but void
in the PPU implementation. The caller did not include the shared declaration in
engine.h; legacy gnu89 accepted implicit int with warnings suppressed. A successful
link therefore did not establish a usable call. LLD documents that its default
signature mismatch handling creates an unreachable stub and warns:
https://lld.llvm.org/WebAssembly.html#function-signatures

The boss caller now includes that shared header only under PLATFORM_PC. The four
calls, arguments, callback and gameplay logic are unchanged. Web linking also
uses --fatal-warnings, so future linker signature warnings stop delivery rather
than produce a publishable module. Legacy C warnings are not globally fatal.

The focused test extracts the real setter body, shared declaration and four boss
call expressions. A fixture callback verifies registration/origin and all clears.
Using the SDK's canonical clang/wasm-ld directly avoids Emscripten shared-cache
mutation. The implicit declaration links with a warning then traps under Node;
fatal warnings reject it at link time. The declared candidate links without a
warning and executes all four effects, with or without fatal warnings. This is
an ABI regression, not a natural boss run or browser/GPU certification.

The full candidate translation unit also compiles for Windows32 with its real
-mno-ms-bitfields contract and an arm64_32 guest target. Initial private harness
attempts omitted required target/layout or resource include flags; they do not
establish a product failure. Final Web delivery requires all three region links
under the fatal-warning guard and the usual artifact/ABI/frontend gates.

This is a changed C input. Previously closed native artifacts remain compiled
from their recorded source; do not claim native-input equality to this commit.
Their machine-level calls ignore the unused return, unlike Wasm's strict direct
call type. The Web objects for this caller must be rebuilt. A link-only cache
adapter may retain other objects only with independently checked AST/recipe,
header, flag, compiler and symbol equivalence, with explicit provenance.

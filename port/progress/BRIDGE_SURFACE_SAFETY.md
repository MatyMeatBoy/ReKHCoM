# Shared surface/image stage: follow-up to texture safety

The texture safety audit found the preceding surface stage has the same NULL
dereference in conversion, scaled blit and BMP export. The complete-shim
fixture reproduces the conversion access violation before the fix.

Validate dimensions, byte size, row pitch and source pixels before reading or
allocating. Preserve SDL's deferred CreateSurfaceFrom(NULL,0) metadata and
empty surfaces, while rejecting use until nonempty surfaces have pixels.
Use size_t for row offsets, wide products for scaling, and iterate only the
visible destination rectangle. Validate the BMP32-bit file size before writing.

Checks use the entire production shim with allocator failures: NULL/deferred
inputs, both failed surface allocations, copied/converted pixels and BMP
channel/row order, and a tiny visible region of an INT_MAX-sized rectangle.
Keep valid original scaling/blending and GBA logic. This addresses a proven
potential crash/hang pattern, without attributing a reporter's failure to it.

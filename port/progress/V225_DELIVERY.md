# 2.2.5 delivery preparation

The owner selected 2.2.5 instead of 2.3 while reporter confirmation continues.
This source preparation is not a publication receipt. The README's latest link
continues to describe the public 2.2 release until publication is verified.

Included reviewed changes cover font-atlas startup admission, bounded Android
attempt ownership/reports, font allocation/upload failure, Linux keyboard input,
HD movie controls/import coverage, preview fallback, negotiated output controls,
and native64 Pulse client packaging. Per-change tests and limits are documented
in the associated progress files; CPU regression success does not certify a
physical Moto, Samsung, Steam Deck or console.

Delivery requires eight separately hashed packages plus the browser beta,
exact source/component identities and checks of their packaged contents.
Keep Android, Anbernic, consoles and Web beta labels. Keep device reports open
where the evidence only supports a tentative correction or diagnostic test.

Builds share toolchains and immutable inputs. Generated objects, staging copies
and caches are temporary; remove them after closed packages and useful small
receipts are retained. The owner's target is under30MB of generated residue,
excluding retained source, originals, shared SDKs and published releases.

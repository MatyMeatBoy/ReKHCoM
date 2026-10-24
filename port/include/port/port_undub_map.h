/* Address metadata only; audio and sequences are read from the user's JP ROM. */
#if defined(VERSION_EU)
static const UndubSample kUndubSamples[] = {
    { 0x1dde294, 0x1e9d718, 0x1e06cf8, 2390 }, /* the voice group's first instrument, SONG_VO_SR_ATTACK00 (issue #45, check_undub.py) */
    { 0x1dde2a0, 0x1e9e070, 0x1e07650, 2300 },
    { 0x1dde2ac, 0x1e9e970, 0x1e07f50, 2318 },
    { 0x1dde2b8, 0x1e9f280, 0x1e08860, 2839 },
    { 0x1dde2d0, 0x1e9fd98, 0x1e09378, 4357 },
    { 0x1dde2e8, 0x1ea0ea0, 0x1e0a480, 6092 },
    { 0x1dde2f4, 0x1ea2670, 0x1e0bc50, 5192 },
    { 0x1dde300, 0x1ea3abc, 0x1e0d09c, 4238 },
    { 0x1dde318, 0x1ea4b4c, 0x1e0e12c, 1856 },
    { 0x1dde324, 0x1ea5290, 0x1e0e870, 7580 },
    { 0x1dde330, 0x1ea7030, 0x1e10610, 8667 },
    { 0x1dde33c, 0x1ea920c, 0x1e127ec, 3368 },
    { 0x1dde348, 0x1ea9f38, 0x1e13518, 4948 },
    { 0x1dde354, 0x1eab290, 0x1e14870, 7656 },
    { 0x1dde360, 0x1ead07c, 0x1e1665c, 4579 },
    { 0x1dde36c, 0x1eae260, 0x1e17840, 4844 },
    { 0x1dde378, 0x1eaf550, 0x1e18b30, 4753 },
    { 0x1dde384, 0x1eb07e4, 0x1e19dc4, 4538 },
    { 0x1dde390, 0x1eb19a0, 0x1e1af80, 6514 },
    { 0x1dde39c, 0x1eb3314, 0x1e1c8f4, 4495 },
    { 0x1dde3b4, 0x1eb44a4, 0x1e1da84, 4775 },
    { 0x1dde3c0, 0x1eb574c, 0x1e1ed2c, 2765 },
    { 0x1dde3cc, 0x1eb621c, 0x1e1f7fc, 1945 },
    { 0x1dde3d8, 0x1eb69b8, 0x1e1ff98, 2070 },
    { 0x1dde3e4, 0x1eb71d0, 0x1e207b0, 7374 },
    { 0x1dde3f0, 0x1eb8ea0, 0x1e22480, 5219 },
    { 0x1dde3fc, 0x1eba304, 0x1e238e4, 5633 },
    { 0x1dde408, 0x1ebb908, 0x1e24ee8, 5572 },
    { 0x1dde414, 0x1ebced0, 0x1e264b0, 6163 },
    { 0x1dde420, 0x1ebe6e4, 0x1e27cc4, 1726 },
    { 0x1dde42c, 0x1ebeda4, 0x1e28384, 4484 },
    { 0x1dde438, 0x1ebff2c, 0x1e2950c, 6504 },
    { 0x1dde444, 0x1ec1898, 0x1e2ae78, 5100 },
    { 0x1dde450, 0x1ec2c88, 0x1e2c268, 4927 },
    { 0x1dde45c, 0x1ec3fc8, 0x1e2d5a8, 5563 },
    { 0x1dde468, 0x1ec5584, 0x1e2eb64, 9451 },
    { 0x1dde474, 0x1ec7a70, 0x1e31050, 2757 },
    { 0x1dde480, 0x1ec8538, 0x1e31b18, 3526 },
    { 0x1dde48c, 0x1ec9300, 0x1e328e0, 6494 },
    { 0x1dde498, 0x1ecac60, 0x1e34240, 4791 },
    { 0x1dde4a4, 0x1ecbf18, 0x1e354f8, 1331 },
    { 0x1dde4b0, 0x1ecc44c, 0x1e35a2c, 2885 },
    { 0x1dde4bc, 0x1eccf94, 0x1e36574, 1897 },
    { 0x1dde4c8, 0x1ecd700, 0x1e36ce0, 1800 },
    { 0x1dde4d4, 0x1ecde0c, 0x1e373ec, 1944 },
    { 0x1dde4e0, 0x1ece5a8, 0x1e37b88, 2736 },
    { 0x1dde4ec, 0x1ecf05c, 0x1e3863c, 2604 },
    { 0x1dde4f8, 0x1ecfa8c, 0x1e3906c, 4181 },
    { 0x1dde504, 0x1ed0ae4, 0x1e3a0c4, 12591 },
    { 0x1dde510, 0x1ed3c14, 0x1e3d1f4, 4632 },
    { 0x1dde51c, 0x1ed4e30, 0x1e3e410, 5983 },
    { 0x1dde528, 0x1ed6590, 0x1e3fb70, 2760 },
    { 0x1dde534, 0x1ed705c, 0x1e4063c, 7156 },
    { 0x1dde540, 0x1ed8c54, 0x1e42234, 10729 },
    { 0x1dde54c, 0x1edb640, 0x1e44c20, 1785 },
    { 0x1dde558, 0x1edbd3c, 0x1e4531c, 1737 },
    { 0x1dde564, 0x1edc408, 0x1e459e8, 8074 },
    { 0x1dde570, 0x1ede394, 0x1e47974, 2413 },
    { 0x1dde57c, 0x1eded04, 0x1e482e4, 2712 },
    { 0x1dde588, 0x1edf7a0, 0x1e48d80, 4434 },
    { 0x1dde594, 0x1ee08f4, 0x1e49ed4, 6669 },
    { 0x1dde5a0, 0x1ee2304, 0x1e4b8e4, 1692 },
    { 0x1dde5ac, 0x1ee29a4, 0x1e4bf84, 3093 },
    { 0x1dde5b8, 0x1ee35bc, 0x1e4cb9c, 2915 },
    { 0x1dde5c4, 0x1ee4120, 0x1e4d700, 6095 },
    { 0x1dde5d0, 0x1ee58f0, 0x1e4eed0, 3342 },
    { 0x1dde5dc, 0x1ee6600, 0x1e4fbe0, 4802 },
    { 0x1dde5e8, 0x1ee78c4, 0x1e50ea4, 5800 },
    { 0x1dde5f4, 0x1ee8f70, 0x1e52550, 6330 },
    { 0x1dde600, 0x1eea82c, 0x1e53e0c, 8086 },
    { 0x1dde60c, 0x1eec7c4, 0x1e55da4, 12417 },
    { 0x1dde618, 0x1eef848, 0x1e58e28, 5308 },
    { 0x1dde624, 0x1ef0d08, 0x1e5a2e8, 4030 },
    { 0x1dde630, 0x1ef1cc8, 0x1e5b2a8, 5754 },
    { 0x1dde63c, 0x1ef3344, 0x1e5c924, 1764 },
    { 0x1dde648, 0x1ef3a2c, 0x1e5d00c, 9314 },
    { 0x1dde654, 0x1ef5e90, 0x1e5f470, 1226 },
    { 0x1dde660, 0x1ef635c, 0x1e5f93c, 6479 },
    { 0x1dde66c, 0x1ef7cac, 0x1e6128c, 3947 },
    { 0x1dde678, 0x1ef8c18, 0x1e621f8, 3332 },
    { 0x1dde684, 0x1ef9920, 0x1e62f00, 5791 },
    { 0x1dde690, 0x1efafc0, 0x1e645a0, 6223 },
    { 0x1dde69c, 0x1efc810, 0x1e65df0, 6047 },
    { 0x1dde6a8, 0x1efdfb0, 0x1e67590, 4840 },
    { 0x1dde6b4, 0x1eff29c, 0x1e6887c, 4137 },
    { 0x1dde6c0, 0x1f002c8, 0x1e698a8, 3474 },
    { 0x1dde744, 0x1f0105c, 0x0, 16 },
};
static const UndubSong kUndubSongs[] = {
    { 0x1f2e36c, 0x1e9672c, 16, 16 }, /* gSongVoSrAttack00 */
    { 0x1f2e3a4, 0x1e96764, 16, 16 }, /* gSongVoSrAttack02 */
    { 0x1f2e3c0, 0x1e96780, 16, 16 }, /* gSongVoSrAttack03 */
    { 0x1f2e3f8, 0x1e967b8, 16, 16 }, /* gSongVoSrAttack07 */
    { 0x1f2e484, 0x1e96844, 16, 16 }, /* gSongVoSrDeath00 */
    { 0x1f2e4a0, 0x1e96860, 16, 16 }, /* gSongVoSrFire00 */
    { 0x1f2e4bc, 0x1e9687c, 16, 16 }, /* gSongVoSrBurizad00 */
    { 0x1f2e4d8, 0x1e96898, 16, 16 }, /* gSongVoSrThnder00 */
    { 0x1f2e4f4, 0x1e968b4, 16, 16 }, /* gSongVoSrCarel00 */
    { 0x1f2e510, 0x1e968d0, 16, 16 }, /* gSongVoSrGurabi00 */
    { 0x1f2e52c, 0x1e968ec, 16, 16 }, /* gSongVoSrStop00 */
    { 0x1f2e548, 0x1e96908, 16, 16 }, /* gSongVoSrAiro00 */
    { 0x1f2e564, 0x1e96924, 16, 16 }, /* gSongVoSrSummon00 */
    { 0x1f2e580, 0x1e96940, 16, 16 }, /* gSongVoSrSummon01 */
    { 0x1f2e618, 0x1e969d8, 20, 20 }, /* gSongVoGfAttack00 */
    { 0x1f2e638, 0x1e969f8, 20, 20 }, /* gSongVoDlAttack00 */
    { 0x1f2e658, 0x1e96a18, 16, 16 }, /* gSongVoSrSummon04 */
    { 0x1f2e6ac, 0x1e96a6c, 16, 16 }, /* gSongVoSrAttack09 */
    { 0x1f2e6c8, 0x1e96a88, 16, 16 }, /* gSongVoSrSummon07 */
    { 0x1f2e6e4, 0x1e96aa4, 16, 16 }, /* gSongVoSrSummon08 */
    { 0x1f2e700, 0x1e96ac0, 16, 16 }, /* gSongVoSrSummon09 */
    { 0x1f2e754, 0x1e96b14, 20, 20 }, /* gSongVoBeAttack00 */
    { 0x1f2e774, 0x1e96b34, 20, 20 }, /* gSongVoArAttack00 */
    { 0x1f2e7b4, 0x1e96b74, 20, 20 }, /* gSongVoAdAttack00 */
    { 0x1f2e7f4, 0x1e96bb4, 20, 20 }, /* gSongVoJcAttack01 */
    { 0x1f2e874, 0x1e96c34, 16, 16 }, /* gSongVoHoVoice00 */
    { 0x1f2faa8, 0x1e97e68, 16, 16 }, /* gSongVoHaAttack00 */
    { 0x1f2fac4, 0x1e97e84, 16, 16 }, /* gSongVoHaAttack01 */
    { 0x1f2fae0, 0x1e97ea0, 16, 16 }, /* gSongVoRkAttack00 */
    { 0x1f2fb18, 0x1e97ed8, 16, 16 }, /* gSongVoRkAttack03 */
    { 0x1f2fba4, 0x1e97f64, 16, 16 }, /* gSongVoRkAttack02 */
    { 0x1f2fbf8, 0x1e97fb8, 16, 16 }, /* gSongVoRkDamage01 */
    { 0x1f2fc30, 0x1e97ff0, 16, 16 }, /* gSongVoRkDeath00 */
    { 0x1f2fca0, 0x1e98060, 20, 20 }, /* gSongVoAnAttack00 */
    { 0x1f2fcc0, 0x1e98080, 20, 20 }, /* gSongVoAnAttack01 */
    { 0x1f2fce0, 0x1e980a0, 16, 16 }, /* gSongVoBoAttack00 */
    { 0x1f2fcfc, 0x1e980bc, 20, 20 }, /* gSongVoUrAttack00 */
    { 0x1f2fd44, 0x1e98104, 20, 20 }, /* gSongVoVicAttack00 */
    { 0x1f2fd64, 0x1e98124, 20, 20 }, /* gSongVoVicAttack01 */
    { 0x1f2fda4, 0x1e98164, 20, 20 }, /* gSongVoMarlAttack00 */
    { 0x1f2fe04, 0x1e981c4, 20, 20 }, /* gSongVoMarlAttack03 */
    { 0x1f2fe44, 0x1e98204, 20, 20 }, /* gSongSnd283 */
    { 0x1f2fe84, 0x1e98244, 20, 20 }, /* gSongSnd285 */
    { 0x1f2fec4, 0x1e98284, 20, 20 }, /* gSongSnd287 */
    { 0x1f2ff04, 0x1e982c4, 20, 20 }, /* gSongSnd289 */
};
#endif
#if defined(VERSION_US)
static const UndubSample kUndubSamples[] = {
    { 0x1d6e368, 0x1e2d7ec, 0x1e06cf8, 2390 }, /* the voice group's first instrument, SONG_VO_SR_ATTACK00 (issue #45, check_undub.py) */
    { 0x1d6e374, 0x1e2e144, 0x1e07650, 2300 },
    { 0x1d6e380, 0x1e2ea44, 0x1e07f50, 2318 },
    { 0x1d6e38c, 0x1e2f354, 0x1e08860, 2839 },
    { 0x1d6e3a4, 0x1e2fe6c, 0x1e09378, 4357 },
    { 0x1d6e3bc, 0x1e30f74, 0x1e0a480, 6092 },
    { 0x1d6e3c8, 0x1e32744, 0x1e0bc50, 5192 },
    { 0x1d6e3d4, 0x1e33b90, 0x1e0d09c, 4238 },
    { 0x1d6e3ec, 0x1e34c20, 0x1e0e12c, 1856 },
    { 0x1d6e3f8, 0x1e35364, 0x1e0e870, 7580 },
    { 0x1d6e404, 0x1e37104, 0x1e10610, 8667 },
    { 0x1d6e410, 0x1e392e0, 0x1e127ec, 3368 },
    { 0x1d6e41c, 0x1e3a00c, 0x1e13518, 4948 },
    { 0x1d6e428, 0x1e3b364, 0x1e14870, 7656 },
    { 0x1d6e434, 0x1e3d150, 0x1e1665c, 4579 },
    { 0x1d6e440, 0x1e3e334, 0x1e17840, 4844 },
    { 0x1d6e44c, 0x1e3f624, 0x1e18b30, 4753 },
    { 0x1d6e458, 0x1e408b8, 0x1e19dc4, 4538 },
    { 0x1d6e464, 0x1e41a74, 0x1e1af80, 6514 },
    { 0x1d6e470, 0x1e433e8, 0x1e1c8f4, 4495 },
    { 0x1d6e488, 0x1e44578, 0x1e1da84, 4775 },
    { 0x1d6e494, 0x1e45820, 0x1e1ed2c, 2765 },
    { 0x1d6e4a0, 0x1e462f0, 0x1e1f7fc, 1945 },
    { 0x1d6e4ac, 0x1e46a8c, 0x1e1ff98, 2070 },
    { 0x1d6e4b8, 0x1e472a4, 0x1e207b0, 7374 },
    { 0x1d6e4c4, 0x1e48f74, 0x1e22480, 5219 },
    { 0x1d6e4d0, 0x1e4a3d8, 0x1e238e4, 5633 },
    { 0x1d6e4dc, 0x1e4b9dc, 0x1e24ee8, 5572 },
    { 0x1d6e4e8, 0x1e4cfa4, 0x1e264b0, 6163 },
    { 0x1d6e4f4, 0x1e4e7b8, 0x1e27cc4, 1726 },
    { 0x1d6e500, 0x1e4ee78, 0x1e28384, 4484 },
    { 0x1d6e50c, 0x1e50000, 0x1e2950c, 6504 },
    { 0x1d6e518, 0x1e5196c, 0x1e2ae78, 5100 },
    { 0x1d6e524, 0x1e52d5c, 0x1e2c268, 4927 },
    { 0x1d6e530, 0x1e5409c, 0x1e2d5a8, 5563 },
    { 0x1d6e53c, 0x1e55658, 0x1e2eb64, 9451 },
    { 0x1d6e548, 0x1e57b44, 0x1e31050, 2757 },
    { 0x1d6e554, 0x1e5860c, 0x1e31b18, 3526 },
    { 0x1d6e560, 0x1e593d4, 0x1e328e0, 6494 },
    { 0x1d6e56c, 0x1e5ad34, 0x1e34240, 4791 },
    { 0x1d6e578, 0x1e5bfec, 0x1e354f8, 1331 },
    { 0x1d6e584, 0x1e5c520, 0x1e35a2c, 2885 },
    { 0x1d6e590, 0x1e5d068, 0x1e36574, 1897 },
    { 0x1d6e59c, 0x1e5d7d4, 0x1e36ce0, 1800 },
    { 0x1d6e5a8, 0x1e5dee0, 0x1e373ec, 1944 },
    { 0x1d6e5b4, 0x1e5e67c, 0x1e37b88, 2736 },
    { 0x1d6e5c0, 0x1e5f130, 0x1e3863c, 2604 },
    { 0x1d6e5cc, 0x1e5fb60, 0x1e3906c, 4181 },
    { 0x1d6e5d8, 0x1e60bb8, 0x1e3a0c4, 12591 },
    { 0x1d6e5e4, 0x1e63ce8, 0x1e3d1f4, 4632 },
    { 0x1d6e5f0, 0x1e64f04, 0x1e3e410, 5983 },
    { 0x1d6e5fc, 0x1e66664, 0x1e3fb70, 2760 },
    { 0x1d6e608, 0x1e67130, 0x1e4063c, 7156 },
    { 0x1d6e614, 0x1e68d28, 0x1e42234, 10729 },
    { 0x1d6e620, 0x1e6b714, 0x1e44c20, 1785 },
    { 0x1d6e62c, 0x1e6be10, 0x1e4531c, 1737 },
    { 0x1d6e638, 0x1e6c4dc, 0x1e459e8, 8074 },
    { 0x1d6e644, 0x1e6e468, 0x1e47974, 2413 },
    { 0x1d6e650, 0x1e6edd8, 0x1e482e4, 2712 },
    { 0x1d6e65c, 0x1e6f874, 0x1e48d80, 4434 },
    { 0x1d6e668, 0x1e709c8, 0x1e49ed4, 6669 },
    { 0x1d6e674, 0x1e723d8, 0x1e4b8e4, 1692 },
    { 0x1d6e680, 0x1e72a78, 0x1e4bf84, 3093 },
    { 0x1d6e68c, 0x1e73690, 0x1e4cb9c, 2915 },
    { 0x1d6e698, 0x1e741f4, 0x1e4d700, 6095 },
    { 0x1d6e6a4, 0x1e759c4, 0x1e4eed0, 3342 },
    { 0x1d6e6b0, 0x1e766d4, 0x1e4fbe0, 4802 },
    { 0x1d6e6bc, 0x1e77998, 0x1e50ea4, 5800 },
    { 0x1d6e6c8, 0x1e79044, 0x1e52550, 6330 },
    { 0x1d6e6d4, 0x1e7a900, 0x1e53e0c, 8086 },
    { 0x1d6e6e0, 0x1e7c898, 0x1e55da4, 12417 },
    { 0x1d6e6ec, 0x1e7f91c, 0x1e58e28, 5308 },
    { 0x1d6e6f8, 0x1e80ddc, 0x1e5a2e8, 4030 },
    { 0x1d6e704, 0x1e81d9c, 0x1e5b2a8, 5754 },
    { 0x1d6e710, 0x1e83418, 0x1e5c924, 1764 },
    { 0x1d6e71c, 0x1e83b00, 0x1e5d00c, 9314 },
    { 0x1d6e728, 0x1e85f64, 0x1e5f470, 1226 },
    { 0x1d6e734, 0x1e86430, 0x1e5f93c, 6479 },
    { 0x1d6e740, 0x1e87d80, 0x1e6128c, 3947 },
    { 0x1d6e74c, 0x1e88cec, 0x1e621f8, 3332 },
    { 0x1d6e758, 0x1e899f4, 0x1e62f00, 5791 },
    { 0x1d6e764, 0x1e8b094, 0x1e645a0, 6223 },
    { 0x1d6e770, 0x1e8c8e4, 0x1e65df0, 6047 },
    { 0x1d6e77c, 0x1e8e084, 0x1e67590, 4840 },
    { 0x1d6e788, 0x1e8f370, 0x1e6887c, 4137 },
    { 0x1d6e794, 0x1e9039c, 0x1e698a8, 3474 },
    { 0x1d6e818, 0x1e91130, 0x0, 16 },
};
static const UndubSong kUndubSongs[] = {
    { 0x1ebe440, 0x1e9672c, 16, 16 }, /* gSongVoSrAttack00 */
    { 0x1ebe478, 0x1e96764, 16, 16 }, /* gSongVoSrAttack02 */
    { 0x1ebe494, 0x1e96780, 16, 16 }, /* gSongVoSrAttack03 */
    { 0x1ebe4cc, 0x1e967b8, 16, 16 }, /* gSongVoSrAttack07 */
    { 0x1ebe558, 0x1e96844, 16, 16 }, /* gSongVoSrDeath00 */
    { 0x1ebe574, 0x1e96860, 16, 16 }, /* gSongVoSrFire00 */
    { 0x1ebe590, 0x1e9687c, 16, 16 }, /* gSongVoSrBurizad00 */
    { 0x1ebe5ac, 0x1e96898, 16, 16 }, /* gSongVoSrThnder00 */
    { 0x1ebe5c8, 0x1e968b4, 16, 16 }, /* gSongVoSrCarel00 */
    { 0x1ebe5e4, 0x1e968d0, 16, 16 }, /* gSongVoSrGurabi00 */
    { 0x1ebe600, 0x1e968ec, 16, 16 }, /* gSongVoSrStop00 */
    { 0x1ebe61c, 0x1e96908, 16, 16 }, /* gSongVoSrAiro00 */
    { 0x1ebe638, 0x1e96924, 16, 16 }, /* gSongVoSrSummon00 */
    { 0x1ebe654, 0x1e96940, 16, 16 }, /* gSongVoSrSummon01 */
    { 0x1ebe6ec, 0x1e969d8, 20, 20 }, /* gSongVoGfAttack00 */
    { 0x1ebe70c, 0x1e969f8, 20, 20 }, /* gSongVoDlAttack00 */
    { 0x1ebe72c, 0x1e96a18, 16, 16 }, /* gSongVoSrSummon04 */
    { 0x1ebe780, 0x1e96a6c, 16, 16 }, /* gSongVoSrAttack09 */
    { 0x1ebe79c, 0x1e96a88, 16, 16 }, /* gSongVoSrSummon07 */
    { 0x1ebe7b8, 0x1e96aa4, 16, 16 }, /* gSongVoSrSummon08 */
    { 0x1ebe7d4, 0x1e96ac0, 16, 16 }, /* gSongVoSrSummon09 */
    { 0x1ebe828, 0x1e96b14, 20, 20 }, /* gSongVoBeAttack00 */
    { 0x1ebe848, 0x1e96b34, 20, 20 }, /* gSongVoArAttack00 */
    { 0x1ebe888, 0x1e96b74, 20, 20 }, /* gSongVoAdAttack00 */
    { 0x1ebe8c8, 0x1e96bb4, 20, 20 }, /* gSongVoJcAttack01 */
    { 0x1ebe948, 0x1e96c34, 16, 16 }, /* gSongVoHoVoice00 */
    { 0x1ebfb7c, 0x1e97e68, 16, 16 }, /* gSongVoHaAttack00 */
    { 0x1ebfb98, 0x1e97e84, 16, 16 }, /* gSongVoHaAttack01 */
    { 0x1ebfbb4, 0x1e97ea0, 16, 16 }, /* gSongVoRkAttack00 */
    { 0x1ebfbec, 0x1e97ed8, 16, 16 }, /* gSongVoRkAttack03 */
    { 0x1ebfc78, 0x1e97f64, 16, 16 }, /* gSongVoRkAttack02 */
    { 0x1ebfccc, 0x1e97fb8, 16, 16 }, /* gSongVoRkDamage01 */
    { 0x1ebfd04, 0x1e97ff0, 16, 16 }, /* gSongVoRkDeath00 */
    { 0x1ebfd74, 0x1e98060, 20, 20 }, /* gSongVoAnAttack00 */
    { 0x1ebfd94, 0x1e98080, 20, 20 }, /* gSongVoAnAttack01 */
    { 0x1ebfdb4, 0x1e980a0, 16, 16 }, /* gSongVoBoAttack00 */
    { 0x1ebfdd0, 0x1e980bc, 20, 20 }, /* gSongVoUrAttack00 */
    { 0x1ebfe18, 0x1e98104, 20, 20 }, /* gSongVoVicAttack00 */
    { 0x1ebfe38, 0x1e98124, 20, 20 }, /* gSongVoVicAttack01 */
    { 0x1ebfe78, 0x1e98164, 20, 20 }, /* gSongVoMarlAttack00 */
    { 0x1ebfed8, 0x1e981c4, 20, 20 }, /* gSongVoMarlAttack03 */
    { 0x1ebff18, 0x1e98204, 20, 20 }, /* gSongSnd283 */
    { 0x1ebff58, 0x1e98244, 20, 20 }, /* gSongSnd285 */
    { 0x1ebff98, 0x1e98284, 20, 20 }, /* gSongSnd287 */
    { 0x1ebffd8, 0x1e982c4, 20, 20 }, /* gSongSnd289 */
};
#endif

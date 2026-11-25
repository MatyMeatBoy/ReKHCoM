/* One row of the handheld's 240 -> 640 nearest fit. With a 16.16 step of
 * (240 << 16) / 640 and pixel-centre sampling, every three source pixels
 * a b c become a a a b b c c c. Include after <arm_neon.h>. */
static inline void FbFitRow240To640(uint32_t* const* rows, int count, const uint32_t* input) {
    for (int group = 0; group < 80; group++, input += 3) {
        const uint32x2_t ab = vld1_u32(input);
        const uint32x2_t cc = vld1_dup_u32(input + 2);
        const uint32x4_t first = vcombine_u32(vdup_lane_u32(ab, 0), ab);
        const uint32x4_t second = vcombine_u32(vext_u32(ab, cc, 1), cc);
        for (int row = 0; row < count; row++) {
            vst1q_u32(rows[row] + 8 * group, first);
            vst1q_u32(rows[row] + 8 * group + 4, second);
        }
    }
}

/* The same rows built once and then copied, so the processor writes one
 * sequential stream at a time instead of interleaving up to three. */
static inline void FbFitRows240To640Copy(uint32_t* const* rows, int count, const uint32_t* input) {
    FbFitRow240To640(rows, 1, input);
    for (int row = 1; row < count; row++)
        memcpy(rows[row], rows[0], 640 * sizeof(**rows));
}

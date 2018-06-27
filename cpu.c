/*
 * Copyright (c) 2018 Sugizaki Yukimasa (ysugi@idein.jp)
 * All rights reserved.
 *
 * This software is licensed under a Modified (3-Clause) BSD License.
 * You should have received a copy of this license along with this
 * software. If not, contact the copyright holder above.
 */

/*
 * List of ARM and Thumb insns: http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0489i/Cihedhif.html
 * List of NEON and VFP insns: http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0489i/Bcfjicfj.html
 */

/*
 * According to [1], Cortex-A57 has 2 Simple Cluter units, 1 Branch unit, 2
 * Complex Cluster units, 1 Integer unit and 2 Load/Store units.  According to
 * [2], it seems that Cortex-A53 has only one of the each units.
 *
 * [1]: https://pc.watch.impress.co.jp/video/pcw/docs/592/202/p21.pdf
 * [2]: https://pc.watch.impress.co.jp/img/pcw/docs/569/691/221.jpg
 */

#if !defined(__arm__) || defined(__aarch64__)
#error "This code is for AArch32 only.  Use a suitable compiler."
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#if 0
#define MEASURE_CPI
#include "cycle_count.h"
#define mb() asm volatile ("" : : : "memory")
#endif

int do_cpu(const unsigned nsec)
{
    /* The number of loops */
    const unsigned num_loops = (1<<20) * (1200.0/17) * nsec;
    unsigned i __attribute__((aligned(4))) = num_loops;
#ifdef MEASURE_CPI
    long long start, end;
#endif
    /*
     * CPU L2C of RPi 3 has 512 sets and 16 ways with 16 word of line size.  So
     * the stride between memory areas which will be located to the same way of
     * a set is 512*(16*4)=32768B.
     */
    void *p = malloc(32768 * 8);

    if (p == NULL) {
        fprintf(stderr, "malloc: %s\n", strerror(errno));
        return 1;
    }

#ifdef MEASURE_CPI
    cycle_count_init();
    start = cycle_count();
    mb();
#endif

    asm volatile (
            "mov r2, #8\n\t"        /* idx=1,2,4 */
            "add r3, r2, r2, lsl#1\n\t" /* idx=3,6   */
            "add r4, r2, r2, lsl#2\n\t" /* idx=5     */
            "add r5, r2, r3, lsl#1\n\t" /* idx=7 */
            ".align 4\n\t"
            "0:\n\t"
            "subs %[cnt], #1\n\t"
                                           /* lat th */
            //"vsqrt.f32   s0,  s1\n\t"      /*  12  9 */
            //"vdiv.f32    s2,  s3,  s3\n\t" /*  13 10 */
            //"vtrn.8      q8,  q9\n\t"      /*   4  3 */

            "vfms.f32    q0,  q0,  q0\n\t" /*   8  1 */
            //"ldr  r0,  [%[p], r4, lsl#0]\n\t"
            "vrecps.f32  q1,  q1,  q1\n\t" /*   8  1 */
            //"ldr  r1,  [%[p], r2, lsl#0]\n\t"
            "mlsgt r1, r4, r5, r3\n\t"

            "vfms.f32    q2,  q2,  q2\n\t" /*   8  1 */
            "ldrgt  r0,  [%[p], r3, lsl#1]\n\t"
            "vrecps.f32  q3,  q3,  q3\n\t" /*   8  1 */
            "ldrgt  r1,  [%[p], r2, lsl#1]\n\t"

            "vfms.f32    q4,  q4,  q4\n\t" /*   8  1 */
            "ldrgt  r0,  [%[p], r5, lsl#0]\n\t"
            "vrecps.f32  q5,  q5,  q5\n\t" /*   8  1 */
            "ldrgt  r1,  [%[p], r3, lsl#0]\n\t"

            "vfms.f32    q6,  q6,  q6\n\t" /*   8  1 */
            "ldrgt  r0,  [%[p], #3]\n\t"
            "vrecps.f32  q7,  q7,  q7\n\t" /*   8  1 */
            "ldrgt  r1,  [%[p], r2, lsl#2]\n\t"

            "vfms.f32    q8,  q8,  q8\n\t" /*   8  1 */
            "ldrgt  r0,  [%[p], r2, lsl#0]\n\t"
            "vrecps.f32  q9,  q9,  q9\n\t" /*   8  1 */
            "ldrgt  r1,  [%[p], r4, lsl#0]\n\t"

            "vfms.f32   q10, q10, q10\n\t" /*   8  1 */
            "ldrgt  r0,  [%[p], r2, lsl#1]\n\t"
            "vrecps.f32 q11, q11, q11\n\t" /*   8  1 */
            "ldrgt  r1,  [%[p], r3, lsl#1]\n\t"

            "vfms.f32   q12, q12, q12\n\t" /*   8  1 */
            "ldrgt  r0,  [%[p], r3, lsl#0]\n\t"
            "vrecps.f32 q13, q13, q13\n\t" /*   8  1 */
            "ldrgt  r1,  [%[p], r5, lsl#0]\n\t"

            "vfms.f32   q14, q14, q14\n\t" /*   8  1 */
            "ldrgt  r0,  [%[p], r2, lsl#2]\n\t"
            "vrecps.f32 q15, q15, q15\n\t" /*   8  1 */
            "ldrgt  r1,  [%[p], #3]\n\t"

            "bgt 0b\n\t"
            : [cnt] "+r" (i)
            : [p]   "r"  (p)
            :  "r0",  "r1",  "r2", "r3", "r4", "r5",
               "q0",  "q1",  "q2",  "q3",  "q4",  "q5",  "q6",  "q7",
               "q8",  "q9", "q10", "q11", "q12", "q13", "q14", "q15",
               "cc", "memory"
    );

#ifdef MEASURE_CPI
    mb();
    end = cycle_count();
    mb();
    printf("CPI=%e\n", (double) (end - start) / num_loops);
    cycle_count_finalize();
#endif
    return 0;
}

int main(int argc, char *argv[])
{
    unsigned nsec;

    if (argc != 1 + 1)
        return 1;
    nsec = atol(argv[1]);
    return do_cpu(nsec);
}

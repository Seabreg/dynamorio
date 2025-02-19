/* **********************************************************
 * Copyright (c) 2011-2019 Google, Inc.  All rights reserved.
 * Copyright (c) 2007-2008 VMware, Inc.  All rights reserved.
 * **********************************************************/

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of VMware, Inc. nor the names of its contributors may be
 *   used to endorse or promote products derived from this software without
 *   specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL VMWARE, INC. OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

/* Define DR_FAST_IR to verify that everything compiles when we call the inline
 * versions of these routines.
 */
#ifndef STANDALONE_DECODER
#    define DR_FAST_IR 1
#endif

/* Uses the DR CLIENT_INTERFACE API, using DR as a standalone library, rather than
 * being a client library working with DR on a target program.
 *
 * To run, you need to put dynamorio.dll into either the current directory
 * or system32.
 */

#ifndef USE_DYNAMO
#    error NEED USE_DYNAMO
#endif

#include "configure.h"
#include "dr_api.h"
#include "tools.h"

#ifdef WINDOWS
#    define _USE_MATH_DEFINES 1
#    include <math.h> /* for M_PI, M_LN2, and M_LN10 for OP_fldpi, etc. */
#endif

#define VERBOSE 0

#ifdef STANDALONE_DECODER
#    define ASSERT(x)                                                              \
        ((void)((!(x)) ? (fprintf(stderr, "ASSERT FAILURE: %s:%d: %s\n", __FILE__, \
                                  __LINE__, #x),                                   \
                          abort(), 0)                                              \
                       : 0))
#else
#    define ASSERT(x)                                                                 \
        ((void)((!(x)) ? (dr_fprintf(STDERR, "ASSERT FAILURE: %s:%d: %s\n", __FILE__, \
                                     __LINE__, #x),                                   \
                          dr_abort(), 0)                                              \
                       : 0))
#endif

#define BOOLS_MATCH(b1, b2) (!!(b1) == !!(b2))

#define BUFFER_SIZE_BYTES(buf) sizeof(buf)
#define BUFFER_SIZE_ELEMENTS(buf) (BUFFER_SIZE_BYTES(buf) / sizeof(buf[0]))

static byte buf[32768];

#define DEFAULT_DISP 0x37
#define EVEX_SCALABLE_DISP 0x100
static int memarg_disp = DEFAULT_DISP;

/***************************************************************************
 * make sure the following are consistent (though they could still all be wrong :))
 * with respect to instr length and opcode:
 * - decode_fast
 * - decode
 * - INSTR_CREATE_
 * - encode
 */

/* we split testing up to avoid VS2010 from taking 25 minutes to compile
 * this file.
 * we cannot pass on variadic args as separate args to another
 * macro, so we must split ours by # args (xref PR 208603).
 */

/* we can encode+fast-decode some instrs cross-platform but we
 * leave that testing to the regression run on that platform
 */

/* these are shared among all test_all_opcodes_*() routines: */
#define MEMARG(sz) (opnd_create_base_disp(DR_REG_XCX, DR_REG_NULL, 0, memarg_disp, sz))
#define IMMARG(sz) opnd_create_immed_int(37, sz)
#define TGTARG opnd_create_instr(instrlist_last(ilist))
#define REGARG(reg) opnd_create_reg(DR_REG_##reg)
#define REGARG_PARTIAL(reg, sz) opnd_create_reg_partial(DR_REG_##reg, sz)
#define VSIBX6(sz) (opnd_create_base_disp(DR_REG_XCX, DR_REG_XMM6, 2, 0x42, sz))
#define VSIBY6(sz) (opnd_create_base_disp(DR_REG_XDX, DR_REG_YMM6, 2, 0x17, sz))
#define VSIBZ6(sz) (opnd_create_base_disp(DR_REG_XDX, DR_REG_ZMM6, 2, 0x35, sz))
#define VSIBX15(sz) (opnd_create_base_disp(DR_REG_XCX, DR_REG_XMM15, 2, 0x42, sz))
#define VSIBY15(sz) (opnd_create_base_disp(DR_REG_XDX, DR_REG_YMM15, 2, 0x17, sz))
#define VSIBZ31(sz) (opnd_create_base_disp(DR_REG_XDX, DR_REG_ZMM31, 2, 0x35, sz))

#define X86_ONLY 1
#define X64_ONLY 2
#define VERIFY_EVEX 4
#define FIRST_EVEX_BYTE 0x62

/****************************************************************************
 * OPCODE_FOR_CREATE 0 args
 */

#define OPCODE_FOR_CREATE(name, opc, icnm, flags)                 \
    do {                                                          \
        if ((flags & IF_X64_ELSE(X86_ONLY, X64_ONLY)) == 0) {     \
            instrlist_append(ilist, INSTR_CREATE_##icnm(dc));     \
            len_##name = instr_length(dc, instrlist_last(ilist)); \
        }                                                         \
    } while (0);
#define XOPCODE_FOR_CREATE(name, opc, icnm, flags)                \
    do {                                                          \
        if ((flags & IF_X64_ELSE(X86_ONLY, X64_ONLY)) == 0) {     \
            instrlist_append(ilist, XINST_CREATE_##icnm(dc));     \
            len_##name = instr_length(dc, instrlist_last(ilist)); \
        }                                                         \
    } while (0);

static void
test_all_opcodes_0(void *dc)
{
#define INCLUDE_NAME "ir_x86_0args.h"
#include "ir_x86_all_opc.h"
#undef INCLUDE_NAME
}

#undef OPCODE_FOR_CREATE
#undef XOPCODE_FOR_CREATE

/****************************************************************************
 * OPCODE_FOR_CREATE 1 arg
 */

#define OPCODE_FOR_CREATE(name, opc, icnm, flags, arg1)             \
    do {                                                            \
        if ((flags & IF_X64_ELSE(X86_ONLY, X64_ONLY)) == 0) {       \
            instrlist_append(ilist, INSTR_CREATE_##icnm(dc, arg1)); \
            len_##name = instr_length(dc, instrlist_last(ilist));   \
        }                                                           \
    } while (0);
#define XOPCODE_FOR_CREATE(name, opc, icnm, flags, arg1)            \
    do {                                                            \
        if ((flags & IF_X64_ELSE(X86_ONLY, X64_ONLY)) == 0) {       \
            instrlist_append(ilist, XINST_CREATE_##icnm(dc, arg1)); \
            len_##name = instr_length(dc, instrlist_last(ilist));   \
        }                                                           \
    } while (0);

#ifndef STANDALONE_DECODER
/* vs2005 cl takes many minute to compile w/ static drdecode lib
 * so we disable part of this test since for static we just want a
 * sanity check
 */
static void
test_all_opcodes_1(void *dc)
{
#    define INCLUDE_NAME "ir_x86_1args.h"
#    include "ir_x86_all_opc.h"
#    undef INCLUDE_NAME
}

#    undef OPCODE_FOR_CREATE
#    undef XOPCODE_FOR_CREATE

/****************************************************************************
 * OPCODE_FOR_CREATE 2 args
 */

#    define OPCODE_FOR_CREATE(name, opc, icnm, flags, arg1, arg2)             \
        do {                                                                  \
            if ((flags & IF_X64_ELSE(X86_ONLY, X64_ONLY)) == 0) {             \
                instrlist_append(ilist, INSTR_CREATE_##icnm(dc, arg1, arg2)); \
                len_##name = instr_length(dc, instrlist_last(ilist));         \
            }                                                                 \
        } while (0);
#    define XOPCODE_FOR_CREATE(name, opc, icnm, flags, arg1, arg2)            \
        do {                                                                  \
            if ((flags & IF_X64_ELSE(X86_ONLY, X64_ONLY)) == 0) {             \
                instrlist_append(ilist, XINST_CREATE_##icnm(dc, arg1, arg2)); \
                len_##name = instr_length(dc, instrlist_last(ilist));         \
            }                                                                 \
        } while (0);

static void
test_all_opcodes_2(void *dc)
{
#    define INCLUDE_NAME "ir_x86_2args.h"
#    include "ir_x86_all_opc.h"
#    undef INCLUDE_NAME
}

static void
test_all_opcodes_2_mm(void *dc)
{
#    define INCLUDE_NAME "ir_x86_2args_mm.h"
#    include "ir_x86_all_opc.h"
#    undef INCLUDE_NAME
}

static void
test_all_opcodes_2_avx512_vex(void *dc)
{
#    define INCLUDE_NAME "ir_x86_2args_avx512_vex.h"
#    include "ir_x86_all_opc.h"
#    undef INCLUDE_NAME
}

static void
test_all_opcodes_2_avx512_evex_mask(void *dc)
{
#    define INCLUDE_NAME "ir_x86_2args_avx512_evex_mask.h"
#    include "ir_x86_all_opc.h"
#    undef INCLUDE_NAME
}

/* No separate memarg_disp = EVEX_SCALABLE_DISP compressed displacement test needed. */

#    undef OPCODE_FOR_CREATE
#    undef XOPCODE_FOR_CREATE

/****************************************************************************
 * OPCODE_FOR_CREATE 2 args, evex encoding hint
 */

#    define OPCODE_FOR_CREATE(name, opc, icnm, flags, arg1, arg2)            \
        do {                                                                 \
            if ((flags & IF_X64_ELSE(X86_ONLY, X64_ONLY)) == 0) {            \
                instrlist_append(                                            \
                    ilist,                                                   \
                    INSTR_ENCODING_HINT(INSTR_CREATE_##icnm(dc, arg1, arg2), \
                                        DR_ENCODING_HINT_X86_EVEX));         \
                len_##name = instr_length(dc, instrlist_last(ilist));        \
            }                                                                \
        } while (0);

static void
test_all_opcodes_2_avx512_evex(void *dc)
{
#    define INCLUDE_NAME "ir_x86_2args_avx512_evex.h"
#    include "ir_x86_all_opc.h"
#    undef INCLUDE_NAME
}

static void
test_all_opcodes_2_avx512_evex_scaled_disp8(void *dc)
{
#    define INCLUDE_NAME "ir_x86_2args_avx512_evex.h"
    memarg_disp = EVEX_SCALABLE_DISP;
#    include "ir_x86_all_opc.h"
    memarg_disp = DEFAULT_DISP;
#    undef INCLUDE_NAME
}

#    undef OPCODE_FOR_CREATE
#    undef XOPCODE_FOR_CREATE

/****************************************************************************
 * OPCODE_FOR_CREATE 3 args
 */

#    define OPCODE_FOR_CREATE(name, opc, icnm, flags, arg1, arg2, arg3)             \
        do {                                                                        \
            if ((flags & IF_X64_ELSE(X86_ONLY, X64_ONLY)) == 0) {                   \
                instrlist_append(ilist, INSTR_CREATE_##icnm(dc, arg1, arg2, arg3)); \
                len_##name = instr_length(dc, instrlist_last(ilist));               \
            }                                                                       \
        } while (0);
#    define XOPCODE_FOR_CREATE(name, opc, icnm, flags, arg1, arg2, arg3)            \
        do {                                                                        \
            if ((flags & IF_X64_ELSE(X86_ONLY, X64_ONLY)) == 0) {                   \
                instrlist_append(ilist, XINST_CREATE_##icnm(dc, arg1, arg2, arg3)); \
                len_##name = instr_length(dc, instrlist_last(ilist));               \
            }                                                                       \
        } while (0);

static void
test_all_opcodes_3(void *dc)
{
#    define INCLUDE_NAME "ir_x86_3args.h"
#    include "ir_x86_all_opc.h"
#    undef INCLUDE_NAME
}

static void
test_all_opcodes_3_avx(void *dc)
{
#    define INCLUDE_NAME "ir_x86_3args_avx.h"
#    include "ir_x86_all_opc.h"
#    undef INCLUDE_NAME
}

static void
test_all_opcodes_3_avx512_vex(void *dc)
{
#    define INCLUDE_NAME "ir_x86_3args_avx512_vex.h"
#    include "ir_x86_all_opc.h"
#    undef INCLUDE_NAME
}

static void
test_all_opcodes_3_avx512_evex_mask(void *dc)
{
#    define INCLUDE_NAME "ir_x86_3args_avx512_evex_mask.h"
#    include "ir_x86_all_opc.h"
#    undef INCLUDE_NAME
}

static void
test_all_opcodes_3_avx512_evex_mask_scaled_disp8(void *dc)
{
#    define INCLUDE_NAME "ir_x86_3args_avx512_evex_mask.h"
    memarg_disp = EVEX_SCALABLE_DISP;
#    include "ir_x86_all_opc.h"
    memarg_disp = DEFAULT_DISP;
#    undef INCLUDE_NAME
}

#    undef OPCODE_FOR_CREATE
#    undef XOPCODE_FOR_CREATE

/****************************************************************************
 * OPCODE_FOR_CREATE 3 args, evex encoding hint
 */

#    define OPCODE_FOR_CREATE(name, opc, icnm, flags, arg1, arg2, arg3)            \
        do {                                                                       \
            if ((flags & IF_X64_ELSE(X86_ONLY, X64_ONLY)) == 0) {                  \
                instrlist_append(                                                  \
                    ilist,                                                         \
                    INSTR_ENCODING_HINT(INSTR_CREATE_##icnm(dc, arg1, arg2, arg3), \
                                        DR_ENCODING_HINT_X86_EVEX));               \
                len_##name = instr_length(dc, instrlist_last(ilist));              \
            }                                                                      \
        } while (0);

static void
test_all_opcodes_3_avx512_evex(void *dc)
{
#    define INCLUDE_NAME "ir_x86_3args_avx512_evex.h"
#    include "ir_x86_all_opc.h"
#    undef INCLUDE_NAME
}

static void
test_all_opcodes_3_avx512_evex_scaled_disp8(void *dc)
{
#    define INCLUDE_NAME "ir_x86_3args_avx512_evex.h"
    memarg_disp = EVEX_SCALABLE_DISP;
#    include "ir_x86_all_opc.h"
    memarg_disp = DEFAULT_DISP;
#    undef INCLUDE_NAME
}

#    undef OPCODE_FOR_CREATE
#    undef XOPCODE_FOR_CREATE

/****************************************************************************
 * OPCODE_FOR_CREATE 4 args, evex encoding hint
 */

#    define OPCODE_FOR_CREATE(name, opc, icnm, flags, arg1, arg2, arg3, arg4)            \
        do {                                                                             \
            if ((flags & IF_X64_ELSE(X86_ONLY, X64_ONLY)) == 0) {                        \
                instrlist_append(                                                        \
                    ilist,                                                               \
                    INSTR_ENCODING_HINT(INSTR_CREATE_##icnm(dc, arg1, arg2, arg3, arg4), \
                                        DR_ENCODING_HINT_X86_EVEX));                     \
                len_##name = instr_length(dc, instrlist_last(ilist));                    \
            }                                                                            \
        } while (0);

static void
test_all_opcodes_4_avx512_evex(void *dc)
{
#    define INCLUDE_NAME "ir_x86_4args_avx512_evex.h"
#    include "ir_x86_all_opc.h"
#    undef INCLUDE_NAME
}

static void
test_all_opcodes_4_avx512_evex_scaled_disp8(void *dc)
{
#    define INCLUDE_NAME "ir_x86_4args_avx512_evex.h"
    memarg_disp = EVEX_SCALABLE_DISP;
#    include "ir_x86_all_opc.h"
    memarg_disp = DEFAULT_DISP;
#    undef INCLUDE_NAME
}

#    undef OPCODE_FOR_CREATE
#    undef XOPCODE_FOR_CREATE

/****************************************************************************
 * OPCODE_FOR_CREATE 4 args
 */

#    define OPCODE_FOR_CREATE(name, opc, icnm, flags, arg1, arg2, arg3, arg4)      \
        do {                                                                       \
            if ((flags & IF_X64_ELSE(X86_ONLY, X64_ONLY)) == 0) {                  \
                instrlist_append(ilist,                                            \
                                 INSTR_CREATE_##icnm(dc, arg1, arg2, arg3, arg4)); \
                len_##name = instr_length(dc, instrlist_last(ilist));              \
            }                                                                      \
        } while (0);
#    define XOPCODE_FOR_CREATE(name, opc, icnm, flags, arg1, arg2, arg3, arg4)     \
        do {                                                                       \
            if ((flags & IF_X64_ELSE(X86_ONLY, X64_ONLY)) == 0) {                  \
                instrlist_append(ilist,                                            \
                                 XINST_CREATE_##icnm(dc, arg1, arg2, arg3, arg4)); \
                len_##name = instr_length(dc, instrlist_last(ilist));              \
            }                                                                      \
        } while (0);

static void
test_all_opcodes_4(void *dc)
{
#    define INCLUDE_NAME "ir_x86_4args.h"
#    include "ir_x86_all_opc.h"
#    undef INCLUDE_NAME
}

static void
test_all_opcodes_4_avx512_evex_mask(void *dc)
{
#    define INCLUDE_NAME "ir_x86_4args_avx512_evex_mask.h"
#    include "ir_x86_all_opc.h"
#    undef INCLUDE_NAME
}

static void
test_all_opcodes_4_avx512_evex_mask_scaled_disp8(void *dc)
{
#    define INCLUDE_NAME "ir_x86_4args_avx512_evex_mask.h"
    memarg_disp = EVEX_SCALABLE_DISP;
#    include "ir_x86_all_opc.h"
    memarg_disp = DEFAULT_DISP;
#    undef INCLUDE_NAME
}

#    undef OPCODE_FOR_CREATE
#    undef XOPCODE_FOR_CREATE

/****************************************************************************
 * OPCODE_FOR_CREATE 5 args
 */

#    define OPCODE_FOR_CREATE(name, opc, icnm, flags, arg1, arg2, arg3, arg4, arg5)      \
        do {                                                                             \
            if ((flags & IF_X64_ELSE(X86_ONLY, X64_ONLY)) == 0) {                        \
                instrlist_append(ilist,                                                  \
                                 INSTR_CREATE_##icnm(dc, arg1, arg2, arg3, arg4, arg5)); \
                len_##name = instr_length(dc, instrlist_last(ilist));                    \
            }                                                                            \
        } while (0);

static void
test_all_opcodes_5_avx512_evex_mask(void *dc)
{
#    define INCLUDE_NAME "ir_x86_5args_avx512_evex_mask.h"
#    include "ir_x86_all_opc.h"
#    undef INCLUDE_NAME
}

static void
test_all_opcodes_5_avx512_evex_mask_scaled_disp8(void *dc)
{
#    define INCLUDE_NAME "ir_x86_5args_avx512_evex_mask.h"
    memarg_disp = EVEX_SCALABLE_DISP;
#    include "ir_x86_all_opc.h"
    memarg_disp = DEFAULT_DISP;
#    undef INCLUDE_NAME
}

#    undef OPCODE_FOR_CREATE

/*
 ****************************************************************************/

static void
test_opmask_disas_avx512(void *dc)
{
    /* Test AVX-512 k-registers. */
    byte *pc;
    const byte b1[] = { 0xc5, 0xf8, 0x90, 0xee };
    const byte b2[] = { 0x67, 0xc5, 0xf8, 0x90, 0x29 };
    char dbuf[512];
    int len;

    pc =
        disassemble_to_buffer(dc, (byte *)b1, (byte *)b1, false /*no pc*/,
                              false /*no bytes*/, dbuf, BUFFER_SIZE_ELEMENTS(dbuf), &len);
    ASSERT(pc != NULL);
    ASSERT(strcmp(dbuf, "kmovw  %k6 -> %k5\n") == 0);

    pc =
        disassemble_to_buffer(dc, (byte *)b2, (byte *)b2, false /*no pc*/,
                              false /*no bytes*/, dbuf, BUFFER_SIZE_ELEMENTS(dbuf), &len);
    ASSERT(pc != NULL);
    ASSERT(strcmp(dbuf,
                  IF_X64_ELSE("addr32 kmovw  (%ecx)[2byte] -> %k5\n",
                              "addr16 kmovw  (%bx,%di)[2byte] -> %k5\n")) == 0);
}

static void
test_disas_3_avx512_evex_mask(void *dc)
{
    /* Test AVX-512 k-registers. */
    byte *pc;
    const byte b1[] = { 0x62, 0xb1, 0x7c, 0x49, 0x10, 0xc8 };
    const byte b2[] = { 0x62, 0x21, 0x7c, 0x49, 0x10, 0xf8 };
    const byte b3[] = { 0x62, 0x61, 0x7c, 0x49, 0x11, 0x3c, 0x24 };
    const byte b4[] = { 0x62, 0x61, 0x7c, 0x49, 0x10, 0x3c, 0x24 };
    const byte b5[] = { 0x62, 0x61, 0x7c, 0x49, 0x10, 0xf9 };
    const byte b6[] = { 0x62, 0xf1, 0x7c, 0x49, 0x10, 0x0c, 0x24 };
    const byte b7[] = { 0x62, 0xf1, 0x7c, 0x49, 0x10, 0xc1 };
    const byte b8[] = { 0x62, 0xf1, 0x7c, 0x49, 0x11, 0x0c, 0x24 };

    char dbuf[512];
    int len;

#    ifdef X64
    pc =
        disassemble_to_buffer(dc, (byte *)b1, (byte *)b1, false /*no pc*/,
                              false /*no bytes*/, dbuf, BUFFER_SIZE_ELEMENTS(dbuf), &len);
    ASSERT(pc != NULL);
    ASSERT(strcmp(dbuf, "vmovups {%k1} %zmm16 -> %zmm1\n") == 0);

    pc =
        disassemble_to_buffer(dc, (byte *)b2, (byte *)b2, false /*no pc*/,
                              false /*no bytes*/, dbuf, BUFFER_SIZE_ELEMENTS(dbuf), &len);
    ASSERT(pc != NULL);
    ASSERT(strcmp(dbuf, "vmovups {%k1} %zmm16 -> %zmm31\n") == 0);

    pc =
        disassemble_to_buffer(dc, (byte *)b3, (byte *)b3, false /*no pc*/,
                              false /*no bytes*/, dbuf, BUFFER_SIZE_ELEMENTS(dbuf), &len);
    ASSERT(pc != NULL);
    ASSERT(strcmp(dbuf, "vmovups {%k1} %zmm31 -> (%rsp)[64byte]\n") == 0);

    pc =
        disassemble_to_buffer(dc, (byte *)b4, (byte *)b4, false /*no pc*/,
                              false /*no bytes*/, dbuf, BUFFER_SIZE_ELEMENTS(dbuf), &len);
    ASSERT(pc != NULL);
    ASSERT(strcmp(dbuf,
                  IF_X64_ELSE("vmovups {%k1} (%rsp)[64byte] -> %zmm31\n",
                              "vmovups {%k1} (%esp)[64byte] -> %zmm31\n")) == 0);

    pc =
        disassemble_to_buffer(dc, (byte *)b5, (byte *)b5, false /*no pc*/,
                              false /*no bytes*/, dbuf, BUFFER_SIZE_ELEMENTS(dbuf), &len);
    ASSERT(pc != NULL);
    ASSERT(strcmp(dbuf, "vmovups {%k1} %zmm1 -> %zmm31\n") == 0);
#    endif

    pc =
        disassemble_to_buffer(dc, (byte *)b6, (byte *)b6, false /*no pc*/,
                              false /*no bytes*/, dbuf, BUFFER_SIZE_ELEMENTS(dbuf), &len);
    ASSERT(pc != NULL);
    ASSERT(strcmp(dbuf,
                  IF_X64_ELSE("vmovups {%k1} (%rsp)[64byte] -> %zmm1\n",
                              "vmovups {%k1} (%esp)[64byte] -> %zmm1\n")) == 0);

    pc =
        disassemble_to_buffer(dc, (byte *)b7, (byte *)b7, false /*no pc*/,
                              false /*no bytes*/, dbuf, BUFFER_SIZE_ELEMENTS(dbuf), &len);
    ASSERT(pc != NULL);
    ASSERT(strcmp(dbuf, "vmovups {%k1} %zmm1 -> %zmm0\n") == 0);

    pc =
        disassemble_to_buffer(dc, (byte *)b8, (byte *)b8, false /*no pc*/,
                              false /*no bytes*/, dbuf, BUFFER_SIZE_ELEMENTS(dbuf), &len);
    ASSERT(pc != NULL);
    ASSERT(strcmp(dbuf,
                  IF_X64_ELSE("vmovups {%k1} %zmm1 -> (%rsp)[64byte]\n",
                              "vmovups {%k1} %zmm1 -> (%esp)[64byte]\n")) == 0);
}

#endif /* !STANDALONE_DECODER */

static void
test_disp_control_helper(void *dc, int disp, bool encode_zero_disp, bool force_full_disp,
                         bool disp16, uint len_expect)
{
    byte *pc;
    uint len;
    instr_t *instr = INSTR_CREATE_mov_ld(
        dc, opnd_create_reg(DR_REG_ECX),
        opnd_create_base_disp_ex(disp16 ? IF_X64_ELSE(DR_REG_EBX, DR_REG_BX) : DR_REG_XBX,
                                 DR_REG_NULL, 0, disp, OPSZ_4, encode_zero_disp,
                                 force_full_disp, disp16));
    pc = instr_encode(dc, instr, buf);
    len = (int)(pc - (byte *)buf);
#if VERBOSE
    pc = disassemble_with_info(dc, buf, STDOUT, true, true);
#endif
    ASSERT(len == len_expect);
    instr_reset(dc, instr);
    decode(dc, buf, instr);
    ASSERT(
        instr_num_srcs(instr) == 1 && opnd_is_base_disp(instr_get_src(instr, 0)) &&
        BOOLS_MATCH(encode_zero_disp,
                    opnd_is_disp_encode_zero(instr_get_src(instr, 0))) &&
        BOOLS_MATCH(force_full_disp, opnd_is_disp_force_full(instr_get_src(instr, 0))) &&
        BOOLS_MATCH(disp16, opnd_is_disp_short_addr(instr_get_src(instr, 0))));
    instr_destroy(dc, instr);
}

/* Test encode_zero_disp and force_full_disp control from case 4457 */
static void
test_disp_control(void *dc)
{
    /*
    0x004275b4   8b 0b                mov    (%ebx) -> %ecx
    0x004275b4   8b 4b 00             mov    $0x00(%ebx) -> %ecx
    0x004275b4   8b 8b 00 00 00 00    mov    $0x00000000 (%ebx) -> %ecx
    0x004275b4   8b 4b 7f             mov    $0x7f(%ebx) -> %ecx
    0x004275b4   8b 8b 7f 00 00 00    mov    $0x0000007f (%ebx) -> %ecx
    0x00430258   67 8b 4f 7f          addr16 mov    0x7f(%bx) -> %ecx
    0x00430258   67 8b 8f 7f 00       addr16 mov    0x007f(%bx) -> %ecx
    */
    test_disp_control_helper(dc, 0, false, false, false, 2);
    test_disp_control_helper(dc, 0, true, false, false, 3);
    test_disp_control_helper(dc, 0, true, true, false, 6);
    test_disp_control_helper(dc, 0x7f, false, false, false, 3);
    test_disp_control_helper(dc, 0x7f, false, true, false, 6);
    test_disp_control_helper(dc, 0x7f, false, false, true, 4);
    test_disp_control_helper(dc, 0x7f, false, true, true, IF_X64_ELSE(7, 5));
}

/* emits the instruction to buf (for tests that wish to do additional checks on
 * the output) */
static void
test_instr_encode(void *dc, instr_t *instr, uint len_expect)
{
    instr_t *decin;
    uint len;
    byte *pc = instr_encode(dc, instr, buf);
    len = (int)(pc - (byte *)buf);
#if VERBOSE
    disassemble_with_info(dc, buf, STDOUT, true, true);
#endif
    ASSERT(len == len_expect);
    decin = instr_create(dc);
    decode(dc, buf, decin);
    ASSERT(instr_same(instr, decin));
    instr_destroy(dc, instr);
    instr_destroy(dc, decin);
}

static void
test_instr_decode(void *dc, instr_t *instr, byte *bytes, uint bytes_len, bool size_match)
{
    instr_t *decin;
    if (size_match) {
        uint len;
        byte *pc = instr_encode(dc, instr, buf);
        len = (int)(pc - (byte *)buf);
#if VERBOSE
        disassemble_with_info(dc, buf, STDOUT, true, true);
#endif
        ASSERT(len == bytes_len);
        ASSERT(memcmp(buf, bytes, bytes_len) == 0);
    }
    decin = instr_create(dc);
    decode(dc, bytes, decin);
#if VERBOSE
    print("Comparing |");
    instr_disassemble(dc, instr, STDOUT);
    print("|\n       to |");
    instr_disassemble(dc, decin, STDOUT);
    print("|\n");
#endif
    ASSERT(instr_same(instr, decin));
    instr_destroy(dc, instr);
    instr_destroy(dc, decin);
}

/* emits the instruction to buf (for tests that wish to do additional checks on
 * the output) */
static void
test_instr_encode_and_decode(void *dc, instr_t *instr, uint len_expect,
                             /* also checks one operand's size */
                             bool src, uint opnum, opnd_size_t sz, uint bytes)
{
    opnd_t op;
    opnd_size_t opsz;
    instr_t *decin;
    uint len;
    byte *pc = instr_encode(dc, instr, buf);
    len = (int)(pc - (byte *)buf);
#if VERBOSE
    disassemble_with_info(dc, buf, STDOUT, true, true);
#endif
    ASSERT(len == len_expect);
    decin = instr_create(dc);
    decode(dc, buf, decin);
    ASSERT(instr_same(instr, decin));

    /* PR 245805: variable sizes should be resolved on decode */
    if (src)
        op = instr_get_src(decin, opnum);
    else
        op = instr_get_dst(decin, opnum);
    opsz = opnd_get_size(op);
    ASSERT(opsz == sz && opnd_size_in_bytes(opsz) == bytes);

    instr_destroy(dc, instr);
    instr_destroy(dc, decin);
}

static void
test_indirect_cti(void *dc)
{
    /*
    0x004275f4   ff d1                call   %ecx %esp -> %esp (%esp)
    0x004275f4   66 ff d1             data16 call   %cx %esp -> %esp (%esp)
    0x004275f4   67 ff d1             addr16 call   %ecx %esp -> %esp (%esp)
    0x00427794   ff 19                lcall  (%ecx) %esp -> %esp (%esp)
    0x00427794   66 ff 19             data16 lcall  (%ecx) %esp -> %esp (%esp)
    0x00427794   67 ff 1f             addr16 lcall  (%bx) %esp -> %esp (%esp)
    */
    instr_t *instr;
    byte bytes_addr16_call[] = { 0x67, 0xff, 0xd1 };
    instr = INSTR_CREATE_call_ind(dc, opnd_create_reg(DR_REG_XCX));
    test_instr_encode(dc, instr, 2);
#ifndef X64 /* only on AMD can we shorten, so we don't test it */
    instr = instr_create_2dst_2src(
        dc, OP_call_ind, opnd_create_reg(DR_REG_XSP),
        opnd_create_base_disp(DR_REG_XSP, DR_REG_NULL, 0, -2, OPSZ_2),
        opnd_create_reg(DR_REG_CX), opnd_create_reg(DR_REG_XSP));
    test_instr_encode(dc, instr, 3);
#endif
    /* addr16 prefix does nothing here */
    instr = INSTR_CREATE_call_ind(dc, opnd_create_reg(DR_REG_XCX));
    test_instr_decode(dc, instr, bytes_addr16_call, sizeof(bytes_addr16_call), false);

    /* invalid to have far call go through reg since needs 6 bytes */
    instr = INSTR_CREATE_call_far_ind(
        dc, opnd_create_base_disp(DR_REG_XCX, DR_REG_NULL, 0, 0, OPSZ_6));
    test_instr_encode(dc, instr, 2);
    instr = instr_create_2dst_2src(
        dc, OP_call_far_ind, opnd_create_reg(DR_REG_XSP),
        opnd_create_base_disp(DR_REG_XSP, DR_REG_NULL, 0, -4, OPSZ_4),
        opnd_create_base_disp(DR_REG_XCX, DR_REG_NULL, 0, 0, OPSZ_4),
        opnd_create_reg(DR_REG_XSP));
    test_instr_encode(dc, instr, 3);
    instr = instr_create_2dst_2src(
        dc, OP_call_far_ind, opnd_create_reg(DR_REG_XSP),
        opnd_create_base_disp(DR_REG_XSP, DR_REG_NULL, 0, -8, OPSZ_8_rex16_short4),
        opnd_create_base_disp(IF_X64_ELSE(DR_REG_EBX, DR_REG_BX), DR_REG_NULL, 0, 0,
                              OPSZ_6),
        opnd_create_reg(DR_REG_XSP));
    test_instr_encode(dc, instr, 3);

    /* case 10710: make sure we can encode these guys
         0x00428844   0e                   push   %cs %esp -> %esp (%esp)
         0x00428844   1e                   push   %ds %esp -> %esp (%esp)
         0x00428844   16                   push   %ss %esp -> %esp (%esp)
         0x00428844   06                   push   %es %esp -> %esp (%esp)
         0x00428844   0f a0                push   %fs %esp -> %esp (%esp)
         0x00428844   0f a8                push   %gs %esp -> %esp (%esp)
         0x00428844   1f                   pop    %esp (%esp) -> %ds %esp
         0x00428844   17                   pop    %esp (%esp) -> %ss %esp
         0x00428844   07                   pop    %esp (%esp) -> %es %esp
         0x00428844   0f a1                pop    %esp (%esp) -> %fs %esp
         0x00428844   0f a9                pop    %esp (%esp) -> %gs %esp
     */
#ifndef X64
    test_instr_encode(dc, INSTR_CREATE_push(dc, opnd_create_reg(SEG_CS)), 1);
    test_instr_encode(dc, INSTR_CREATE_push(dc, opnd_create_reg(SEG_DS)), 1);
    test_instr_encode(dc, INSTR_CREATE_push(dc, opnd_create_reg(SEG_SS)), 1);
    test_instr_encode(dc, INSTR_CREATE_push(dc, opnd_create_reg(SEG_ES)), 1);
#endif
    test_instr_encode(dc, INSTR_CREATE_push(dc, opnd_create_reg(SEG_FS)), 2);
    test_instr_encode(dc, INSTR_CREATE_push(dc, opnd_create_reg(SEG_GS)), 2);
#ifndef X64
    test_instr_encode(dc, INSTR_CREATE_pop(dc, opnd_create_reg(SEG_DS)), 1);
    test_instr_encode(dc, INSTR_CREATE_pop(dc, opnd_create_reg(SEG_SS)), 1);
    test_instr_encode(dc, INSTR_CREATE_pop(dc, opnd_create_reg(SEG_ES)), 1);
#endif
    test_instr_encode(dc, INSTR_CREATE_pop(dc, opnd_create_reg(SEG_FS)), 2);
    test_instr_encode(dc, INSTR_CREATE_pop(dc, opnd_create_reg(SEG_GS)), 2);
}

static void
test_cti_prefixes(void *dc)
{
    /* case 10689: test decoding jmp/call w/ 16-bit prefixes
     *   0x00428844   66 e9 ab cd          data16 jmp    $0x55f3
     *   0x00428844   67 e9 ab cd ef 12    addr16 jmp    $0x133255f5
     */
    buf[0] = 0x66;
    buf[1] = 0xe9;
    buf[2] = 0xab;
    buf[3] = 0xcd;
    buf[4] = 0xef;
    buf[5] = 0x12;
    /* data16 (0x66) == 4 bytes, while addr16 (0x67) == 6 bytes */
#ifndef X64 /* no jmp16 for x64 */
#    if VERBOSE
    disassemble_with_info(dc, buf, STDOUT, true, true);
#    endif
    ASSERT(decode_next_pc(dc, buf) == (byte *)&buf[4]);
#endif
    buf[0] = 0x67;
#if VERBOSE
    disassemble_with_info(dc, buf, STDOUT, true, true);
#endif
    ASSERT(decode_next_pc(dc, buf) == (byte *)&buf[6]);
}

static void
test_modrm16_helper(void *dc, reg_id_t base, reg_id_t scale, uint disp, uint len)
{
    instr_t *instr;
    /* Avoid DR_REG_EAX b/c of the special 0xa0-0xa3 opcodes */
    instr = INSTR_CREATE_mov_ld(dc, opnd_create_reg(DR_REG_EBX),
                                opnd_create_base_disp(base, scale,
                                                      (scale == DR_REG_NULL ? 0 : 1),
                                                      /* we need OPSZ_4_short2 to match
                                                       * instr_same on decode! */
                                                      disp, OPSZ_4_short2));
    if (base == DR_REG_NULL && scale == DR_REG_NULL) {
        /* Don't need _ex unless abs addr, in which case should get 32-bit
         * disp!  Test both sides. */
        test_instr_encode(dc, instr, len + 1 /*32-bit disp but no prefix*/);
        instr = INSTR_CREATE_mov_ld(
            dc, opnd_create_reg(DR_REG_EBX),
            opnd_create_base_disp_ex(base, scale, (scale == DR_REG_NULL ? 0 : 1),
                                     /* we need OPSZ_4_short2 to match
                                      * instr_same on decode! */
                                     disp, OPSZ_4_short2, false, false, true));
        test_instr_encode(dc, instr, len);
    } else {
        test_instr_encode(dc, instr, len);
    }
}

static void
test_modrm16(void *dc)
{
    /*
     *   0x00428964   67 8b 18             addr16 mov    (%bx,%si,1) -> %ebx
     *   0x00428964   67 8b 19             addr16 mov    (%bx,%di,1) -> %ebx
     *   0x00428964   67 8b 1a             addr16 mov    (%bp,%si,1) -> %ebx
     *   0x00428964   67 8b 1b             addr16 mov    (%bp,%di,1) -> %ebx
     *   0x00428964   67 8b 1c             addr16 mov    (%si) -> %ebx
     *   0x00428964   67 8b 1d             addr16 mov    (%di) -> %ebx
     *   0x004289c4   8b 1d 7f 00 00 00    mov    0x7f -> %ebx
     *   0x004289c4   67 8b 1e 7f 00       addr16 mov    0x7f -> %ebx
     *   0x004289c4   67 8b 5e 00          addr16 mov    (%bp) -> %ebx
     *   0x004289c4   67 8b 1f             addr16 mov    (%bx) -> %ebx
     *   0x004289c4   67 8b 58 7f          addr16 mov    0x7f(%bx,%si,1) -> %ebx
     *   0x004289c4   67 8b 59 7f          addr16 mov    0x7f(%bx,%di,1) -> %ebx
     *   0x004289c4   67 8b 5a 7f          addr16 mov    0x7f(%bp,%si,1) -> %ebx
     *   0x004289c4   67 8b 5b 7f          addr16 mov    0x7f(%bp,%di,1) -> %ebx
     *   0x004289c4   67 8b 5c 7f          addr16 mov    0x7f(%si) -> %ebx
     *   0x004289c4   67 8b 5d 7f          addr16 mov    0x7f(%di) -> %ebx
     *   0x004289c4   67 8b 5e 7f          addr16 mov    0x7f(%bp) -> %ebx
     *   0x004289c4   67 8b 5f 7f          addr16 mov    0x7f(%bx) -> %ebx
     *   0x004289c4   67 8b 98 80 00       addr16 mov    0x0080(%bx,%si,1) -> %ebx
     *   0x004289c4   67 8b 99 80 00       addr16 mov    0x0080(%bx,%di,1) -> %ebx
     *   0x004289c4   67 8b 9a 80 00       addr16 mov    0x0080(%bp,%si,1) -> %ebx
     *   0x004289c4   67 8b 9b 80 00       addr16 mov    0x0080(%bp,%di,1) -> %ebx
     *   0x004289c4   67 8b 9c 80 00       addr16 mov    0x0080(%si) -> %ebx
     *   0x004289c4   67 8b 9d 80 00       addr16 mov    0x0080(%di) -> %ebx
     *   0x004289c4   67 8b 9e 80 00       addr16 mov    0x0080(%bp) -> %ebx
     *   0x004289c4   67 8b 9f 80 00       addr16 mov    0x0080(%bx) -> %ebx
     */
    test_modrm16_helper(dc, DR_REG_BX, DR_REG_SI, 0, 3);
    test_modrm16_helper(dc, DR_REG_BX, DR_REG_DI, 0, 3);
    test_modrm16_helper(dc, DR_REG_BP, DR_REG_SI, 0, 3);
    test_modrm16_helper(dc, DR_REG_BP, DR_REG_DI, 0, 3);
    test_modrm16_helper(dc, DR_REG_SI, DR_REG_NULL, 0, 3);
    test_modrm16_helper(dc, DR_REG_DI, DR_REG_NULL, 0, 3);
    test_modrm16_helper(dc, DR_REG_NULL, DR_REG_NULL, 0x7f, 5); /* must do disp16 */
    test_modrm16_helper(dc, DR_REG_BP, DR_REG_NULL, 0, 4);      /* must do disp8 */
    test_modrm16_helper(dc, DR_REG_BX, DR_REG_NULL, 0, 3);

    test_modrm16_helper(dc, DR_REG_BX, DR_REG_SI, 0x7f, 4);
    test_modrm16_helper(dc, DR_REG_BX, DR_REG_DI, 0x7f, 4);
    test_modrm16_helper(dc, DR_REG_BP, DR_REG_SI, 0x7f, 4);
    test_modrm16_helper(dc, DR_REG_BP, DR_REG_DI, 0x7f, 4);
    test_modrm16_helper(dc, DR_REG_SI, DR_REG_NULL, 0x7f, 4);
    test_modrm16_helper(dc, DR_REG_DI, DR_REG_NULL, 0x7f, 4);
    test_modrm16_helper(dc, DR_REG_BP, DR_REG_NULL, 0x7f, 4);
    test_modrm16_helper(dc, DR_REG_BX, DR_REG_NULL, 0x7f, 4);

    test_modrm16_helper(dc, DR_REG_BX, DR_REG_SI, 0x80, 5);
    test_modrm16_helper(dc, DR_REG_BX, DR_REG_DI, 0x80, 5);
    test_modrm16_helper(dc, DR_REG_BP, DR_REG_SI, 0x80, 5);
    test_modrm16_helper(dc, DR_REG_BP, DR_REG_DI, 0x80, 5);
    test_modrm16_helper(dc, DR_REG_SI, DR_REG_NULL, 0x80, 5);
    test_modrm16_helper(dc, DR_REG_DI, DR_REG_NULL, 0x80, 5);
    test_modrm16_helper(dc, DR_REG_BP, DR_REG_NULL, 0x80, 5);
    test_modrm16_helper(dc, DR_REG_BX, DR_REG_NULL, 0x80, 5);
}

/* PR 215143: auto-magically add size prefixes */
static void
test_size_changes(void *dc)
{
    /*
     *   0x004299d4   66 51                data16 push   %cx %esp -> %esp (%esp)
     *   0x004298a4   e3 fe                jecxz  $0x004298a4 %ecx
     *   0x004298a4   67 e3 fd             addr16 jecxz  $0x004298a4 %cx
     *   0x080a5260   67 e2 fd             addr16 loop   $0x080a5260 %cx -> %cx
     *   0x080a5260   67 e1 fd             addr16 loope  $0x080a5260 %cx -> %cx
     *   0x080a5260   67 e0 fd             addr16 loopne $0x080a5260 %cx -> %cx
     */
    instr_t *instr;
    /* addr16 doesn't affect push so we only test data16 here */
#ifndef X64 /* can only shorten on AMD */
    /* push data16 */
    instr = instr_create_2dst_2src(
        dc, OP_push, opnd_create_reg(DR_REG_XSP),
        opnd_create_base_disp(DR_REG_XSP, DR_REG_NULL, 0, -2, OPSZ_2),
        opnd_create_reg(DR_REG_CX), opnd_create_reg(DR_REG_XSP));
    test_instr_encode(dc, instr, 2);
#endif
    /* jecxz and jcxz */
    test_instr_encode(dc, INSTR_CREATE_jecxz(dc, opnd_create_pc(buf)), 2);
    /* test non-default count register size (requires addr prefix) */
    instr = instr_create_0dst_2src(dc, OP_jecxz, opnd_create_pc(buf),
                                   opnd_create_reg(IF_X64_ELSE(DR_REG_ECX, DR_REG_CX)));
    test_instr_encode(dc, instr, 3);
    instr = instr_create_1dst_2src(
        dc, OP_loop, opnd_create_reg(IF_X64_ELSE(DR_REG_ECX, DR_REG_CX)),
        opnd_create_pc(buf), opnd_create_reg(IF_X64_ELSE(DR_REG_ECX, DR_REG_CX)));
    test_instr_encode(dc, instr, 3);
    instr = instr_create_1dst_2src(
        dc, OP_loope, opnd_create_reg(IF_X64_ELSE(DR_REG_ECX, DR_REG_CX)),
        opnd_create_pc(buf), opnd_create_reg(IF_X64_ELSE(DR_REG_ECX, DR_REG_CX)));
    test_instr_encode(dc, instr, 3);
    instr = instr_create_1dst_2src(
        dc, OP_loopne, opnd_create_reg(IF_X64_ELSE(DR_REG_ECX, DR_REG_CX)),
        opnd_create_pc(buf), opnd_create_reg(IF_X64_ELSE(DR_REG_ECX, DR_REG_CX)));
    test_instr_encode(dc, instr, 3);

    /*
     *   0x004ee0b8   a6                   cmps   %ds:(%esi) %es:(%edi) %esi %edi -> %esi
     * %edi 0x004ee0b8   67 a6                addr16 cmps   %ds:(%si) %es:(%di) %si %di ->
     * %si %di 0x004ee0b8   66 a7                data16 cmps   %ds:(%esi) %es:(%edi) %esi
     * %edi -> %esi %edi 0x004ee0b8   d7                   xlat   %ds:(%ebx,%al,1) -> %al
     *   0x004ee0b8   67 d7                addr16 xlat   %ds:(%bx,%al,1) -> %al
     *   0x004ee0b8   0f f7 c1             maskmovq %mm0 %mm1 -> %ds:(%edi)
     *   0x004ee0b8   67 0f f7 c1          addr16 maskmovq %mm0 %mm1 -> %ds:(%di)
     *   0x004ee0b8   66 0f f7 c1          maskmovdqu %xmm0 %xmm1 -> %ds:(%edi)
     *   0x004ee0b8   67 66 0f f7 c1       addr16 maskmovdqu %xmm0 %xmm1 -> %ds:(%di)
     */
    test_instr_encode(dc, INSTR_CREATE_cmps_1(dc), 1);
    instr = instr_create_2dst_4src(
        dc, OP_cmps, opnd_create_reg(IF_X64_ELSE(DR_REG_ESI, DR_REG_SI)),
        opnd_create_reg(IF_X64_ELSE(DR_REG_EDI, DR_REG_DI)),
        opnd_create_far_base_disp(SEG_DS, IF_X64_ELSE(DR_REG_ESI, DR_REG_SI), DR_REG_NULL,
                                  0, 0, OPSZ_1),
        opnd_create_far_base_disp(SEG_ES, IF_X64_ELSE(DR_REG_EDI, DR_REG_DI), DR_REG_NULL,
                                  0, 0, OPSZ_1),
        opnd_create_reg(IF_X64_ELSE(DR_REG_ESI, DR_REG_SI)),
        opnd_create_reg(IF_X64_ELSE(DR_REG_EDI, DR_REG_DI)));
    test_instr_encode(dc, instr, 2);

    instr = instr_create_2dst_4src(
        dc, OP_cmps, opnd_create_reg(DR_REG_XSI), opnd_create_reg(DR_REG_XDI),
        opnd_create_far_base_disp(SEG_DS, DR_REG_XSI, DR_REG_NULL, 0, 0, OPSZ_2),
        opnd_create_far_base_disp(SEG_ES, DR_REG_XDI, DR_REG_NULL, 0, 0, OPSZ_2),
        opnd_create_reg(DR_REG_XSI), opnd_create_reg(DR_REG_XDI));
    test_instr_encode_and_decode(dc, instr, 2, true /*src*/, 0, OPSZ_2, 2);

    test_instr_encode(dc, INSTR_CREATE_xlat(dc), 1);
    instr = instr_create_1dst_1src(
        dc, OP_xlat, opnd_create_reg(DR_REG_AL),
        opnd_create_far_base_disp(SEG_DS, IF_X64_ELSE(DR_REG_EBX, DR_REG_BX), DR_REG_AL,
                                  1, 0, OPSZ_1));
    test_instr_encode(dc, instr, 2);

    instr = INSTR_CREATE_maskmovq(dc, opnd_create_reg(DR_REG_MM0),
                                  opnd_create_reg(DR_REG_MM1));
    test_instr_encode(dc, instr, 3);
    instr = INSTR_PRED(
        instr_create_1dst_2src(
            dc, OP_maskmovq,
            opnd_create_far_base_disp(SEG_DS, IF_X64_ELSE(DR_REG_EDI, DR_REG_DI),
                                      DR_REG_NULL, 0, 0, OPSZ_8),
            opnd_create_reg(DR_REG_MM0), opnd_create_reg(DR_REG_MM1)),
        DR_PRED_COMPLEX);
    test_instr_encode(dc, instr, 4);

    instr = INSTR_CREATE_maskmovdqu(dc, opnd_create_reg(DR_REG_XMM0),
                                    opnd_create_reg(DR_REG_XMM1));
    test_instr_encode(dc, instr, 4);
    instr = INSTR_PRED(
        instr_create_1dst_2src(
            dc, OP_maskmovdqu,
            opnd_create_far_base_disp(SEG_DS, IF_X64_ELSE(DR_REG_EDI, DR_REG_DI),
                                      DR_REG_NULL, 0, 0, OPSZ_16),
            opnd_create_reg(DR_REG_XMM0), opnd_create_reg(DR_REG_XMM1)),
        DR_PRED_COMPLEX);
    test_instr_encode(dc, instr, 5);

    /* Test iretw, iretd, iretq (unlike most stack operation iretd (and lretd on AMD)
     * exist and are the default in 64-bit mode. As such, it has a different size/type
     * then most other stack operations).  Our instr_create routine should match stack
     * (iretq on 64-bit, iretd on 32-bit). See PR 191977. */
    instr = INSTR_CREATE_iret(dc);
#ifdef X64
    test_instr_encode_and_decode(dc, instr, 2, true /*src*/, 1, OPSZ_40, 40);
    ASSERT(buf[0] == 0x48); /* check for rex.w prefix */
#else
    test_instr_encode_and_decode(dc, instr, 1, true /*src*/, 1, OPSZ_12, 12);
#endif
    instr = instr_create_1dst_2src(
        dc, OP_iret, opnd_create_reg(DR_REG_XSP), opnd_create_reg(DR_REG_XSP),
        opnd_create_base_disp(DR_REG_XSP, DR_REG_NULL, 0, 0, OPSZ_12));
    test_instr_encode_and_decode(dc, instr, 1, true /*src*/, 1, OPSZ_12, 12);
    instr = instr_create_1dst_2src(
        dc, OP_iret, opnd_create_reg(DR_REG_XSP), opnd_create_reg(DR_REG_XSP),
        opnd_create_base_disp(DR_REG_XSP, DR_REG_NULL, 0, 0, OPSZ_6));
    test_instr_encode_and_decode(dc, instr, 2, true /*src*/, 1, OPSZ_6, 6);
    ASSERT(buf[0] == 0x66); /* check for data prefix */
}

/* PR 332254: test xchg vs nop */
static void
test_nop_xchg(void *dc)
{
    /*   0x0000000000671460  87 c0                xchg   %eax %eax -> %eax %eax
     *   0x0000000000671460  48 87 c0             xchg   %rax %rax -> %rax %rax
     *   0x0000000000671460  41 87 c0             xchg   %r8d %eax -> %r8d %eax
     *   0x0000000000671460  46 90                nop
     *   0x0000000000671460  4e 90                nop
     *   0x0000000000671460  41 90                xchg   %r8d %eax -> %r8d %eax
     */
    instr_t *instr;
    instr =
        INSTR_CREATE_xchg(dc, opnd_create_reg(DR_REG_EAX), opnd_create_reg(DR_REG_EAX));
    test_instr_encode(dc, instr, 2);
#ifdef X64
    /* we don't do the optimal "48 90" instead of "48 87 c0" */
    instr =
        INSTR_CREATE_xchg(dc, opnd_create_reg(DR_REG_RAX), opnd_create_reg(DR_REG_RAX));
    test_instr_encode(dc, instr, 3);
    /* we don't do the optimal "41 90" instead of "41 87 c0" */
    instr =
        INSTR_CREATE_xchg(dc, opnd_create_reg(DR_REG_R8D), opnd_create_reg(DR_REG_EAX));
    test_instr_encode(dc, instr, 3);
    /* ensure we treat as nop and NOT xchg if doesn't have rex.b */
    buf[0] = 0x46;
    buf[1] = 0x90;
    instr = instr_create(dc);
#    if VERBOSE
    disassemble_with_info(dc, buf, STDOUT, true, true);
#    endif
    decode(dc, buf, instr);
    ASSERT(instr_get_opcode(instr) == OP_nop);
    instr_destroy(dc, instr);
    buf[0] = 0x4e;
    buf[1] = 0x90;
    instr = instr_create(dc);
#    if VERBOSE
    disassemble_with_info(dc, buf, STDOUT, true, true);
#    endif
    decode(dc, buf, instr);
    ASSERT(instr_get_opcode(instr) == OP_nop);
    instr_destroy(dc, instr);
    buf[0] = 0x41;
    buf[1] = 0x90;
    instr = instr_create(dc);
#    if VERBOSE
    disassemble_with_info(dc, buf, STDOUT, true, true);
#    endif
    decode(dc, buf, instr);
    ASSERT(instr_get_opcode(instr) == OP_xchg);
    instr_destroy(dc, instr);
#endif
}

static void
test_hint_nops(void *dc)
{
    byte *pc;
    instr_t *instr;
    instr = instr_create(dc);

    /* ensure we treat as nop. */
    buf[0] = 0x0f;
    buf[1] = 0x18;
    /* nop [eax], and then ecx, edx, ebx */
    for (buf[2] = 0x38; buf[2] <= 0x3b; buf[2]++) {
        pc = decode(dc, buf, instr);
        ASSERT(instr_get_opcode(instr) == OP_nop_modrm);
        instr_reset(dc, instr);
    }

    /* other types of hintable nop [eax] */
    buf[2] = 0x00;
    for (buf[1] = 0x19; buf[1] <= 0x1f; buf[1]++) {
        /* Intel is using these encodings now for the MPX instructions bndldx and bndstx.
         */
        if (buf[1] == 0x1a || buf[1] == 0x1b)
            continue;
        pc = decode(dc, buf, instr);
        ASSERT(instr_get_opcode(instr) == OP_nop_modrm);
        instr_reset(dc, instr);
    }

    instr_destroy(dc, instr);
}

#ifdef X64
static void
test_x86_mode(void *dc)
{
    byte *pc, *end;
    instr_t *instr;

    /* create instr that looks different in x86 vs x64 */
    instr = INSTR_CREATE_add(dc, opnd_create_reg(DR_REG_RAX), OPND_CREATE_INT32(42));
    end = instr_encode(dc, instr, buf);
    ASSERT(end - buf < BUFFER_SIZE_ELEMENTS(buf));

    /* read back in */
    set_x86_mode(dc, false /*64-bit*/);
    instr_reset(dc, instr);
    pc = decode(dc, buf, instr);
    ASSERT(pc != NULL);
    ASSERT(instr_get_opcode(instr) == OP_add);

    /* now interpret as 32-bit where rex will be an inc */
    set_x86_mode(dc, true /*32-bit*/);
    instr_reset(dc, instr);
    pc = decode(dc, buf, instr);
    ASSERT(pc != NULL);
    ASSERT(instr_get_opcode(instr) == OP_dec);

    /* test i#352: in x86 mode, sysexit should have esp as dest, not rsp */
    set_x86_mode(dc, true /*32-bit*/);
    buf[0] = 0x0f;
    buf[1] = 0x35;
    instr_reset(dc, instr);
    pc = decode(dc, buf, instr);
    ASSERT(pc != NULL);
    ASSERT(instr_get_opcode(instr) == OP_sysexit);
    ASSERT(opnd_get_reg(instr_get_dst(instr, 0)) == DR_REG_ESP);

    instr_free(dc, instr);
    set_x86_mode(dc, false /*64-bit*/);
}

static void
test_x64_abs_addr(void *dc)
{
    /* 48 a1 ef be ad de ef be ad de    mov    0xdeadbeefdeadbeef -> %rax
     * 48 a3 ef be ad de ef be ad de    mov    %rax -> 0xdeadbeefdeadbeef
     */
    instr_t *instr;
    opnd_t abs_addr = opnd_create_abs_addr((void *)0xdeadbeefdeadbeef, OPSZ_8);

    /* movabs load */
    instr = INSTR_CREATE_mov_ld(dc, opnd_create_reg(DR_REG_RAX), abs_addr);
    test_instr_encode(dc, instr, 10); /* REX + op + 8 */

    /* movabs store */
    instr = INSTR_CREATE_mov_st(dc, abs_addr, opnd_create_reg(DR_REG_RAX));
    test_instr_encode(dc, instr, 10); /* REX + op + 8 */
}

static void
test_x64_inc(void *dc)
{
    /* i#842: inc/dec should not be encoded as 40-4f in x64 */
    instr_t *instr;

    instr = INSTR_CREATE_inc(dc, opnd_create_reg(DR_REG_EAX));
    test_instr_encode(dc, instr, 2);
}
#endif

static void
test_regs(void *dc)
{
    reg_id_t reg;
    /* Various subregs of xax to OPSZ_1. */
#ifdef X64
    reg = reg_resize_to_opsz(DR_REG_RAX, OPSZ_1);
    ASSERT(reg == DR_REG_AL);
#endif
    reg = reg_resize_to_opsz(DR_REG_EAX, OPSZ_1);
    ASSERT(reg == DR_REG_AL);
    reg = reg_resize_to_opsz(DR_REG_AX, OPSZ_1);
    ASSERT(reg == DR_REG_AL);
    reg = reg_resize_to_opsz(DR_REG_AH, OPSZ_1);
    ASSERT(reg == DR_REG_AL);
    reg = reg_resize_to_opsz(DR_REG_AL, OPSZ_1);
    ASSERT(reg == DR_REG_AL);

    /* xax to OPSZ_2 */
#ifdef X64
    reg = reg_resize_to_opsz(DR_REG_RAX, OPSZ_2);
    ASSERT(reg == DR_REG_AX);
#endif
    reg = reg_resize_to_opsz(DR_REG_EAX, OPSZ_2);
    ASSERT(reg == DR_REG_AX);
    reg = reg_resize_to_opsz(DR_REG_AX, OPSZ_2);
    ASSERT(reg == DR_REG_AX);
    reg = reg_resize_to_opsz(DR_REG_AH, OPSZ_2);
    ASSERT(reg == DR_REG_AX);
    reg = reg_resize_to_opsz(DR_REG_AL, OPSZ_2);
    ASSERT(reg == DR_REG_AX);

    /* xax to OPSZ_4 */
#ifdef X64
    reg = reg_resize_to_opsz(DR_REG_RAX, OPSZ_4);
    ASSERT(reg == DR_REG_EAX);
#endif
    reg = reg_resize_to_opsz(DR_REG_EAX, OPSZ_4);
    ASSERT(reg == DR_REG_EAX);
    reg = reg_resize_to_opsz(DR_REG_AX, OPSZ_4);
    ASSERT(reg == DR_REG_EAX);
    reg = reg_resize_to_opsz(DR_REG_AH, OPSZ_4);
    ASSERT(reg == DR_REG_EAX);
    reg = reg_resize_to_opsz(DR_REG_AL, OPSZ_4);
    ASSERT(reg == DR_REG_EAX);

#ifdef X64
    /* xax to OPSZ_8 */
    reg = reg_resize_to_opsz(DR_REG_RAX, OPSZ_8);
    ASSERT(reg == DR_REG_RAX);
    reg = reg_resize_to_opsz(DR_REG_EAX, OPSZ_8);
    ASSERT(reg == DR_REG_RAX);
    reg = reg_resize_to_opsz(DR_REG_AX, OPSZ_8);
    ASSERT(reg == DR_REG_RAX);
    reg = reg_resize_to_opsz(DR_REG_AH, OPSZ_8);
    ASSERT(reg == DR_REG_RAX);
    reg = reg_resize_to_opsz(DR_REG_AL, OPSZ_8);
    ASSERT(reg == DR_REG_RAX);
#endif

    /* Quick check of other regs. */
    reg = reg_resize_to_opsz(DR_REG_XBX, OPSZ_1);
    ASSERT(reg == DR_REG_BL);
    reg = reg_resize_to_opsz(DR_REG_XCX, OPSZ_1);
    ASSERT(reg == DR_REG_CL);
    reg = reg_resize_to_opsz(DR_REG_XDX, OPSZ_1);
    ASSERT(reg == DR_REG_DL);

    /* X64 only subregs, OPSZ_1. */
    reg = reg_resize_to_opsz(DR_REG_XDI, OPSZ_1);
    ASSERT(reg == IF_X64_ELSE(DR_REG_DIL, DR_REG_NULL));
    reg = reg_resize_to_opsz(DR_REG_XSI, OPSZ_1);
    ASSERT(reg == IF_X64_ELSE(DR_REG_SIL, DR_REG_NULL));
    reg = reg_resize_to_opsz(DR_REG_XSP, OPSZ_1);
    ASSERT(reg == IF_X64_ELSE(DR_REG_SPL, DR_REG_NULL));
    reg = reg_resize_to_opsz(DR_REG_XBP, OPSZ_1);
    ASSERT(reg == IF_X64_ELSE(DR_REG_BPL, DR_REG_NULL));

    /* X64 only subregs, OPSZ_2. */
    reg = reg_resize_to_opsz(DR_REG_XDI, OPSZ_2);
    ASSERT(reg == DR_REG_DI);
    reg = reg_resize_to_opsz(DR_REG_XSI, OPSZ_2);
    ASSERT(reg == DR_REG_SI);
    reg = reg_resize_to_opsz(DR_REG_XSP, OPSZ_2);
    ASSERT(reg == DR_REG_SP);
    reg = reg_resize_to_opsz(DR_REG_XBP, OPSZ_2);
    ASSERT(reg == DR_REG_BP);
}

static void
test_instr_opnds(void *dc)
{
    /* Verbose disasm looks like this:
     * 32-bit:
     *   0x080f1ae0  ff 25 e7 1a 0f 08    jmp    0x080f1ae7
     *   0x080f1ae6  b8 ef be ad de       mov    $0xdeadbeef -> %eax
     *   0x080f1ae0  a0 e6 1a 0f 08       mov    0x080f1ae6 -> %al
     *   0x080f1ae5  b8 ef be ad de       mov    $0xdeadbeef -> %eax
     * 64-bit:
     *   0x00000000006b8de0  ff 25 02 00 00 00    jmp    <rel> 0x00000000006b8de8
     *   0x00000000006b8de6  48 b8 ef be ad de 00 mov    $0x00000000deadbeef -> %rax
     *                       00 00 00
     *   0x00000000006b8de0  8a 05 02 00 00 00    mov    <rel> 0x00000000006b8de8 -> %al
     *   0x00000000006b8de6  48 b8 ef be ad de 00 mov    $0x00000000deadbeef -> %rax
     *                       00 00 00
     */
    instrlist_t *ilist;
    instr_t *tgt, *instr;
    byte *pc;
    short disp;

    ilist = instrlist_create(dc);

    /* test mem instr as ind jmp target */
    tgt = INSTR_CREATE_mov_imm(dc, opnd_create_reg(DR_REG_XAX),
                               opnd_create_immed_int(0xdeadbeef, OPSZ_PTR));
    /* skip rex+opcode */
    disp = IF_X64_ELSE(2, 1);
    instrlist_append(
        ilist, INSTR_CREATE_jmp_ind(dc, opnd_create_mem_instr(tgt, disp, OPSZ_PTR)));
    instrlist_append(ilist, tgt);
    pc = instrlist_encode(dc, ilist, buf, true /*instr targets*/);
    ASSERT(pc != NULL);
    instrlist_clear(dc, ilist);
#if VERBOSE
    pc = disassemble_with_info(dc, buf, STDOUT, true, true);
    pc = disassemble_with_info(dc, pc, STDOUT, true, true);
#endif
    pc = buf;
    instr = instr_create(dc);
    pc = decode(dc, pc, instr);
    ASSERT(pc != NULL);
    ASSERT(instr_get_opcode(instr) == OP_jmp_ind);
#ifdef X64
    ASSERT(opnd_is_rel_addr(instr_get_src(instr, 0)));
    ASSERT(opnd_get_addr(instr_get_src(instr, 0)) == pc + disp);
#else
    ASSERT(opnd_is_base_disp(instr_get_src(instr, 0)));
    ASSERT(opnd_get_base(instr_get_src(instr, 0)) == DR_REG_NULL);
    ASSERT(opnd_get_index(instr_get_src(instr, 0)) == DR_REG_NULL);
    ASSERT(opnd_get_disp(instr_get_src(instr, 0)) == (ptr_int_t)pc + disp);
#endif

    /* test mem instr as TYPE_O */
    tgt = INSTR_CREATE_mov_imm(dc, opnd_create_reg(DR_REG_XAX),
                               opnd_create_immed_int(0xdeadbeef, OPSZ_PTR));
    /* skip rex+opcode */
    disp = IF_X64_ELSE(2, 1);
    instrlist_append(ilist,
                     INSTR_CREATE_mov_ld(dc, opnd_create_reg(DR_REG_AL),
                                         opnd_create_mem_instr(tgt, disp, OPSZ_1)));
    instrlist_append(ilist, tgt);
    pc = instrlist_encode(dc, ilist, buf, true /*instr targets*/);
    ASSERT(pc != NULL);
    instrlist_clear(dc, ilist);
#if VERBOSE
    pc = disassemble_with_info(dc, buf, STDOUT, true, true);
    pc = disassemble_with_info(dc, pc, STDOUT, true, true);
#endif
    pc = buf;
    instr_reset(dc, instr);
    pc = decode(dc, pc, instr);
    ASSERT(pc != NULL);
    ASSERT(instr_get_opcode(instr) == OP_mov_ld);
#ifdef X64
    ASSERT(opnd_is_rel_addr(instr_get_src(instr, 0)));
    ASSERT(opnd_get_addr(instr_get_src(instr, 0)) == pc + disp);
#else
    ASSERT(opnd_is_base_disp(instr_get_src(instr, 0)));
    ASSERT(opnd_get_base(instr_get_src(instr, 0)) == DR_REG_NULL);
    ASSERT(opnd_get_index(instr_get_src(instr, 0)) == DR_REG_NULL);
    ASSERT(opnd_get_disp(instr_get_src(instr, 0)) == (ptr_int_t)pc + disp);
#endif

    instr_free(dc, instr);
    instrlist_destroy(dc, ilist);
}

static void
test_strict_invalid(void *dc)
{
    instr_t instr;
    byte *pc;
    const byte buf1[] = { 0xf2, 0x0f, 0xd8, 0xe9 }; /* psubusb w/ invalid prefix */
    const byte buf2[] = { 0xc5, 0x84, 0x41, 0xd0 }; /* kandw k0, (invalid), k2 */

    instr_init(dc, &instr);

    /* The instr should be valid by default and invalid if decode_strict */
    pc = decode(dc, (byte *)buf1, &instr);
    ASSERT(pc != NULL);

    disassemble_set_syntax(DR_DISASM_STRICT_INVALID);
    instr_reset(dc, &instr);
    pc = decode(dc, (byte *)buf1, &instr);
    ASSERT(pc == NULL);

#ifdef X64
    /* The instruction should always be invalid. In 32-bit mode, the instruction will
     * decode as lds, because the very bits[7:6] of the second byte of the 2-byte VEX
     * form are used to differentiate lds from the VEX prefix 0xc5.
     */
    instr_reset(dc, &instr);
    pc = decode(dc, (byte *)buf2, &instr);
    ASSERT(pc == NULL);
#endif

    instr_free(dc, &instr);
}

static void
test_tsx(void *dc)
{
    /* Test the xacquire and xrelease prefixes */
    byte *pc;
    const byte b1[] = { 0xf3, 0xa3, 0x9a, 0x7a, 0x21, 0x02, 0xfa, 0x8c, 0xec, 0xa3 };
    const byte b2[] = { 0xf3, 0x89, 0x39 };
    const byte b3[] = { 0xf2, 0x89, 0x39 };
    const byte b4[] = { 0xf2, 0xf0, 0x00, 0x00 };
    char buf[512];
    int len;

    pc = disassemble_to_buffer(dc, (byte *)b1, (byte *)b1, false /*no pc*/,
                               false /*no bytes*/, buf, BUFFER_SIZE_ELEMENTS(buf), &len);
    ASSERT(pc != NULL);
    ASSERT(strcmp(buf,
                  IF_X64_ELSE("mov    %eax -> 0xa3ec8cfa02217a9a[4byte]\n",
                              "mov    %eax -> 0x02217a9a[4byte]\n")) == 0);

    pc = disassemble_to_buffer(dc, (byte *)b2, (byte *)b2, false /*no pc*/,
                               false /*no bytes*/, buf, BUFFER_SIZE_ELEMENTS(buf), &len);
    ASSERT(pc != NULL);
    ASSERT(strcmp(buf,
                  IF_X64_ELSE("mov    %edi -> (%rcx)[4byte]\n",
                              "mov    %edi -> (%ecx)[4byte]\n")) == 0);

    pc = disassemble_to_buffer(dc, (byte *)b3, (byte *)b3, false /*no pc*/,
                               false /*no bytes*/, buf, BUFFER_SIZE_ELEMENTS(buf), &len);
    ASSERT(pc != NULL);
    ASSERT(strcmp(buf,
                  IF_X64_ELSE("xacquire mov    %edi -> (%rcx)[4byte]\n",
                              "xacquire mov    %edi -> (%ecx)[4byte]\n")) == 0);

    pc = disassemble_to_buffer(dc, (byte *)b4, (byte *)b4, false /*no pc*/,
                               false /*no bytes*/, buf, BUFFER_SIZE_ELEMENTS(buf), &len);
    ASSERT(pc != NULL);
    ASSERT(strcmp(buf,
                  IF_X64_ELSE(
                      "xacquire lock add    %al (%rax)[1byte] -> (%rax)[1byte]\n",
                      "xacquire lock add    %al (%eax)[1byte] -> (%eax)[1byte]\n")) == 0);
}

static void
test_vsib_helper(void *dc, dr_mcontext_t *mc, instr_t *instr, reg_t base, int index_idx,
                 int scale, int disp, int count, opnd_size_t index_sz, bool is_evex,
                 bool expect_write)
{
    uint memopidx, memoppos;
    app_pc addr;
    bool write;
    for (memopidx = 0;
         instr_compute_address_ex_pos(instr, mc, memopidx, &addr, &write, &memoppos);
         memopidx++) {
        /* should be a read from 1st source */
        ptr_int_t index =
            ((index_sz == OPSZ_4) ?
                                  /* this only works w/ the mask all enabled */
                 (mc->simd[index_idx].u32[memopidx])
                                  :
#ifdef X64
                                  (mc->simd[index_idx].u64[memopidx])
#else
                                  ((((int64)mc->simd[index_idx].u32[memopidx * 2 + 1])
                                    << 32) |
                                   mc->simd[index_idx].u32[memopidx * 2])
#endif
            );
        ASSERT(write == expect_write);
        ASSERT((is_evex && !write && memoppos == 1) ||
               (is_evex && write && memoppos == 0) || memoppos == 0);
        ASSERT((ptr_int_t)addr == base + disp + scale * index);
    }
    ASSERT(memopidx == count);
}

static void
test_vsib(void *dc)
{
    dr_mcontext_t mc;
    instr_t *instr;

    /* Test VSIB addressing */
    byte *pc;
    const byte b1[] = { 0xc4, 0xe2, 0xe9, 0x90, 0x24, 0x42 };
    /* Invalid b/c modrm doesn't ask for SIB */
    const byte b2[] = { 0xc4, 0xe2, 0xe9, 0x90, 0x00 };
    char dbuf[512];
    int len;

    pc =
        disassemble_to_buffer(dc, (byte *)b1, (byte *)b1, false /*no pc*/,
                              false /*no bytes*/, dbuf, BUFFER_SIZE_ELEMENTS(dbuf), &len);
    ASSERT(pc != NULL);
    ASSERT(strcmp(dbuf,
                  IF_X64_ELSE(
                      "vpgatherdq (%rdx,%xmm0,2)[8byte] %xmm2 -> %xmm4 %xmm2\n",
                      "vpgatherdq (%edx,%xmm0,2)[8byte] %xmm2 -> %xmm4 %xmm2\n")) == 0);

    pc =
        disassemble_to_buffer(dc, (byte *)b2, (byte *)b2, false /*no pc*/,
                              false /*no bytes*/, dbuf, BUFFER_SIZE_ELEMENTS(dbuf), &len);
    ASSERT(pc == NULL);

    /* AVX VEX opcodes */

    /* Test mem addr emulation */
    mc.size = sizeof(mc);
    mc.flags = DR_MC_ALL;
    mc.xcx = 0x42;
    mc.simd[1].u32[0] = 0x11111111;
    mc.simd[1].u32[1] = 0x22222222;
    mc.simd[1].u32[2] = 0x33333333;
    mc.simd[1].u32[3] = 0x44444444;
    mc.simd[1].u32[4] = 0x12345678;
    mc.simd[1].u32[5] = 0x87654321;
    mc.simd[1].u32[6] = 0xabababab;
    mc.simd[1].u32[7] = 0xcdcdcdcd;
    /* mask */
    mc.simd[2].u32[0] = 0xf1111111;
    mc.simd[2].u32[1] = 0xf2222222;
    mc.simd[2].u32[2] = 0xf3333333;
    mc.simd[2].u32[3] = 0xf4444444;
    mc.simd[2].u32[4] = 0xf5444444;
    mc.simd[2].u32[5] = 0xf6444444;
    mc.simd[2].u32[6] = 0xf7444444;
    mc.simd[2].u32[7] = 0xf8444444;

    /* test index size 4 and mem size 8 */
    instr = INSTR_CREATE_vgatherdpd(
        dc, opnd_create_reg(DR_REG_XMM0),
        opnd_create_base_disp(DR_REG_XCX, DR_REG_XMM1, 2, 0x12, OPSZ_8),
        opnd_create_reg(DR_REG_XMM2));
    test_vsib_helper(dc, &mc, instr, mc.xcx, 1, 2, 0x12, 2, OPSZ_4, false /* !evex */,
                     false /* !expect_write */);
    instr_destroy(dc, instr);

    /* test index size 8 and mem size 4 */
    instr = INSTR_CREATE_vgatherqpd(
        dc, opnd_create_reg(DR_REG_XMM0),
        opnd_create_base_disp(DR_REG_XCX, DR_REG_XMM1, 2, 0x12, OPSZ_8),
        opnd_create_reg(DR_REG_XMM2));
    test_vsib_helper(dc, &mc, instr, mc.xcx, 1, 2, 0x12, 2, OPSZ_8, false /* !evex */,
                     false /* !expect_write */);
    instr_destroy(dc, instr);

    /* test index size 4 and mem size 4 */
    instr = INSTR_CREATE_vgatherdps(
        dc, opnd_create_reg(DR_REG_XMM0),
        opnd_create_base_disp(DR_REG_XCX, DR_REG_XMM1, 2, 0x12, OPSZ_4),
        opnd_create_reg(DR_REG_XMM2));
    test_vsib_helper(dc, &mc, instr, mc.xcx, 1, 2, 0x12, 4, OPSZ_4, false /* !evex */,
                     false /* !expect_write */);
    instr_destroy(dc, instr);

    /* test index size 8 and mem size 4 */
    instr = INSTR_CREATE_vgatherqps(
        dc, opnd_create_reg(DR_REG_XMM0),
        opnd_create_base_disp(DR_REG_XCX, DR_REG_XMM1, 2, 0x12, OPSZ_4),
        opnd_create_reg(DR_REG_XMM2));
    test_vsib_helper(dc, &mc, instr, mc.xcx, 1, 2, 0x12, 2, OPSZ_8, false /* !evex */,
                     false /* !expect_write */);
    instr_destroy(dc, instr);

    /* test 256-byte */
    instr = INSTR_CREATE_vgatherdps(
        dc, opnd_create_reg(DR_REG_YMM0),
        opnd_create_base_disp(DR_REG_XCX, DR_REG_YMM1, 2, 0x12, OPSZ_4),
        opnd_create_reg(DR_REG_YMM2));
    test_vsib_helper(dc, &mc, instr, mc.xcx, 1, 2, 0x12, 8, OPSZ_4, false /* !evex */,
                     false /* !expect_write */);
    instr_destroy(dc, instr);

    /* test mask not selecting things -- in the middle complicates
     * our helper checks so we just do the ends
     */
    mc.simd[2].u32[0] = 0x71111111;
    mc.simd[2].u32[1] = 0x32222222;
    mc.simd[2].u32[2] = 0x13333333;
    mc.simd[2].u32[3] = 0x04444444;
    mc.simd[2].u32[4] = 0x65444444;
    mc.simd[2].u32[5] = 0x56444444;
    mc.simd[2].u32[6] = 0x47444444;
    mc.simd[2].u32[7] = 0x28444444;
    instr = INSTR_CREATE_vgatherdps(
        dc, opnd_create_reg(DR_REG_YMM0),
        opnd_create_base_disp(DR_REG_XCX, DR_REG_YMM1, 2, 0x12, OPSZ_4),
        opnd_create_reg(DR_REG_YMM2));
    test_vsib_helper(dc, &mc, instr, mc.xcx, 1, 2, 0x12, 0 /*nothing*/, OPSZ_4,
                     false /* !evex */, false /* !expect_write */);
    instr_destroy(dc, instr);

    /* AVX-512 EVEX opcodes */

    /* evex mask */
    mc.opmask[0] = 0xffff;

    /* test index size 4 and mem size 8 */
    instr = INSTR_CREATE_vgatherdpd_mask(
        dc, opnd_create_reg(DR_REG_XMM0), opnd_create_reg(DR_REG_K0),
        opnd_create_base_disp(DR_REG_XCX, DR_REG_XMM1, 2, 0x12, OPSZ_8));
    test_vsib_helper(dc, &mc, instr, mc.xcx, 1, 2, 0x12, 2, OPSZ_4, true /* evex */,
                     false /* !expect_write */);
    instr_destroy(dc, instr);

    instr = INSTR_CREATE_vscatterdpd_mask(
        dc, opnd_create_base_disp(DR_REG_XCX, DR_REG_XMM1, 2, 0x12, OPSZ_8),
        opnd_create_reg(DR_REG_K0), opnd_create_reg(DR_REG_XMM0));
    test_vsib_helper(dc, &mc, instr, mc.xcx, 1, 2, 0x12, 2, OPSZ_4, true /* evex */,
                     true /* expect_write */);
    instr_destroy(dc, instr);

    /* test index size 8 and mem size 4 */
    instr = INSTR_CREATE_vgatherqpd_mask(
        dc, opnd_create_reg(DR_REG_XMM0), opnd_create_reg(DR_REG_K0),
        opnd_create_base_disp(DR_REG_XCX, DR_REG_XMM1, 2, 0x12, OPSZ_8));
    test_vsib_helper(dc, &mc, instr, mc.xcx, 1, 2, 0x12, 2, OPSZ_8, true /* evex */,
                     false /* !expect_write */);
    instr_destroy(dc, instr);

    instr = INSTR_CREATE_vscatterqpd_mask(
        dc, opnd_create_base_disp(DR_REG_XCX, DR_REG_XMM1, 2, 0x12, OPSZ_8),
        opnd_create_reg(DR_REG_K0), opnd_create_reg(DR_REG_XMM0));
    test_vsib_helper(dc, &mc, instr, mc.xcx, 1, 2, 0x12, 2, OPSZ_8, true /* evex */,
                     true /* expect_write */);
    instr_destroy(dc, instr);

    /* test index size 4 and mem size 4 */
    instr = INSTR_CREATE_vgatherdps_mask(
        dc, opnd_create_reg(DR_REG_XMM0), opnd_create_reg(DR_REG_K0),
        opnd_create_base_disp(DR_REG_XCX, DR_REG_XMM1, 2, 0x12, OPSZ_4));
    test_vsib_helper(dc, &mc, instr, mc.xcx, 1, 2, 0x12, 4, OPSZ_4, true /* evex */,
                     false /* !expect_write */);
    instr_destroy(dc, instr);

    instr = INSTR_CREATE_vscatterdps_mask(
        dc, opnd_create_base_disp(DR_REG_XCX, DR_REG_XMM1, 2, 0x12, OPSZ_4),
        opnd_create_reg(DR_REG_K0), opnd_create_reg(DR_REG_XMM0));
    test_vsib_helper(dc, &mc, instr, mc.xcx, 1, 2, 0x12, 4, OPSZ_4, true /* evex */,
                     true /* expect_write */);
    instr_destroy(dc, instr);

    /* test index size 8 and mem size 4 */
    instr = INSTR_CREATE_vgatherqps_mask(
        dc, opnd_create_reg(DR_REG_XMM0), opnd_create_reg(DR_REG_K0),
        opnd_create_base_disp(DR_REG_XCX, DR_REG_XMM1, 2, 0x12, OPSZ_4));
    test_vsib_helper(dc, &mc, instr, mc.xcx, 1, 2, 0x12, 2, OPSZ_8, true /* evex */,
                     false /* !expect_write */);
    instr_destroy(dc, instr);

    instr = INSTR_CREATE_vscatterqps_mask(
        dc, opnd_create_base_disp(DR_REG_XCX, DR_REG_XMM1, 2, 0x12, OPSZ_4),
        opnd_create_reg(DR_REG_K0), opnd_create_reg(DR_REG_XMM0));
    test_vsib_helper(dc, &mc, instr, mc.xcx, 1, 2, 0x12, 2, OPSZ_8, true /* evex */,
                     true /* expect_write */);
    instr_destroy(dc, instr);

    /* test 256-bit */
    instr = INSTR_CREATE_vgatherdps_mask(
        dc, opnd_create_reg(DR_REG_YMM0), opnd_create_reg(DR_REG_K0),
        opnd_create_base_disp(DR_REG_XCX, DR_REG_YMM1, 2, 0x12, OPSZ_4));
    test_vsib_helper(dc, &mc, instr, mc.xcx, 1, 2, 0x12, 8, OPSZ_4, true /* evex */,
                     false /* !expect_write */);
    instr_destroy(dc, instr);

    instr = INSTR_CREATE_vscatterdps_mask(
        dc, opnd_create_base_disp(DR_REG_XCX, DR_REG_YMM1, 2, 0x12, OPSZ_4),
        opnd_create_reg(DR_REG_K0), opnd_create_reg(DR_REG_YMM0));
    test_vsib_helper(dc, &mc, instr, mc.xcx, 1, 2, 0x12, 8, OPSZ_4, true /* evex */,
                     true /* expect_write */);
    instr_destroy(dc, instr);

    /* test 512-bit */
    instr = INSTR_CREATE_vgatherdps_mask(
        dc, opnd_create_reg(DR_REG_ZMM0), opnd_create_reg(DR_REG_K0),
        opnd_create_base_disp(DR_REG_XCX, DR_REG_ZMM1, 2, 0x12, OPSZ_4));
    test_vsib_helper(dc, &mc, instr, mc.xcx, 1, 2, 0x12, 16, OPSZ_4, true /* evex */,
                     false /* !expect_write */);
    instr_destroy(dc, instr);

    instr = INSTR_CREATE_vscatterdps_mask(
        dc, opnd_create_base_disp(DR_REG_XCX, DR_REG_ZMM1, 2, 0x12, OPSZ_4),
        opnd_create_reg(DR_REG_K0), opnd_create_reg(DR_REG_ZMM0));
    test_vsib_helper(dc, &mc, instr, mc.xcx, 1, 2, 0x12, 16, OPSZ_4, true /* evex */,
                     true /* expect_write */);
    instr_destroy(dc, instr);

    /* test mask not selecting things -- in the middle complicates
     * our helper checks so we just do the ends
     */
    mc.opmask[0] = 0x0;

    instr = INSTR_CREATE_vgatherdps_mask(
        dc, opnd_create_reg(DR_REG_YMM0), opnd_create_reg(DR_REG_K0),
        opnd_create_base_disp(DR_REG_XCX, DR_REG_YMM1, 2, 0x12, OPSZ_4));
    test_vsib_helper(dc, &mc, instr, mc.xcx, 1, 2, 0x12, 0 /*nothing*/, OPSZ_4,
                     true /* evex */, false /* !expect_write */);
    instr_destroy(dc, instr);

    instr = INSTR_CREATE_vscatterdps_mask(
        dc, opnd_create_base_disp(DR_REG_XCX, DR_REG_YMM1, 2, 0x12, OPSZ_4),
        opnd_create_reg(DR_REG_K0), opnd_create_reg(DR_REG_YMM0));
    test_vsib_helper(dc, &mc, instr, mc.xcx, 1, 2, 0x12, 0 /*nothing*/, OPSZ_4,
                     true /* evex */, true /* expect_write */);
    instr_destroy(dc, instr);

    /* Test invalid k0 mask with scatter/gather opcodes. */
    const byte b_scattergatherinv[] = { /* vpscatterdd %xmm0,(%rax,%xmm1,2){%k0} */
                                        0x62, 0xf2, 0x7d, 0x08, 0xa0, 0x04, 0x48,
                                        /* vpscatterdq %xmm0,(%rax,%xmm1,2){%k0} */
                                        0x62, 0xf2, 0xfd, 0x08, 0xa0, 0x04, 0x48,
                                        /* vpscatterqd %xmm0,(%rax,%xmm1,2){%k0} */
                                        0x62, 0xf2, 0x7d, 0x08, 0xa1, 0x04, 0x48,
                                        /* vpscatterqq %xmm0,(%rax,%xmm1,2){%k0} */
                                        0x62, 0xf2, 0xfd, 0x08, 0xa1, 0x04, 0x48,
                                        /* vscatterdps %xmm0,(%rax,%xmm1,2){%k0} */
                                        0x62, 0xf2, 0x7d, 0x08, 0xa2, 0x04, 0x48,
                                        /* vscatterdpd %xmm0,(%rax,%xmm1,2){%k0} */
                                        0x62, 0xf2, 0xfd, 0x08, 0xa2, 0x04, 0x48,
                                        /* vscatterqps %xmm0,(%rax,%xmm1,2){%k0} */
                                        0x62, 0xf2, 0x7d, 0x08, 0xa3, 0x04, 0x48,
                                        /* vscatterqpd %xmm0,(%rax,%xmm1,2){%k0} */
                                        0x62, 0xf2, 0xfd, 0x08, 0xa3, 0x04, 0x48,
                                        /* vpgatherdd (%rax,%xmm1,2),%xmm0{%k0} */
                                        0x62, 0xf2, 0x7d, 0x08, 0x90, 0x04, 0x48,
                                        /* vpgatherdq (%rax,%xmm1,2),%xmm0{%k0} */
                                        0x62, 0xf2, 0xfd, 0x08, 0x90, 0x04, 0x48,
                                        /* vpgatherqd (%rax,%xmm1,2),%xmm0{%k0} */
                                        0x62, 0xf2, 0x7d, 0x08, 0x91, 0x04, 0x48,
                                        /* vpgatherqq (%rax,%xmm1,2),%xmm0{%k0} */
                                        0x62, 0xf2, 0xfd, 0x08, 0x91, 0x04, 0x48,
                                        /* vgatherdps (%rax,%xmm1,2),%xmm0{%k0} */
                                        0x62, 0xf2, 0x7d, 0x08, 0x92, 0x04, 0x48,
                                        /* vgatherdpd (%rax,%xmm1,2),%xmm0{%k0} */
                                        0x62, 0xf2, 0xfd, 0x08, 0x92, 0x04, 0x48,
                                        /* vgatherqps (%rax,%xmm1,2),%xmm0{%k0} */
                                        0x62, 0xf2, 0x7d, 0x08, 0x93, 0x04, 0x48,
                                        /* vgatherqpd (%rax,%xmm1,2),%xmm0{%k0} */
                                        0x62, 0xf2, 0xfd, 0x08, 0x93, 0x04, 0x48
    };
    instr_t invinstr;
    instr_init(dc, &invinstr);
    for (int i = 0; i < sizeof(b_scattergatherinv); i += 7) {
        pc = decode(dc, (byte *)&b_scattergatherinv[i], &invinstr);
        ASSERT(pc == NULL);
    }
}

static void
test_disasm_sizes(void *dc)
{
    byte *pc;
    char dbuf[512];
    int len;

    {
        const byte b1[] = { 0xac };
        const byte b2[] = { 0xad };
        pc = disassemble_to_buffer(dc, (byte *)b1, (byte *)b1, false, false, dbuf,
                                   BUFFER_SIZE_ELEMENTS(dbuf), &len);
        ASSERT(pc != NULL);
        ASSERT(strcmp(dbuf,
                      IF_X64_ELSE("lods   %ds:(%rsi)[1byte] %rsi -> %al %rsi\n",
                                  "lods   %ds:(%esi)[1byte] %esi -> %al %esi\n")) == 0);
        pc = disassemble_to_buffer(dc, (byte *)b2, (byte *)b2, false, false, dbuf,
                                   BUFFER_SIZE_ELEMENTS(dbuf), &len);
        ASSERT(pc != NULL);
        ASSERT(strcmp(dbuf,
                      IF_X64_ELSE("lods   %ds:(%rsi)[4byte] %rsi -> %eax %rsi\n",
                                  "lods   %ds:(%esi)[4byte] %esi -> %eax %esi\n")) == 0);
    }
#ifdef X64
    {
        const byte b3[] = { 0x48, 0xad };
        pc = disassemble_to_buffer(dc, (byte *)b3, (byte *)b3, false, false, dbuf,
                                   BUFFER_SIZE_ELEMENTS(dbuf), &len);
        ASSERT(pc != NULL);
        ASSERT(strcmp(dbuf, "lods   %ds:(%rsi)[8byte] %rsi -> %rax %rsi\n") == 0);
    }
#endif

#ifdef X64
    {
        const byte b1[] = { 0xc7, 0x80, 0x90, 0xe4, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00 };
        const byte b2[] = { 0x48, 0xc7, 0x80, 0x90, 0xe4, 0xff,
                            0xff, 0x00, 0x00, 0x00, 0x00 };
        pc = disassemble_to_buffer(dc, (byte *)b1, (byte *)b1, false, false, dbuf,
                                   BUFFER_SIZE_ELEMENTS(dbuf), &len);
        ASSERT(pc != NULL);
        ASSERT(strcmp(dbuf, "mov    $0x00000000 -> 0xffffe490(%rax)[4byte]\n") == 0);
        pc = disassemble_to_buffer(dc, (byte *)b2, (byte *)b2, false, false, dbuf,
                                   BUFFER_SIZE_ELEMENTS(dbuf), &len);
        ASSERT(pc != NULL);
        ASSERT(strcmp(dbuf, "mov    $0x0000000000000000 -> 0xffffe490(%rax)[8byte]\n") ==
               0);
    }
#endif
}

static void
test_predication(void *dc)
{
    byte *pc;
    uint usage;
    instr_t *instr = INSTR_CREATE_vmaskmovps(
        dc, opnd_create_reg(DR_REG_XMM0), opnd_create_reg(DR_REG_XMM1), MEMARG(OPSZ_16));
    ASSERT(instr_reads_from_reg(instr, DR_REG_XMM1, DR_QUERY_DEFAULT));
    ASSERT(instr_reads_from_reg(instr, DR_REG_XMM1, DR_QUERY_INCLUDE_ALL));
    ASSERT(instr_reads_from_reg(instr, DR_REG_XMM1, DR_QUERY_INCLUDE_COND_DSTS));
    ASSERT(instr_reads_from_reg(instr, DR_REG_XMM1, 0));
    ASSERT(!instr_writes_to_reg(instr, DR_REG_XMM0, DR_QUERY_DEFAULT));
    ASSERT(instr_writes_to_reg(instr, DR_REG_XMM0, DR_QUERY_INCLUDE_ALL));
    ASSERT(instr_writes_to_reg(instr, DR_REG_XMM0, DR_QUERY_INCLUDE_COND_DSTS));
    ASSERT(!instr_writes_to_reg(instr, DR_REG_XMM0, 0));
    pc = instr_encode(dc, instr, buf);
    ASSERT(pc != NULL);
    instr_reset(dc, instr);
    decode(dc, buf, instr);
    ASSERT(instr_reads_from_reg(instr, DR_REG_XMM1, DR_QUERY_DEFAULT));
    ASSERT(instr_reads_from_reg(instr, DR_REG_XMM1, DR_QUERY_INCLUDE_ALL));
    ASSERT(instr_reads_from_reg(instr, DR_REG_XMM1, DR_QUERY_INCLUDE_COND_DSTS));
    ASSERT(instr_reads_from_reg(instr, DR_REG_XMM1, 0));
    ASSERT(!instr_writes_to_reg(instr, DR_REG_XMM0, DR_QUERY_DEFAULT));
    ASSERT(instr_writes_to_reg(instr, DR_REG_XMM0, DR_QUERY_INCLUDE_ALL));
    ASSERT(instr_writes_to_reg(instr, DR_REG_XMM0, DR_QUERY_INCLUDE_COND_DSTS));
    ASSERT(!instr_writes_to_reg(instr, DR_REG_XMM0, 0));

    instr_reset(dc, instr);
    instr = INSTR_CREATE_cmovcc(dc, OP_cmovnle, opnd_create_reg(DR_REG_EAX),
                                opnd_create_reg(DR_REG_ECX));
    ASSERT(instr_reads_from_reg(instr, DR_REG_ECX, DR_QUERY_DEFAULT));
    ASSERT(instr_reads_from_reg(instr, DR_REG_ECX, DR_QUERY_INCLUDE_ALL));
    ASSERT(!instr_reads_from_reg(instr, DR_REG_ECX, DR_QUERY_INCLUDE_COND_DSTS));
    ASSERT(!instr_reads_from_reg(instr, DR_REG_ECX, 0));
    ASSERT(!instr_writes_to_reg(instr, DR_REG_EAX, DR_QUERY_DEFAULT));
    ASSERT(instr_writes_to_reg(instr, DR_REG_EAX, DR_QUERY_INCLUDE_ALL));
    ASSERT(instr_writes_to_reg(instr, DR_REG_EAX, DR_QUERY_INCLUDE_COND_DSTS));
    ASSERT(!instr_writes_to_reg(instr, DR_REG_EAX, 0));
    pc = instr_encode(dc, instr, buf);
    ASSERT(pc != NULL);
    instr_reset(dc, instr);
    decode(dc, buf, instr);
    ASSERT(instr_reads_from_reg(instr, DR_REG_ECX, DR_QUERY_DEFAULT));
    ASSERT(instr_reads_from_reg(instr, DR_REG_ECX, DR_QUERY_INCLUDE_ALL));
    ASSERT(!instr_reads_from_reg(instr, DR_REG_ECX, DR_QUERY_INCLUDE_COND_DSTS));
    ASSERT(!instr_reads_from_reg(instr, DR_REG_ECX, 0));
    ASSERT(!instr_writes_to_reg(instr, DR_REG_EAX, DR_QUERY_DEFAULT));
    ASSERT(instr_writes_to_reg(instr, DR_REG_EAX, DR_QUERY_INCLUDE_ALL));
    ASSERT(instr_writes_to_reg(instr, DR_REG_EAX, DR_QUERY_INCLUDE_COND_DSTS));
    ASSERT(!instr_writes_to_reg(instr, DR_REG_EAX, 0));

    /* bsf always writes to eflags */
    instr_reset(dc, instr);
    instr =
        INSTR_CREATE_bsf(dc, opnd_create_reg(DR_REG_EAX), opnd_create_reg(DR_REG_ECX));
    ASSERT(TESTALL(EFLAGS_WRITE_6, instr_get_eflags(instr, DR_QUERY_DEFAULT)));
    ASSERT(TESTALL(EFLAGS_WRITE_6, instr_get_eflags(instr, DR_QUERY_INCLUDE_ALL)));
    ASSERT(TESTALL(EFLAGS_WRITE_6, instr_get_eflags(instr, DR_QUERY_INCLUDE_COND_DSTS)));
    ASSERT(TESTALL(EFLAGS_WRITE_6, instr_get_eflags(instr, 0)));
    pc = instr_encode(dc, instr, buf);
    ASSERT(pc != NULL);
    ASSERT(decode_eflags_usage(dc, buf, &usage, DR_QUERY_DEFAULT) != NULL &&
           TESTALL(EFLAGS_WRITE_6, usage));
    ASSERT(decode_eflags_usage(dc, buf, &usage, DR_QUERY_INCLUDE_ALL) != NULL &&
           TESTALL(EFLAGS_WRITE_6, usage));
    ASSERT(decode_eflags_usage(dc, buf, &usage, DR_QUERY_INCLUDE_COND_DSTS) != NULL &&
           TESTALL(EFLAGS_WRITE_6, usage));
    ASSERT(decode_eflags_usage(dc, buf, &usage, 0) != NULL &&
           TESTALL(EFLAGS_WRITE_6, usage));
    instr_reset(dc, instr);
    decode(dc, buf, instr);
    ASSERT(TESTALL(EFLAGS_WRITE_6, instr_get_eflags(instr, DR_QUERY_DEFAULT)));
    ASSERT(TESTALL(EFLAGS_WRITE_6, instr_get_eflags(instr, DR_QUERY_INCLUDE_ALL)));
    ASSERT(TESTALL(EFLAGS_WRITE_6, instr_get_eflags(instr, DR_QUERY_INCLUDE_COND_DSTS)));
    ASSERT(TESTALL(EFLAGS_WRITE_6, instr_get_eflags(instr, 0)));

    instr_destroy(dc, instr);
}

static void
test_xinst_create(void *dc)
{
    byte *pc;
    reg_id_t reg = DR_REG_XDX;
    instr_t *ins1, *ins2;
    /* load 1 byte zextend */
    ins1 = XINST_CREATE_load_1byte_zext4(
        dc, opnd_create_reg(reg_resize_to_opsz(reg, OPSZ_4)), MEMARG(OPSZ_1));
    pc = instr_encode(dc, ins1, buf);
    ASSERT(pc != NULL);
    ins2 = instr_create(dc);
    decode(dc, buf, ins2);
    ASSERT(instr_same(ins1, ins2));
    instr_reset(dc, ins1);
    instr_reset(dc, ins2);
    /* load 1 byte */
    ins1 = XINST_CREATE_load_1byte(dc, opnd_create_reg(reg_resize_to_opsz(reg, OPSZ_1)),
                                   MEMARG(OPSZ_1));
    pc = instr_encode(dc, ins1, buf);
    ASSERT(pc != NULL);
    ins2 = instr_create(dc);
    decode(dc, buf, ins2);
    ASSERT(instr_same(ins1, ins2));
    instr_reset(dc, ins1);
    instr_reset(dc, ins2);
    /* load 2 bytes */
    ins1 = XINST_CREATE_load_2bytes(dc, opnd_create_reg(reg_resize_to_opsz(reg, OPSZ_2)),
                                    MEMARG(OPSZ_2));
    pc = instr_encode(dc, ins1, buf);
    ASSERT(pc != NULL);
    ins2 = instr_create(dc);
    decode(dc, buf, ins2);
    ASSERT(instr_same(ins1, ins2));
    instr_reset(dc, ins1);
    instr_reset(dc, ins2);
    /* store 1 byte */
    ins1 = XINST_CREATE_store_1byte(dc, MEMARG(OPSZ_1),
                                    opnd_create_reg(reg_resize_to_opsz(reg, OPSZ_1)));
    pc = instr_encode(dc, ins1, buf);
    ASSERT(pc != NULL);
    ins2 = instr_create(dc);
    decode(dc, buf, ins2);
    ASSERT(instr_same(ins1, ins2));
    instr_reset(dc, ins1);
    instr_reset(dc, ins2);
    /* store 1 byte */
    ins1 = XINST_CREATE_store_2bytes(dc, MEMARG(OPSZ_2),
                                     opnd_create_reg(reg_resize_to_opsz(reg, OPSZ_2)));
    pc = instr_encode(dc, ins1, buf);
    ASSERT(pc != NULL);
    ins2 = instr_create(dc);
    decode(dc, buf, ins2);
    ASSERT(instr_same(ins1, ins2));
    instr_reset(dc, ins1);
    instr_reset(dc, ins2);
}

static void
test_stack_pointer_size(void *dc)
{
    /* Test i#2281 where we had the stack pointer size incorrectly varying.
     * We can't simply append these to dis-udis86-randtest.raw b/c our test
     * there uses -syntax_intel.  We could make a new raw DR-style test.
     */
    byte *pc;
    char dbuf[512];
    int len;
    const byte bytes_push[] = { 0x67, 0x51 };
    const byte bytes_ret[] = { 0x67, 0xc3 };
    const byte bytes_enter[] = { 0x67, 0xc8, 0xab, 0xcd, 0xef };
    const byte bytes_leave[] = { 0x67, 0xc9 };

    pc =
        disassemble_to_buffer(dc, (byte *)bytes_push, (byte *)bytes_push, false /*no pc*/,
                              false /*no bytes*/, dbuf, BUFFER_SIZE_ELEMENTS(dbuf), &len);
    ASSERT(pc != NULL && pc - (byte *)bytes_push == sizeof(bytes_push));
    ASSERT(strcmp(dbuf,
                  IF_X64_ELSE(
                      "addr32 push   %rcx %rsp -> %rsp 0xfffffff8(%rsp)[8byte]\n",
                      "addr16 push   %ecx %esp -> %esp 0xfffffffc(%esp)[4byte]\n")) == 0);

    pc =
        disassemble_to_buffer(dc, (byte *)bytes_ret, (byte *)bytes_ret, false /*no pc*/,
                              false /*no bytes*/, dbuf, BUFFER_SIZE_ELEMENTS(dbuf), &len);
    ASSERT(pc != NULL && pc - (byte *)bytes_ret == sizeof(bytes_ret));
    ASSERT(strcmp(dbuf,
                  IF_X64_ELSE("addr32 ret    %rsp (%rsp)[8byte] -> %rsp\n",
                              "addr16 ret    %esp (%esp)[4byte] -> %esp\n")) == 0);

    pc = disassemble_to_buffer(dc, (byte *)bytes_enter, (byte *)bytes_enter,
                               false /*no pc*/, false /*no bytes*/, dbuf,
                               BUFFER_SIZE_ELEMENTS(dbuf), &len);
    ASSERT(pc != NULL && pc - (byte *)bytes_enter == sizeof(bytes_enter));
    ASSERT(
        strcmp(dbuf,
               IF_X64_ELSE(
                   "addr32 enter  $0xcdab $0xef %rsp %rbp -> %rsp 0xfffffff8(%rsp)[8byte]"
                   " %rbp\n",
                   "addr16 enter  $0xcdab $0xef %esp %ebp -> %esp 0xfffffffc(%esp)[4byte]"
                   " %ebp\n")) == 0);

    pc = disassemble_to_buffer(dc, (byte *)bytes_leave, (byte *)bytes_leave,
                               false /*no pc*/, false /*no bytes*/, dbuf,
                               BUFFER_SIZE_ELEMENTS(dbuf), &len);
    ASSERT(pc != NULL && pc - (byte *)bytes_leave == sizeof(bytes_leave));
    ASSERT(strcmp(dbuf,
                  IF_X64_ELSE("addr32 leave  %rbp %rsp (%rbp)[8byte] -> %rsp %rbp\n",
                              "addr16 leave  %ebp %esp (%ebp)[4byte] -> %esp %ebp\n")) ==
           0);
}

int
main(int argc, char *argv[])
{
#ifdef STANDALONE_DECODER
    void *dcontext = GLOBAL_DCONTEXT;
#else
    void *dcontext = dr_standalone_init();

    /* simple test of deadlock_avoidance, etc. being disabled in standalone */
    void *x = dr_mutex_create();
    dr_mutex_lock(x);
    dr_mutex_unlock(x);
    dr_mutex_destroy(x);
#endif

    test_all_opcodes_0(dcontext);
#ifndef STANDALONE_DECODER /* speed up compilation */
    test_all_opcodes_1(dcontext);
    test_all_opcodes_2(dcontext);
    test_all_opcodes_2_mm(dcontext);
    test_all_opcodes_3(dcontext);
    test_all_opcodes_3_avx(dcontext);
    test_all_opcodes_4(dcontext);
#endif

    test_disp_control(dcontext);

    test_indirect_cti(dcontext);

    test_cti_prefixes(dcontext);

#ifndef X64
    test_modrm16(dcontext);
#endif

    test_size_changes(dcontext);

    test_nop_xchg(dcontext);

    test_hint_nops(dcontext);

#ifdef X64
    test_x86_mode(dcontext);

    test_x64_abs_addr(dcontext);

    test_x64_inc(dcontext);
#endif

    test_regs(dcontext);

    test_instr_opnds(dcontext);

    test_strict_invalid(dcontext);

    test_tsx(dcontext);

    test_vsib(dcontext);

    test_disasm_sizes(dcontext);

    test_predication(dcontext);

    test_xinst_create(dcontext);

    test_stack_pointer_size(dcontext);

#ifndef STANDALONE_DECODER /* speed up compilation */
    test_all_opcodes_2_avx512_vex(dcontext);
    test_all_opcodes_3_avx512_vex(dcontext);
    test_opmask_disas_avx512(dcontext);
    test_all_opcodes_3_avx512_evex_mask(dcontext);
    test_disas_3_avx512_evex_mask(dcontext);
    test_all_opcodes_5_avx512_evex_mask(dcontext);
    test_all_opcodes_4_avx512_evex_mask(dcontext);
    test_all_opcodes_4_avx512_evex(dcontext);
    test_all_opcodes_3_avx512_evex(dcontext);
    test_all_opcodes_2_avx512_evex(dcontext);
    test_all_opcodes_2_avx512_evex_mask(dcontext);
    /* We're testing a scalable displacement for evex instructions in addition to the
     * default displacement. The default displacement will become a full 32-bit
     * displacement, while the scalable displacement will get compressed to 8-bit.
     */
    test_all_opcodes_3_avx512_evex_mask_scaled_disp8(dcontext);
    test_all_opcodes_5_avx512_evex_mask_scaled_disp8(dcontext);
    test_all_opcodes_4_avx512_evex_mask_scaled_disp8(dcontext);
    test_all_opcodes_4_avx512_evex_scaled_disp8(dcontext);
    test_all_opcodes_3_avx512_evex_scaled_disp8(dcontext);
    test_all_opcodes_2_avx512_evex_scaled_disp8(dcontext);
#endif

    print("all done\n");
    return 0;
}

#pragma once

#include <cstdint>
#include <vector>
#include "common.h"

class RDNA2Assembler {
public:

    enum class ssrc : uint8_t {
        sgpr0 = 0,
        vcc_lo = 106,
        vcc_hi = 107,
        ttmp0 = 108,
        m0 = 124,
        null_reg = 125,
        exec_lo = 126,
        exec_hi = 127,
        src_zero = 128,
        int_pos_1 = 129,
        int_pos_64 = 192,
        int_neg_1 = 193,
        int_neg_16 = 208,
        shared_base = 235,
        shared_limit = 236,
        private_base = 237,
        private_limit = 238,
        pops_exiting_wave_id = 239,
        float_0_5 = 240,
        float_neg_0_5 = 241,
        float_1_0 = 242,
        float_neg_1_0 = 243,
        float_2_0 = 244,
        float_neg_2_0 = 245,
        float_4_0 = 246,
        float_neg_4_0 = 247,
        float_1_over_2pi = 248,
        vccz = 251,
        execz = 252,
        scc = 253,
        literal_constant = 255
    };

    enum class vsrc : uint16_t {
        sgpr0 = 0,
        vcc_lo = 106,
        vcc_hi = 107,
        ttmp0 = 108,
        m0 = 124,
        null_reg = 125,
        exec_lo = 126,
        exec_hi = 127,
        zero = 128,
        int_pos_1 = 129,
        int_pos_64 = 192,
        int_neg_1 = 193,
        int_neg_16 = 208,
        dpp8 = 233,
        dpp8fi = 234,
        shared_base = 235,
        shared_limit = 236,
        private_base = 237,
        private_limit = 238,
        pops_exiting_wave_id = 239,
        float_0_5 = 240,
        float_neg_0_5 = 241,
        float_1_0 = 242,
        float_neg_1_0 = 243,
        float_2_0 = 244,
        float_neg_2_0 = 245,
        float_4_0 = 246,
        float_neg_4_0 = 247,
        float_1_over_2pi = 248,
        sdwa = 249,
        dpp16 = 250,
        vccz = 251,
        execz = 252,
        scc = 253,
        literal_constant = 255,
        vgpr0 = 256
    };

    enum class sop2_opcode : uint8_t {
        s_add_u32 = 0,
        s_sub_u32 = 1,
        s_add_i32 = 2,
        s_sub_i32 = 3,
        s_addc_u32 = 4,
        s_subb_u32 = 5,
        s_min_i32 = 6,
        s_min_u32 = 7,
        s_max_i32 = 8,
        s_max_u32 = 9,
        s_cselect_b32 = 10,
        s_cselect_b64 = 11,
        s_and_b32 = 14,
        s_and_b64 = 15,
        s_or_b32 = 16,
        s_or_b64 = 17,
        s_xor_b32 = 18,
        s_xor_b64 = 19,
        s_andn2_b32 = 20,
        s_andn2_b64 = 21,
        s_orn2_b32 = 22,
        s_orn2_b64 = 23,
        s_nand_b32 = 24,
        s_nand_b64 = 25,
        s_nor_b32 = 26,
        s_nor_b64 = 27,
        s_xnor_b32 = 28,
        s_xnor_b64 = 29,
        s_lshl_b32 = 30,
        s_lshl_b64 = 31,
        s_lshr_b32 = 32,
        s_lshr_b64 = 33,
        s_ashr_i32 = 34,
        s_ashr_i64 = 35,
        s_bfm_b32 = 36,
        s_bfm_b64 = 37,
        s_mul_i32 = 38,
        s_bfe_u32 = 39,
        s_bfe_i32 = 40,
        s_bfe_u64 = 41,
        s_bfe_i64 = 42,
        s_absdiff_i32 = 44,
        s_lshl1_add_u32 = 46,
        s_lshl2_add_u32 = 47,
        s_lshl3_add_u32 = 48,
        s_lshl4_add_u32 = 49,
        s_pack_ll_b32_b16 = 50,
        s_pack_lh_b32_b16 = 51,
        s_pack_hh_b32_b16 = 52,
        s_mul_hi_u32 = 53,
        s_mul_hi_i32 = 54
    };

    inline void sop2(sop2_opcode op, ssrc sdst, ssrc ssrc0, ssrc ssrc1) {
        assert((uint8_t)sdst < 128, "sop2: invalid ssrc for sdst: %u", (uint8_t)sdst);
        emit(0b10 << 30 | (uint8_t)op << 23 | (uint8_t)sdst << 16 | (uint8_t)ssrc1 << 8 | (uint8_t)ssrc0);
    }

    enum class sopk_opcode : uint8_t {
        s_movk_i32 = 0,
        s_version = 1,
        s_cmovk_i32 = 2,
        s_cmpk_eq_i32 = 3,
        s_cmpk_lg_i32 = 4,
        s_cmpk_gt_i32 = 5,
        s_cmpk_ge_i32 = 6,
        s_cmpk_lt_i32 = 7,
        s_cmpk_le_i32 = 8,
        s_cmpk_eq_u32 = 9,
        s_cmpk_lg_u32 = 10,
        s_cmpk_gt_u32 = 11,
        s_cmpk_ge_u32 = 12,
        s_cmpk_lt_u32 = 13,
        s_cmpk_le_u32 = 14,
        s_addk_i32 = 15,
        s_mulk_i32 = 16,
        s_getreg_b32 = 18,
        s_setreg_b32 = 19,
        s_setreg_imm32_b32 = 21,
        s_call_b64 = 22,
        s_waitcnt_vscnt = 23,
        s_waitcnt_vmcnt = 24,
        s_waitcnt_expcnt = 25,
        s_waitcnt_lgkmcnt = 26,
        s_subvector_loop_begin = 27,
        s_subvector_loop_end = 28
    };

    inline void sopk(sopk_opcode op, ssrc sdst, int16_t imm) {
        assert((uint8_t)sdst < 128, "sopk: invalid ssrc for sdst: %u", (uint8_t)sdst);
        emit(0b1011 << 28 | (uint8_t)op << 23 | (uint8_t)sdst << 16 | imm);
    }

    enum class sop1_opcode : uint8_t {
        s_mov_b32 = 3,
        s_mov_b64 = 4,
        s_cmov_b32 = 5,
        s_cmov_b64 = 6,
        s_not_b32 = 7,
        s_not_b64 = 8,
        s_wqm_b32 = 9,
        s_wqm_b64 = 10,
        s_brev_b32 = 11,
        s_brev_b64 = 12,
        s_bcnt0_i32_b32 = 13,
        s_bcnt0_i32_b64 = 14,
        s_bcnt1_i32_b32 = 15,
        s_bcnt1_i32_b64 = 16,
        s_ff0_i32_b32 = 17,
        s_ff0_i32_b64 = 18,
        s_ff1_i32_b32 = 19,
        s_ff1_i32_b64 = 20,
        s_flbit_i32_b32 = 21,
        s_flbit_i32_b64 = 22,
        s_flbit_i32 = 23,
        s_flbit_i32_i64 = 24,
        s_sext_i32_i8 = 25,
        s_sext_i32_i16 = 26,
        s_bitset0_b32 = 27,
        s_bitset0_b64 = 28,
        s_bitset1_b32 = 29,
        s_bitset1_b64 = 30,
        s_getpc_b64 = 31,
        s_setpc_b64 = 32,
        s_swappc_b64 = 33,
        s_rfe_b64 = 34,
        s_and_saveexec_b64 = 36,
        s_or_saveexec_b64 = 37,
        s_xor_saveexec_b64 = 38,
        s_andn2_saveexec_b64 = 39,
        s_orn2_saveexec_b64 = 40,
        s_nand_saveexec_b64 = 41,
        s_nor_saveexec_b64 = 42,
        s_xnor_saveexec_b64 = 43,
        s_quadmask_b32 = 44,
        s_quadmask_b64 = 45,
        s_movrels_b32 = 46,
        s_movrels_b64 = 47,
        s_movreld_b32 = 48,
        s_movreld_b64 = 49,
        s_abs_i32 = 52,
        s_andn1_saveexec_b64 = 55,
        s_orn1_saveexec_b64 = 56,
        s_andn1_wrexec_b64 = 57,
        s_andn2_wrexec_b64 = 57,
        s_bitreplicate_b64_b32 = 59,
        s_and_saveexec_b32 = 60,
        s_or_saveexec_b32 = 61,
        s_xor_saveexec_b32 = 62,
        s_andn2_saveexec_b32 = 63,
        s_orn2_saveexec_b32 = 64,
        s_nand_saveexec_b32 = 65,
        s_nor_saveexec_b32 = 66,
    };

    inline void sop1(sop1_opcode op, ssrc sdst, ssrc ssrc0) {
        assert((uint8_t)sdst < 128, "sop1: invalid ssrc for sdst: %u", (uint8_t)sdst);
        emit(0b101111101 << 23 | (uint8_t)sdst << 16 | (uint8_t)op << 8 | (uint8_t)ssrc0);
    }

    enum class sopc_opcode : uint8_t {
        s_cmp_eq_i32 = 0,
        s_cmp_lg_i32 = 1,
        s_cmp_gt_i32 = 2,
        s_cmp_ge_i32 = 3,
        s_cmp_lt_i32 = 4,
        s_cmp_le_i32 = 5,
        s_cmp_eq_u32 = 6,
        s_cmp_lg_u32 = 7,
        s_cmp_gt_u32 = 8,
        s_cmp_ge_u32 = 9,
        s_cmp_lt_u32 = 10,
        s_cmp_le_u32 = 11,
        s_bitcmp0_b32 = 12,
        s_bitcmp1_b32 = 13,
        s_bitcmp0_b64 = 14,
        s_bitcmp1_b64 = 15,
        s_cmp_eq_u64 = 18,
        s_cmp_lg_u64 = 19
    };

    inline void sopc(sopc_opcode op, ssrc ssrc0, ssrc ssrc1) {
        emit(0b101111110 << 23 | (uint8_t)op << 16 | (uint8_t)ssrc1 << 8 | (uint8_t)ssrc0);
    }

    enum class sopp_opcode : uint8_t {
        s_nop = 0,
        s_endpgm = 1,
        s_branch = 2,
        s_wakeup = 3,
        s_cbranch_scc0 = 4,
        s_cbranch_scc1 = 5,
        s_cbranch_vccz = 6,
        s_cbranch_vccnz = 7,
        s_cbranch_execz = 8,
        s_cbranch_execnz = 9,
        s_barrier = 10,
        s_setkill = 11,
        s_waitcnt = 12,
        s_sethalt = 13,
        s_sleep = 14,
        s_setprio = 15,
        s_sendmsg = 16,
        s_sendmsghalt = 17,
        s_trap = 18,
        s_icache_inv = 19,
        s_incperflevel = 20,
        s_decperflevel = 21,
        s_ttracedata = 22,
        s_cbranch_cdbgsys = 23,
        s_cbranch_cdbguser = 24,
        s_cbranch_cdbgsys_or_user = 25,
        s_cbranch_cdbgsys_and_user = 26,
        s_endpgm_saved = 27,
        s_endpgm_ordered_ps_done = 30,
        s_code_end = 31,
        s_inst_prefetch = 32,
        s_clause = 33,
        s_waitcnt_depctr = 34,
        s_round_mode = 36,
        s_denorm_mode = 37,
        s_ttracedata_imm = 40
    };

    inline void sopp(sopp_opcode op, int16_t simm) {
        emit(0b101111111 << 23 | ((uint8_t)op & 0x7F) << 16 | simm);
    }

    enum class smem_opcode : uint8_t {
        s_load_dword = 0,
        s_load_dwordx2 = 1,
        s_load_dwordx4 = 2,
        s_load_dwordx8 = 3,
        s_load_dwordx16 = 4,
        s_buffer_load_dword = 8,
        s_buffer_load_dwordx2 = 9,
        s_buffer_load_dwordx4 = 10,
        s_buffer_load_dwordx8 = 11,
        s_buffer_load_dwordx16 = 12,
        s_gl1_inv = 31,
        s_dcache_inv = 32,
        s_memtime = 36,
        s_memrealtime = 37,
        s_atc_probe = 38,
        s_atc_probe_buffer = 39
    };

    inline void smem(smem_opcode op, bool glc, bool dlc, uint8_t sdata, uint8_t sbase, uint8_t soffset, int32_t offset) {
        assert(offset >= -1048576 && offset <= 1048575, "smem: offset exceeds 21-bit signed range: %d", offset);

        // LSB of sbase is ignored.
        auto sbase_enc = (sbase >> 1);
        auto enc_offset = offset & 0x1FFFFF;
        emit(0b111101u << 26 | ((uint8_t)op & 0xFF) << 18 | (glc & 0b1) << 16 | (dlc & 0b1) << 14 | (sdata & 0x7F) << 6 | sbase_enc & 0x3F);
        emit((soffset & 0x7F) << 25 | (offset & 0x1FFFFF));
    }

    enum class vop2_opcode : uint8_t {
        v_cndmask_b32 = 1,
        v_dot2c_f32_f16 = 2,
        v_add_f32 = 3,
        v_sub_f32 = 4,
        v_subrev_f32 = 5,
        v_fmac_legacy_f32 = 6,
        v_mul_legacy_f32 = 7,
        v_mul_f32 = 8,
        v_mul_i32_i24 = 9,
        v_mul_hi_i32_i24 = 10,
        v_mul_u32_u24 = 11,
        v_mul_hi_u32_u24 = 12,
        v_dot4c_i32_i8 = 13,
        v_min_f32 = 15,
        v_max_f32 = 16,
        v_min_i32 = 17,
        v_max_i32 = 18,
        v_min_u32 = 19,
        v_max_u32 = 20,
        v_lshrrev_b32 = 22,
        v_ashrrev_i32 = 24,
        v_lshlrev_b32 = 26,
        v_and_b32 = 27,
        v_or_b32 = 28,
        v_xor_b32 = 29,
        v_xnor_b32 = 30,
        v_add_nc_u32 = 37,
        v_sub_nc_u32 = 38,
        v_subrev_nc_u32 = 39,
        v_add_co_ci_u32 = 40,
        v_sub_co_ci_u32 = 41,
        v_subrev_co_ci_u32 = 42,
        v_fmac_f32 = 43,
        v_fmamk_f32 = 44,
        v_fmaak_f32 = 45,
        v_cvt_pkrtz_f16_f32 = 47,
        v_add_f16 = 50,
        v_sub_f16 = 51,
        v_subrev_f16 = 52,
        v_mul_f16 = 53,
        v_fmac_f16 = 54,
        v_fmamk_f16 = 55,
        v_fmaak_f16 = 56,
        v_max_f16 = 57,
        v_min_f16 = 58,
        v_ldexp_f16 = 59,
        v_pk_fmac_f16 = 60
    };

    inline void vop2(vop2_opcode op, uint8_t vdst, vsrc src0, uint8_t vsrc1) {
        emit(0b0 << 31 | (uint8_t)op << 25 | vdst << 17 | vsrc1 << 9 | (uint16_t)src0 & 0x1FF);
    }

    enum class ds_opcode : uint8_t {
        ds_add_u32 = 0,
        ds_sub_u32 = 1,
        ds_rsub_u32 = 2,
        ds_inc_u32 = 3,
        ds_dec_u32 = 4,
        ds_min_i32 = 5,
        ds_max_i32 = 6,
        ds_min_u32 = 7,
        ds_max_u32 = 8,
        ds_and_b32 = 9,
        ds_or_b32 = 10,
        ds_xor_b32 = 11,
        ds_mskor_b32 = 12,
        ds_write_b32 = 13,
        ds_write2_b32 = 14,
        ds_write2st64_b32 = 15,
        ds_cmpst_b32 = 16,
        ds_cmpst_f32 = 17,
        ds_min_f32 = 18,
        ds_max_f32 = 19,
        ds_nop = 20,
        ds_add_f32 = 21,

        ds_gws_sema_release_all = 24,
        ds_gws_init = 25,
        ds_gws_sema_v = 26,
        ds_gws_sema_br = 27,
        ds_gws_sema_p = 28,
        ds_gws_barrier = 29,

        // @todo: add the rest
    };

    inline void ds(ds_opcode op, bool gds, uint8_t offset0, uint8_t offset1,
                   uint8_t addr, uint8_t data0, uint8_t data1, uint8_t vdst) {
        emit(0b110110 << 26 | (uint8_t)op << 17 | offset1 << 8 | offset0);
        emit((vdst & 0xFF) << 24 | (gds & 0b1) << 17 | (data1 & 0xFF) << 8 | addr & 0xFF);
    }

    // @todo: i think flat & global are really the same..
    // may want to consolidate them, but what about scratch?
    enum class flat_opcode : uint8_t {
        flat_load_ubyte          = 8,
        flat_load_sbyte          = 9,
        flat_load_ushort         = 10,
        flat_load_sshort         = 11,
        flat_load_dword          = 12,
        flat_load_dwordx2        = 13,
        flat_load_dwordx4        = 14,
        flat_load_dwordx3        = 15,
        flat_store_byte          = 24,
        flat_store_byte_d16_hi   = 25,
        flat_store_short         = 26,
        flat_store_short_d16_hi  = 27,
        flat_store_dword         = 28,
        flat_store_dwordx2       = 29,
        flat_store_dwordx4       = 30,
        flat_store_dwordx3       = 31,
        flat_load_ubyte_d16      = 32,
        flat_load_ubyte_d16_hi   = 33,
        flat_load_sbyte_d16      = 34,
        flat_load_sbyte_d16_hi   = 35,
        flat_load_short_d16      = 36,
        flat_load_short_d16_hi   = 37,
        flat_atomic_swap         = 48,
        flat_atomic_cmpswap      = 49,
        flat_atomic_add          = 50,
        flat_atomic_sub          = 51,
        flat_atomic_smin         = 53,
        flat_atomic_umin         = 54,
        flat_atomic_smax         = 55,
        flat_atomic_umax         = 56,
        flat_atomic_and          = 57,
        flat_atomic_or           = 58,
        flat_atomic_xor          = 59,
        flat_atomic_inc          = 60,
        flat_atomic_dec          = 61,
        flat_atomic_fcmpswap     = 62,
        flat_atomic_fmin         = 63,
        flat_atomic_fmax         = 64,
        flat_atomic_swap_x2      = 80,
        flat_atomic_cmpswap_x2   = 81,
        flat_atomic_add_x2       = 82,
        flat_atomic_sub_x2       = 83,
        flat_atomic_smin_x2      = 85,
        flat_atomic_umin_x2      = 86,
        flat_atomic_smax_x2      = 87,
        flat_atomic_umax_x2      = 88,
        flat_atomic_and_x2       = 89,
        flat_atomic_or_x2        = 90,
        flat_atomic_xor_x2       = 91,
        flat_atomic_inc_x2       = 92,
        flat_atomic_dec_x2       = 93,
        flat_atomic_fcmpswap_x2  = 94,
        flat_atomic_fmin_x2      = 95,
        flat_atomic_fmax_x2      = 96
    };

    enum class global_opcode : uint8_t {
        global_load_ubyte          = 8,
        global_load_sbyte          = 9,
        global_load_ushort         = 10,
        global_load_sshort         = 11,
        global_load_dword          = 12,
        global_load_dwordx2        = 13,
        global_load_dwordx4        = 14,
        global_load_dwordx3        = 15,
        global_load_dword_addtid   = 22,
        global_store_dword_addtid  = 23,
        global_store_byte          = 24,
        global_store_byte_d16_hi   = 25,
        global_store_short         = 26,
        global_store_short_d16_hi  = 27,
        global_store_dword         = 28,
        global_store_dwordx2       = 29,
        global_store_dwordx4       = 30,
        global_store_dwordx3       = 31,
        global_load_ubyte_d16      = 32,
        global_load_ubyte_d16_hi   = 33,
        global_load_sbyte_d16      = 34,
        global_load_sbyte_d16_hi   = 35,
        global_load_short_d16      = 36,
        global_load_short_d16_hi   = 37,
        global_atomic_swap         = 48,
        global_atomic_cmpswap      = 49,
        global_atomic_add          = 50,
        global_atomic_sub          = 51,
        global_atomic_csub         = 52,
        global_atomic_smin         = 53,
        global_atomic_umin         = 54,
        global_atomic_smax         = 55,
        global_atomic_umax         = 56,
        global_atomic_and          = 57,
        global_atomic_or           = 58,
        global_atomic_xor          = 59,
        global_atomic_inc          = 60,
        global_atomic_dec          = 61,
        global_atomic_fcmpswap     = 62,
        global_atomic_fmin         = 63,
        global_atomic_fmax         = 64,
        global_atomic_swap_x2      = 80,
        global_atomic_cmpswap_x2   = 81,
        global_atomic_add_x2       = 82,
        global_atomic_sub_x2       = 83,
        global_atomic_smin_x2      = 85,
        global_atomic_umin_x2      = 86,
        global_atomic_smax_x2      = 87,
        global_atomic_umax_x2      = 88,
        global_atomic_and_x2       = 89,
        global_atomic_or_x2        = 90,
        global_atomic_xor_x2       = 91,
        global_atomic_inc_x2       = 92,
        global_atomic_dec_x2       = 93,
        global_atomic_fcmpswap_x2  = 94,
        global_atomic_fmin_x2      = 95,
        global_atomic_fmax_x2      = 96
    };

    enum class scratch_opcode : uint8_t {
        scratch_load_ubyte         = 8,
        scratch_load_sbyte         = 9,
        scratch_load_ushort        = 10,
        scratch_load_sshort        = 11,
        scratch_load_dword         = 12,
        scratch_load_dwordx2       = 13,
        scratch_load_dwordx4       = 14,
        scratch_load_dwordx3       = 15,
        scratch_store_byte         = 24,
        scratch_store_byte_d16_hi  = 25,
        scratch_store_short        = 26,
        scratch_store_short_d16_hi = 27,
        scratch_store_dword        = 28,
        scratch_store_dwordx2      = 29,
        scratch_store_dwordx4      = 30,
        scratch_store_dwordx3      = 31,
        scratch_load_ubyte_d16     = 32,
        scratch_load_ubyte_d16_hi  = 33,
        scratch_load_sbyte_d16     = 34,
        scratch_load_sbyte_d16_hi  = 35,
        scratch_load_short_d16     = 36,
        scratch_load_short_d16_hi  = 37
    };

    inline void flat(flat_opcode op, bool slc, bool glc, bool lds, bool dlc, uint16_t offset, uint8_t vdst, uint8_t saddr, uint8_t data, uint8_t addr) {
        flat_impl((uint8_t)op, slc, glc, 0, lds, dlc, offset, vdst, saddr, data, addr);
    }

    inline void scratch(scratch_opcode op, bool slc, bool glc, bool lds, bool dlc, uint16_t offset, uint8_t vdst, uint8_t saddr, uint8_t data, uint8_t addr) {
        flat_impl((uint8_t)op, slc, glc, 1, lds, dlc, offset, vdst, saddr, data, addr);
    }

    inline void global(global_opcode op, bool slc, bool glc, bool lds, bool dlc, uint16_t offset, uint8_t vdst, uint8_t saddr, uint8_t data, uint8_t addr) {
        flat_impl((uint8_t)op, slc, glc, 2, lds, dlc, offset, vdst, saddr, data, addr);
    }

    inline void waitcnt(uint8_t vmcnt, uint8_t expcnt, uint8_t lgkmcnt) {
        sopp(sopp_opcode::s_waitcnt, (vmcnt & 0x18) << 14 | (lgkmcnt & 0x1F) << 8 | (expcnt & 0x3) << 4 | (vmcnt & 0xF));
    }

    std::vector<uint32_t> &values() {
        return m_values;
    }
private:

    inline void flat_impl(uint8_t op, bool slc, bool glc, uint8_t seg, bool lds, bool dlc, uint16_t offset, uint8_t vdst, uint8_t saddr, uint8_t data, uint8_t addr) {
        emit(0b110111 << 26 | (op & 0x7F) << 18 | (slc & 0b1) << 17 | (glc & 0b1) << 16 | (seg & 0x2) << 14 | (lds & 0b1) << 13 | (dlc & 0b1) << 12 | offset & 0x7FF);
        emit((vdst & 0xFF) << 24 | (saddr & 0x7F) << 16 | (data & 0xFF) << 8 | addr & 0xFF);
    }

    void emit(uint32_t v) {
        m_values.push_back(v);
    }

    std::vector<uint32_t> m_values;
};

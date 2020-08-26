#include "minimap_levels.h"

const struct MiniMapInfo level_unknown[] = {
    {unknown_t3x, unknown_t3x_size, 0x000000},
};
const struct MiniMapInfo level_bbh[] = {
    {bbh_t3x, bbh_t3x_size, 0x000000},
};
const struct MiniMapInfo level_ccm[] = {
    {ccm_1_t3x, ccm_1_t3x_size, 0x848c94},
    {ccm_2_t3x, ccm_2_t3x_size, 0x181818},
};
const struct MiniMapInfo level_castle[] = {
    {castle_1_t3x, castle_1_t3x_size, 0x000000},
    {castle_2_t3x, castle_2_t3x_size, 0x000000},
    {castle_3_t3x, castle_3_t3x_size, 0x000000},
};
const struct MiniMapInfo level_hmc[] = {
    {hmc_t3x, hmc_t3x_size, 0xA06A85},
};
const struct MiniMapInfo level_ssl[] = {
    {ssl_1_t3x, ssl_1_t3x_size, 0x000000},
    {ssl_2_t3x, ssl_2_t3x_size, 0x000000},
    {ssl_3_t3x, ssl_3_t3x_size, 0x000000},
};
const struct MiniMapInfo level_bob[] = {
    {bob_t3x, bob_t3x_size, 0x0cae1f},
};
const struct MiniMapInfo level_sl[] = {
    {sl_1_t3x, sl_1_t3x_size, 0x000000},
    {sl_2_t3x, sl_2_t3x_size, 0x000000},
};
const struct MiniMapInfo level_wdw[] = {
    {wdw_1_t3x, wdw_1_t3x_size, 0x000000},
    {wdw_2_t3x, wdw_2_t3x_size, 0x000000},
};
const struct MiniMapInfo level_jrb[] = {
    {jrb_1_t3x, jrb_1_t3x_size, 0x94cebd},
    {jrb_2_t3x, jrb_2_t3x_size, 0x000000},
};
const struct MiniMapInfo level_thi[] = {
    {thi_1_t3x, thi_1_t3x_size, 0x000000},
    {thi_2_t3x, thi_2_t3x_size, 0x000000},
    {thi_3_t3x, thi_3_t3x_size, 0x000000},
};
const struct MiniMapInfo level_ttc[] = {
    {ttc_t3x, ttc_t3x_size, 0x000000},
};
const struct MiniMapInfo level_rr[] = {
    {rr_t3x, rr_t3x_size, 0x3152ad},
};
const struct MiniMapInfo level_castle_grounds[] = {
    {castle_grounds_t3x, castle_grounds_t3x_size, 0x0ebc00},
    /* drained castle grounds is not an area but a savegame flag */
    {castle_grounds_drained_t3x, castle_grounds_drained_t3x_size, 0x0ebc00},
};
const struct MiniMapInfo level_bitdw[] = {
    {bitdw_t3x, bitdw_t3x_size, 0x000000},
};
const struct MiniMapInfo level_vcutm[] = {
    {vcutm_t3x, vcutm_t3x_size, 0x000000},
};
const struct MiniMapInfo level_bitfs[] = {
    {bitfs_t3x, bitfs_t3x_size, 0x000000},
};
const struct MiniMapInfo level_sa[] = {
    {sa_t3x, sa_t3x_size, 0x000000},
};
const struct MiniMapInfo level_bits[] = {
    {bits_t3x, bits_t3x_size, 0x000000},
};
const struct MiniMapInfo level_lll[] = {
    {lll_1_t3x, lll_1_t3x_size, 0xae1d01},
    {lll_2_t3x, lll_2_t3x_size, 0x000000},
};
const struct MiniMapInfo level_ddd[] = {
    {ddd_1_t3x, ddd_1_t3x_size, 0x000000},
    {ddd_2_t3x, ddd_2_t3x_size, 0x000000},
};
const struct MiniMapInfo level_wf[] = {
    {wf_t3x, wf_t3x_size, 0x8cbdf7},
};
const struct MiniMapInfo level_castle_courtyard[] = {
    {castle_courtyard_t3x, castle_courtyard_t3x_size, 0x52c618},
};
const struct MiniMapInfo level_pss[] = {
    {pss_t3x, pss_t3x_size, 0x000000},
};
const struct MiniMapInfo level_cotmc[] = {
    {cotmc_t3x, cotmc_t3x_size, 0x000000},
};
const struct MiniMapInfo level_totwc[] = {
    {totwc_t3x, totwc_t3x_size, 0x000000},
};
const struct MiniMapInfo level_bowser_1[] = {
    {bowser_1_t3x, bowser_1_t3x_size, 0x00295a},
};
const struct MiniMapInfo level_wmotr[] = {
    {wmotr_t3x, wmotr_t3x_size, 0x000000},
};
const struct MiniMapInfo level_bowser_2[] = {
    {bowser_2_t3x, bowser_2_t3x_size, 0x000000},
};
const struct MiniMapInfo level_bowser_3[] = {
    {bowser_3_t3x, bowser_3_t3x_size, 0x000000},
};
const struct MiniMapInfo level_ttm[] = {
    {ttm_1_t3x, ttm_1_t3x_size, 0x94b5ff},
    {ttm_2_t3x, ttm_2_t3x_size, 0x000000},
    {ttm_3_t3x, ttm_3_t3x_size, 0x000000},
    {ttm_4_t3x, ttm_4_t3x_size, 0x000000},
};

const struct MiniMapInfo *level_info[] = {
    level_unknown,
    level_bbh,
    level_ccm,
    level_castle,
    level_hmc,
    level_ssl,
    level_bob,
    level_sl,
    level_wdw,
    level_jrb,
    level_thi,
    level_ttc,
    level_rr,
    level_castle_grounds,
    level_bitdw,
    level_vcutm,
    level_bitfs,
    level_sa,
    level_bits,
    level_lll,
    level_ddd,
    level_wf,
    level_castle_courtyard,
    level_pss,
    level_cotmc,
    level_totwc,
    level_bowser_1,
    level_wmotr,
    level_bowser_2,
    level_bowser_3,
    level_ttm,
};

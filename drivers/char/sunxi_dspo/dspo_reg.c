/*
 * sunxi_dspo/dspo_reg.c
 *
 * Copyright (c) 2007-2021 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include "dspo_param.h"
#include "dspo_reg.h"

volatile __dspo_dev *dspo_dev[1];

void dspo_set_reg_base(u32 sel, unsigned long base)
{
	dspo_dev[sel] = (__dspo_dev *)(base);
}

void dspo_module_en(u32 dev_sel, u32 module_en, u32 sepa_sync_en)
{
	dspo_dev[dev_sel]->module_ctrl.bits.seperate_sync_en = sepa_sync_en;
	dspo_dev[dev_sel]->module_ctrl.bits.dspo_mudule_en = module_en;
}

void dspo_dma_aidb_sel(u32 dev_sel, u32 sel)
{
	dspo_dev[dev_sel]->module_ctrl.bits.dma_aidb_sel = sel;
}

void dspo_soft_rst(u32 dev_sel, u32 rst)
{
	dspo_dev[dev_sel]->module_ctrl.bits.soft_rst = rst;
}

void dspo_fmt_set(u32 dev_sel, u32 data_seq_sel, u32 sync_fmt, u32 data_width,
		  u32 interlace)
{
	dspo_dev[dev_sel]->fmt_ctrl.bits.data_seq_sel = data_seq_sel;
	dspo_dev[dev_sel]->fmt_ctrl.bits.embedded_sync_fmt = sync_fmt;
	dspo_dev[dev_sel]->fmt_ctrl.bits.output_data_width = data_width;
	dspo_dev[dev_sel]->fmt_ctrl.bits.interlace_mode = interlace;
}

void dspo_yuv_data_src_sel(u32 dev_sel, u32 sel)
{
	dspo_dev[dev_sel]->fmt_ctrl.bits.data_src_sel = sel;
}

void dspo_chroma_spl_set(u32 dev_sel, u8 cr_type, u8 cb_type)
{
	dspo_dev[dev_sel]->hor_spl_ctrl.bits.cb_hori_spl_type = cb_type;
	dspo_dev[dev_sel]->hor_spl_ctrl.bits.cr_hori_spl_type = cr_type;
}

void dspo_clamp_set(u32 dev_sel, u16 y_min, u16 y_max, u16 cb_min, u16 cb_max,
		    u16 cr_min, u16 cr_max)
{
	dspo_dev[dev_sel]->clamp0.bits.y_val_range_min = y_min;
	dspo_dev[dev_sel]->clamp0.bits.y_val_range_max = y_max;
	dspo_dev[dev_sel]->clamp1.bits.cb_val_range_min = cb_min;
	dspo_dev[dev_sel]->clamp1.bits.cb_val_range_max = cb_max;
	dspo_dev[dev_sel]->clamp2.bits.cr_val_range_min = cr_min;
	dspo_dev[dev_sel]->clamp2.bits.cr_val_range_max = cr_max;
}

void dspo_sync_pol_set(u32 dev_sel, u8 hb_pol, u8 vb_pol, u8 field_pol)
{
	dspo_dev[dev_sel]->sync_ctrl.bits.h_blank_pol = hb_pol;
	dspo_dev[dev_sel]->sync_ctrl.bits.v_blank_pol = vb_pol;
	dspo_dev[dev_sel]->sync_ctrl.bits.field_pol = field_pol;
}

void dspo_dclk_adjust(u32 dev_sel, u8 dclk_invt, u8 dly_en, u8 dly_num)
{
	dspo_dev[dev_sel]->sync_ctrl.bits.dclk_invert = dclk_invt;
	dspo_dev[dev_sel]->sync_ctrl.bits.dclk_dly_num = dly_num;
	dspo_dev[dev_sel]->sync_ctrl.bits.dclk_dly_en = dly_en;
}

//////////////////////////////////////////////////////////////////////////
u32 dspo_get_curline(u32 sel)
{
	u32 line = dspo_dev[sel]->status.bits.current_line;
	return line;
}

u32 dspo_get_field(u32 sel)
{
	u32 field = dspo_dev[sel]->status.bits.field_pol_sta;
	return field;
}

void dspo_irq_en(u32 sel, u32 int_type, u32 line)
{
	if (int_type == DSPO_V_INT) {
		dspo_dev[sel]->int_ctrl.bits.vb_int_en = 1;
	} else if (int_type == DSPO_L_INT) {
		dspo_dev[sel]->line_int_num.bits.int_line_num = line;
		dspo_dev[sel]->int_ctrl.bits.line_match_int_en = 1;
	} else if (int_type == DSPO_DMA_DONE_INT) {
		dspo_dev[sel]->int_ctrl.bits.dma_done_int_en = 1;
	} else if (int_type == DSPO_DMA_DESC_INT) {
		dspo_dev[sel]->int_ctrl.bits.dma_desc_int_en = 1;
	} else if (int_type == DSPO_FRAME_DONE_INT) {
		dspo_dev[sel]->int_ctrl.bits.frame_int_en = 1;
	} else {
		printk("irq type 0x%x error!!\n", int_type);
	}
}

void dspo_irq_disable(u32 sel, u32 int_type)
{
	if (int_type == DSPO_V_INT) {
		dspo_dev[sel]->int_ctrl.bits.vb_int_en = 0;
	} else if (int_type == DSPO_L_INT) {
		dspo_dev[sel]->int_ctrl.bits.line_match_int_en = 0;
	} else if (int_type == DSPO_DMA_DONE_INT) {
		dspo_dev[sel]->int_ctrl.bits.dma_done_int_en = 0;
	} else if (int_type == DSPO_DMA_DESC_INT) {
		dspo_dev[sel]->int_ctrl.bits.dma_desc_int_en = 0;
	} else if (int_type == DSPO_FRAME_DONE_INT) {
		dspo_dev[sel]->int_ctrl.bits.frame_int_en = 0;
	} else if (int_type == DSPO_LINE_DONE_INT) {
		dspo_dev[sel]->int_ctrl.bits.line_int_en = 0;
	} else if (int_type == DSPO_UV_LINE_DONE_INT) {
		dspo_dev[sel]->int_ctrl.bits.dma_uv_line_int_en = 0;
	} else if (int_type == DSPO_UV_BUF_UF_INT) {
		dspo_dev[sel]->int_ctrl.bits.line_buf_uv_underflow_int_en = 0;
	} else if (int_type == DSPO_UV_BUF_OF_INT) {
		dspo_dev[sel]->int_ctrl.bits.line_buf_uv_overflow_int_en = 0;
	} else if (int_type == DSPO_Y_BUF_UF_INT) {
		dspo_dev[sel]->int_ctrl.bits.line_buf_y_underflow_int_en = 0;
	} else if (int_type == DSPO_Y_BUF_OF_INT) {
		dspo_dev[sel]->int_ctrl.bits.line_buf_y_overflow_int_en = 0;
	} else {
		printk("irq type error!!\n");
	}
}

u32 dspo_irq_process(u32 sel)
{
	u32 ret = 0;
	if ((dspo_dev[sel]->int_ctrl.bits.vb_int_flag) &&
	    (dspo_dev[sel]->int_ctrl.bits.vb_int_en))
		ret = DSPO_V_INT;
	else if ((dspo_dev[sel]->int_ctrl.bits.line_match_int_flag) &&
		 (dspo_dev[sel]->int_ctrl.bits.line_match_int_en))
		ret = DSPO_L_INT;
	else if ((dspo_dev[sel]->int_ctrl.bits.dma_done_int_flag) &&
		 (dspo_dev[sel]->int_ctrl.bits.dma_done_int_en))
		ret = DSPO_DMA_DONE_INT;
	else if ((dspo_dev[sel]->int_ctrl.bits.dma_desc_int_flag) &&
		 (dspo_dev[sel]->int_ctrl.bits.dma_desc_int_en))
		ret = DSPO_DMA_DESC_INT;
	else if ((dspo_dev[sel]->int_ctrl.bits.frame_int_flag) &&
		 (dspo_dev[sel]->int_ctrl.bits.frame_int_en))
		ret = DSPO_FRAME_DONE_INT;

	if (dspo_dev[sel]->int_ctrl.bits.line_buf_uv_underflow_int_flag)
	    ret |= DSPO_UV_BUF_UF_INT;
	else if (dspo_dev[sel]->int_ctrl.bits.line_buf_uv_overflow_int_flag)
	    ret |= DSPO_UV_BUF_OF_INT;
	else if (dspo_dev[sel]->int_ctrl.bits.line_buf_y_underflow_int_flag)
	    ret |= DSPO_Y_BUF_UF_INT;
	else if (dspo_dev[sel]->int_ctrl.bits.line_buf_y_overflow_int_flag)
	    ret |= DSPO_Y_BUF_OF_INT;

	return ret;
}

u32 dspo_clr_irq(u32 sel, u32 int_type)
{
	if (int_type == DSPO_V_INT)
		dspo_dev[sel]->int_ctrl.bits.vb_int_flag = 0;
	else if (int_type == DSPO_L_INT)
		dspo_dev[sel]->int_ctrl.bits.line_match_int_flag = 0;
	else if (int_type == DSPO_DMA_DONE_INT)
		dspo_dev[sel]->int_ctrl.bits.dma_done_int_flag = 0;
	else if (int_type == DSPO_DMA_DESC_INT)
		dspo_dev[sel]->int_ctrl.bits.dma_desc_int_flag = 0;
	else if (int_type == DSPO_FRAME_DONE_INT)
		dspo_dev[sel]->int_ctrl.bits.frame_int_flag = 0;
	return 0;
}

void dspo_timing_set(u32 sel, u32 h_active, u32 hbp, u32 hfp, u32 hsync,
		     u32 v_active, u32 vbp, u32 v_total, u32 vfp, u32 vsync,
		     u32 interlace, u32 itl_mode)
{
	dspo_dev[sel]->h_timing0.bits.h_sync = hsync;
	dspo_dev[sel]->h_timing0.bits.h_fp = hfp;
	dspo_dev[sel]->h_timing1.bits.h_active = h_active;
	dspo_dev[sel]->h_timing1.bits.h_bp = hbp;
	dspo_dev[sel]->v_timing0.bits.v_sync = vsync;
	dspo_dev[sel]->v_timing0.bits.v_fp = vfp;
	dspo_dev[sel]->v_timing1.bits.v_active = v_active; ///(interlace+1)-1;
	dspo_dev[sel]->v_timing1.bits.v_bp = vbp;

	dspo_dev[sel]->interlace_mode.bits.v_total_para =
	    v_total * 2 / (interlace + 1);
	if (interlace) {
		dspo_dev[sel]->interlace_mode.bits.interlace_mode_op = itl_mode;
	}
}

void dspo_vtotal_cfg_auto_set(u32 sel, u32 set)
{
	dspo_dev[sel]->interlace_mode.bits.V_total_cfg_auto = set;
}

void dspo_dma_start(u32 sel)
{
	dspo_dev[sel]->dma_start.bits.dma_start = 1;
}

void dspo_dma_ctrl_set(u32 sel, u32 desc_mode, u32 block_size_sel,
		       u32 input_data_type)
{
	dspo_dev[sel]->dma_tcon_ctrl.bits.descriptor_mode = desc_mode;
	dspo_dev[sel]->dma_tcon_ctrl.bits.dma_block_size_sel = block_size_sel;
	dspo_dev[sel]->dma_tcon_ctrl.bits.input_data_type = input_data_type;
}

void dspo_dma_start_check_set(u32 sel, u32 start_chk_dly, u32 start_chk_en)
{
	dspo_dev[sel]->dma_tcon_ctrl.bits.dma_start_check = start_chk_dly;
	dspo_dev[sel]->dma_tcon_ctrl.bits.dma_start_check_en = start_chk_en;
}

void dspo_tcon_manu_set(u32 sel, u32 manu_mode, u32 manu_en)
{
	dspo_dev[sel]->dma_tcon_ctrl.bits.tcon_manu = manu_mode;
	dspo_dev[sel]->dma_tcon_ctrl.bits.tcon_manu_en = manu_en;
}

void dspo_work_mode_set(u32 sel, u32 mode)
{
	dspo_dev[sel]->dma_tcon_ctrl.bits.tcon_tri_mode = mode;
	if (mode == 1) {
		dspo_dev[sel]->dma_tcon_ctrl.bits.tcon_manu = 1;
		dspo_dev[sel]->dma_tcon_ctrl.bits.tcon_manu_en = 1;
	}
}

void dspo_tcon_tri_on(u32 sel)
{
	dspo_dev[sel]->dma_tcon_ctrl.bits.tcon_tri = 1;
}

void dspo_tcon_tri_off(u32 sel)
{
	dspo_dev[sel]->dma_tcon_ctrl.bits.tcon_tri = 0;
}

void dspo_dma_size_set(u32 sel, u32 x, u32 y)
{
	dspo_dev[sel]->dma_size.bits.x = x;
	dspo_dev[sel]->dma_size.bits.y = y;
}

void dspo_dma_y_addr_set(u32 sel, u32 addr)
{
	dspo_dev[sel]->dma_y_base_addr.bits.buf0_addr = (addr >> 2);
}

void dspo_dma_uv_addr_set(u32 sel, u32 addr)
{
	dspo_dev[sel]->dma_crcb_base_addr.bits.buf1_addr = (addr >> 2);
}

void dspo_dma_buf_line_stride_set(u32 sel, u32 y_line_stride,
				  u32 uv_line_stride)
{
	dspo_dev[sel]->dma_buf_line_stride.bits.buf0_line_stride =
	    y_line_stride;
	dspo_dev[sel]->dma_buf_line_stride.bits.buf1_line_stride =
	    uv_line_stride;
}

void dspo_desc_base_addr_set(u32 sel, u32 addr)
{
	dspo_dev[sel]->desc_base_addr.bits.desc_base_addr = addr;
}

void dspo_ceu_enable(u32 sel, u32 en)
{
	dspo_dev[sel]->csc_ceu_ctl.bits.ceu_en = en;
}


void dspo_ceu_mul_val_set(u32 sel, u32 rr, u32 rg, u32 rb, u32 gr, u32 gg,
			  u32 gb, u32 br, u32 bg, u32 bb)
{
	dspo_dev[sel]->csc_ceu_coef_mul_rr.bits.ceu_coef_mul_val = rr;
	dspo_dev[sel]->csc_ceu_coef_mul_rg.bits.ceu_coef_mul_val = rg;
	dspo_dev[sel]->csc_ceu_coef_mul_rb.bits.ceu_coef_mul_val = rb;
	dspo_dev[sel]->csc_ceu_coef_mul_gr.bits.ceu_coef_mul_val = gr;
	dspo_dev[sel]->csc_ceu_coef_mul_gg.bits.ceu_coef_mul_val = gg;
	dspo_dev[sel]->csc_ceu_coef_mul_gb.bits.ceu_coef_mul_val = gb;
	dspo_dev[sel]->csc_ceu_coef_mul_br.bits.ceu_coef_mul_val = br;
	dspo_dev[sel]->csc_ceu_coef_mul_bg.bits.ceu_coef_mul_val = bg;
	dspo_dev[sel]->csc_ceu_coef_mul_bb.bits.ceu_coef_mul_val = bb;
}

void dspo_ceu_add_val_set(u32 sel, u32 rc, u32 gc, u32 bc)
{
	dspo_dev[sel]->csc_ceu_coef_rc.bits.ceu_coef_add_val = rc;
	dspo_dev[sel]->csc_ceu_coef_gc.bits.ceu_coef_add_val = gc;
	dspo_dev[sel]->csc_ceu_coef_bc.bits.ceu_coef_add_val = bc;
}

void dspo_ceu_range_set(u32 sel, u32 ymax, u32 ymin, u32 umax, u32 umin,
			u32 vmax, u32 vmin)
{
	dspo_dev[sel]->csc_ceu_coef_rang_y.bits.ceu_coef_range_max = ymax;
	dspo_dev[sel]->csc_ceu_coef_rang_y.bits.ceu_coef_range_min = ymin;
	dspo_dev[sel]->csc_ceu_coef_rang_u.bits.ceu_coef_range_max = umax;
	dspo_dev[sel]->csc_ceu_coef_rang_u.bits.ceu_coef_range_min = umin;
	dspo_dev[sel]->csc_ceu_coef_rang_v.bits.ceu_coef_range_max = vmax;
	dspo_dev[sel]->csc_ceu_coef_rang_v.bits.ceu_coef_range_min = vmin;
}

void dspo_dma_mode_line_buf_range_set(u32 sel, u32 x)
{
	dspo_dev[sel]->line_buf_rang0.bits.dma_mode_lb_y_cap = x;
	dspo_dev[sel]->line_buf_rang1.bits.dma_mode_lb_uv_cap = x;
}

u32 dspo_is_line_buf_ready(u32 sel)
{
	return dspo_dev[sel]->line_buf_rang3.bits.line_buf_cap_rdy;
}

void dspo_rgb2yuv(u32 sel)
{
    u32 CEU_R_Para[6] = {0x0041, 0x0081, 0x0019, 0x00100, 0x00, 0xFF};
    u32 CEU_G_Para[6] = {0x1025, 0x104a, 0x0070, 0x00800, 0x00, 0xFF};
    u32 CEU_B_Para[6] = {0x0070, 0x107e, 0x1012, 0x00800, 0x00, 0xFF};

    dspo_ceu_mul_val_set(sel, CEU_R_Para[0], CEU_R_Para[1], CEU_R_Para[2],
			 CEU_G_Para[0], CEU_G_Para[1], CEU_G_Para[2],
			 CEU_B_Para[0], CEU_B_Para[1], CEU_B_Para[2]);
    dspo_ceu_add_val_set(sel, CEU_R_Para[3], CEU_G_Para[3], CEU_G_Para[3]);
    dspo_ceu_range_set(sel, CEU_R_Para[5], CEU_R_Para[4], CEU_G_Para[5],
		       CEU_G_Para[4], CEU_B_Para[5], CEU_B_Para[4]);
    dspo_ceu_enable(sel, 1);
}

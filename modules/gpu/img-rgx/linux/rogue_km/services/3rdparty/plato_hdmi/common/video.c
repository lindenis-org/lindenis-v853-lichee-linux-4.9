/*************************************************************************/ /*!
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@License        Dual MIT/GPLv2

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public License Version 2 ("GPL") in which case the provisions
of GPL are applicable instead of those above.

If you wish to allow use of your version of this file only under the terms of
GPL, and not to allow others to use your version of this file under the terms
of the MIT license, indicate your decision by deleting the provisions above
and replace them with the notice and other provisions required by GPL as set
out in the file called "GPL-COPYING" included in this distribution. If you do
not delete the provisions above, a recipient may use your version of this file
under the terms of either the MIT license or GPL.

This License is also included in this distribution in the file called
"MIT-COPYING".

EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

@Copyright      Portions Copyright (c) Synopsys Ltd. All Rights Reserved
@License        Synopsys Permissive License

The Synopsys Software Driver and documentation (hereinafter "Software")
is an unsupported proprietary work of Synopsys, Inc. unless otherwise
expressly agreed to in writing between  Synopsys and you.

The Software IS NOT an item of Licensed Software or Licensed Product under
any End User Software License Agreement or Agreement for Licensed Product
with Synopsys or any supplement thereto.  Permission is hereby granted,
free of charge, to any person obtaining a copy of this software annotated
with this license and the Software, to deal in the Software without
restriction, including without limitation the rights to use, copy, modify,
merge, publish, distribute, sublicense, and/or sell copies of the Software,
and to permit persons to whom the Software is furnished to do so, subject
to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THIS SOFTWARE IS BEING DISTRIBUTED BY SYNOPSYS SOLELY ON AN "AS IS" BASIS
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE HEREBY DISCLAIMED. IN NO EVENT SHALL SYNOPSYS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT     LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE.
*/ /**************************************************************************/

#include "video.h"
#include "phy.h"
#include "plato_top_regs.h"
#include <linux/delay.h>

//
// Pixel clock and core clock are driven by same PLL.
// For a given pixel clock find a list of core clocks that match.
// Print these out for the user's information.
static void FindExactCoreClocks(HDMI_DEVICE* pvDevice, IMG_UINT32 ui32PixelClock)
{
    // divide by 3 up to 48, or 4 up to 64 are also valid, but VideoEnablePixelClock doesn't use this.
    #define VALID_PIXDIV(d) (((d) <= 16) || (((d) <= 32) && !((d) & 1)))
    const IMG_UINT32 MinPLL =  700000000;  // 700MHz
    const IMG_UINT32 MaxPLL = 1650000000;  // 1.65GHz
    const IMG_UINT32 MinCoreClk = 396000000;  // 396MHz
    const IMG_UINT32 MaxCoreClk = 600000000;  // 600MHz
    const IMG_UINT32 mindiv = MinPLL / ui32PixelClock;
    const IMG_UINT32 maxdiv = MaxPLL / ui32PixelClock;
    #define MaxStoredClocks 10
    IMG_UINT32 ValidPixelClocks[MaxStoredClocks+1] = { 0 };
    IMG_UINT32 vpi = 0;
    IMG_UINT32 pixdiv;

    for (pixdiv = mindiv; pixdiv <= maxdiv; ++pixdiv)
    {
        IMG_UINT32 pll;
        IMG_UINT32 d;

        if (!VALID_PIXDIV(pixdiv))
            continue;
        pll = ui32PixelClock * pixdiv;
        if (pll < MinPLL)
            continue;
        if (pll > MaxPLL)
            break;
        // find nearest GPU clock to current one for this PLL speed
        for (d = 1; d <= 4; ++d)
        {
            IMG_UINT32 gpuclk = pll / d;
            if (gpuclk < MinCoreClk)
                break;
            if (gpuclk > MaxCoreClk)
                continue;
            if (vpi < MaxStoredClocks)
            {
                // simple insertion sort
                IMG_UINT32 i, j;
                for (i = 0; i < vpi; ++i)
                {
                    if (ValidPixelClocks[i] == gpuclk)
                        break;      // already present
                    if (gpuclk < ValidPixelClocks[i])
                    {
                        for (j = vpi++; j > i; --j)
                            ValidPixelClocks[j] = ValidPixelClocks[j-1];
                        ValidPixelClocks[i] = gpuclk;
                        break;
                    }
                }
                if (i == vpi)
                    ValidPixelClocks[vpi++] = gpuclk;
            }
        }
    }
    if (vpi > 0)
    {
        IMG_UINT32 i;
        HDMI_LOG_ALWAYS(" Current core clock %u.%03uMHz: core clocks to achieve exact pixel clock %ukHz:\n",
                                pvDevice->ui32CoreClockSpeed/1000000, (pvDevice->ui32CoreClockSpeed%1000000)/1000, ui32PixelClock/1000);
        for (i = 0; i < vpi; ++i)
            HDMI_LOG_ALWAYS("\t%u\n", ValidPixelClocks[i]);
    }
}


static int SetPlatoPDPClockDivider(HDMI_DEVICE* pvDevice, IMG_UINT32 pll_clock, IMG_UINT32 pixel_clock)
{
    // max can probably go higher if we need to in future. We need about 300MHz for 4Kx2K@30 monitors
    const IMG_UINT32 MaxPDPClk = (450*1000*1000)+1;
    IMG_UINT32 best_clk = MaxPDPClk;
    IMG_UINT32 reg_value = 0;
    IMG_UINT32 min_clk;
    IMG_UINT32 d0, d1;

    // we always want PDP core to be >= PLATO_MIN_PDP_CLOCK_SPEED
    min_clk = (pixel_clock > PLATO_MIN_PDP_CLOCK_SPEED) ? pixel_clock : PLATO_MIN_PDP_CLOCK_SPEED;

    // we have 2x 2-bit dividers for Plato PDP core clock.
    // start from largest dividers to get to lowest PDP core clock >= pixel clock.
    for (d0 = 4; d0 >= 1; --d0)
    {
        for (d1 = d0; d1 >= 1; --d1)
        {
            IMG_UINT32 pdp_clk = pll_clock / (d0*d1);
            if (pdp_clk >= best_clk)
                break;	// next divider in loop only makes it larger again
            if (pdp_clk >= min_clk)
            {
                reg_value = ((d0-1) << PLATO_CR_PDPV0_DIV_0_SHIFT) | ((d1-1) << PLATO_CR_PDPV1_DIV_0_SHIFT);
                best_clk = pdp_clk;
            }
        }
    }

    if (best_clk < MaxPDPClk)
    {
        // turn off the clock first
        OSWriteHWReg32(pvDevice->pvTopRegCpuVAddr, PLATO_TOP_CR_PDP_CLK_CTRL, 0);
        TOP_REG_POLL(pvDevice->pvTopRegCpuVAddr,   PLATO_TOP_CR_PDP_CLK_CTRL, 0, 0);

        reg_value |= (1 << PLATO_CR_PDPG_GATE_EN_SHIFT);
        OSWriteHWReg32(pvDevice->pvTopRegCpuVAddr, PLATO_TOP_CR_PDP_CLK_CTRL, reg_value);
        TOP_REG_POLL(pvDevice->pvTopRegCpuVAddr,   PLATO_TOP_CR_PDP_CLK_CTRL, reg_value, reg_value);

        HDMI_LOG_ALWAYS(" PDP clock speed set to %u\n", best_clk);
    }
    else
        HDMI_LOG_ALWAYS(" ERROR: Cannot set PDP core clock for PLL %u, pixel clock %u\n", pll_clock, pixel_clock);

    return (best_clk < MaxPDPClk);
}

/*
 * Pixel Clock setup
 */
static void VideoEnablePixelClock(HDMI_DEVICE * pvDevice, IMG_UINT32 ui32PixelClock)
{
    IMG_UINT32 ui32Div0;
    IMG_UINT32 ui32Div1;
    IMG_UINT32 ui32RegVal;
    IMG_UINT32 ui32PLLClock = pvDevice->ui32PLLClockSpeed;
    ui32PixelClock *= 10000;

    /*
     * Obtain divisor, round to nearest.
     * TODO: Shouldn't we round to ceiling here?
     */
    ui32Div1 = (ui32PLLClock + ui32PixelClock / 2) / ui32PixelClock;

    if (ui32Div1 <= 32)
    {
        if (ui32Div1 < 17)
        {
            ui32Div0 = 0;
            ui32Div1--;
        }
        else
        {
            ui32Div0 = 1;
            ui32Div1 /= 2;
            ui32Div1--;
        }
    }
    else
    {
        HDMI_DEBUG_PRINT("- %s: Cannot correctly set up dividers to set pixel clock to requested value! Setting dividers to maximum values ", __func__);
        ui32Div0 = 1;
        ui32Div1 = 15;
    }

    // now we know the actual pixel clock ensure the PDP core clock is fast enough.
    SetPlatoPDPClockDivider(pvDevice, ui32PLLClock, ui32PLLClock / ((ui32Div1+1) * (ui32Div0+1)));

    // and set the pixel clock after the PDP core clock.
    ui32RegVal = (PLATO_CR_HDMIG_GATE_EN_MASK | (ui32Div1 << PLATO_CR_HDMIV1_DIV_0_SHIFT) | (ui32Div0 << PLATO_CR_HDMIV0_DIV_0_SHIFT));

    OSWriteHWReg32(pvDevice->pvTopRegCpuVAddr, PLATO_TOP_CR_HDMI_CLK_CTRL, ui32RegVal);
    TOP_REG_POLL(pvDevice->pvTopRegCpuVAddr, PLATO_TOP_CR_HDMI_CLK_CTRL, ui32RegVal, ui32RegVal);

    HDMI_DEBUG_PRINT("- %s: Setting dividers %d,%d for requested PIXEL clock %ukHz\n", __func__,
                                            ui32Div0+1, ui32Div1+1, ui32PixelClock/1000);
    {
        // Always print out some log information about clock speeds.
        // Consider match exact if within 10000Hz (EDID is in units of 10000Hz)
        IMG_UINT32 diff = ui32PixelClock - ui32PLLClock/((ui32Div0+1)*(ui32Div1+1));
        if ((diff < 10000) || (diff > 0xFFFFD8F0))
            HDMI_LOG_ALWAYS(" Exact pixel clock of %ukHz achieved.\n", ui32PixelClock/1000);
        else
            HDMI_LOG_ALWAYS(" Requested pixel clock %ukHz, actual pixel clock %ukHz.",
                                                ui32PixelClock/1000, ui32PLLClock/(1000*(ui32Div0+1)*(ui32Div1+1)));
    }

    FindExactCoreClocks(pvDevice, ui32PixelClock);
}

/*

Sets up Video Packetizer registers

*/
static bool VideoConfigureVP(HDMI_DEVICE * pvDevice)
{
    VIDEO_PARAMS * pParams = &pvDevice->videoParams;
    IMG_UINT8 regValue = 0;
    IMG_UINT8 outputSelect = 0;
    IMG_UINT8 remapSize = 0;
    IMG_UINT8 colorDepth = 0;

    HDMI_CHECKPOINT;

    if (pParams->mEncodingOut == ENCODING_RGB ||
        pParams->mEncodingOut == ENCODING_YCC444 ||
        pParams->mEncodingOut == ENCODING_YCC420)
    {
        if (pParams->mColorResolution == 0)
            outputSelect = 3;
        else if (pParams->mColorResolution == 8)
        {
            colorDepth = 4;
            outputSelect = 3;
        }
        else if (pParams->mColorResolution == 10)
            colorDepth = 5;
        else if (pParams->mColorResolution == 12)
            colorDepth = 6;
        else if (pParams->mColorResolution == 16)
            colorDepth = 7;
        else
        {
            HDMI_ERROR_PRINT("- %s: Found unsupported color resolution (%d)", __func__, pParams->mColorResolution);
            return false;
        }
    }
    else if (pParams->mEncodingOut == ENCODING_YCC422)
    {
        if ((pParams->mColorResolution == 8)
                || (pParams->mColorResolution == 0))
            remapSize = 0;
        else if (pParams->mColorResolution == 10)
            remapSize = 1;
        else if (pParams->mColorResolution == 12)
            remapSize = 2;
        else
        {
            HDMI_ERROR_PRINT("- %s: Invalid color remap size: %d", __func__, pParams->mColorResolution);
            return false;
        }
        outputSelect = 1;
    }
    else
    {
        HDMI_ERROR_PRINT("- %s: Invalid output encoding type: %d", __func__, pParams->mEncodingOut);
        return false;
    }

    HDMI_DEBUG_PRINT(" - %s: outputSelect: %d, colorDepth: %d\n", __func__, outputSelect, colorDepth);

    regValue = SET_FIELD(HDMI_VP_PR_CD_COLOR_DEPTH_START,
        HDMI_VP_PR_CD_COLOR_DEPTH_MASK,
        colorDepth) |
    SET_FIELD(HDMI_VP_PR_CD_DESIRED_PR_FACTOR_START,
        HDMI_VP_PR_CD_DESIRED_PR_FACTOR_MASK,
        pParams->mPixelRepetitionFactor);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_VP_PR_CD_OFFSET, regValue);

    regValue = SET_FIELD(HDMI_VP_STUFF_IDEFAULT_PHASE_START,
        HDMI_VP_STUFF_IDEFAULT_PHASE_MASK,
        pParams->mPixelPackingDefaultPhase);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_VP_STUFF_OFFSET, regValue);

    regValue = SET_FIELD(HDMI_VP_REMAP_YCC422_SIZE_START,
        HDMI_VP_REMAP_YCC422_SIZE_MASK,
        remapSize);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_VP_REMAP_OFFSET, regValue);

    /* Grabbed from reference driver */
    regValue = 0;
    if (outputSelect == 0)
    {
        /* pixel packing */
        regValue = SET_FIELD(HDMI_VP_CONF_PP_EN_START,
            HDMI_VP_CONF_PP_EN_MASK,
            1);
    }
    else if (outputSelect == 1)
    {
        /* YCC422 */
        regValue = SET_FIELD(HDMI_VP_CONF_YCC422_EN_START,
            HDMI_VP_CONF_YCC422_EN_MASK,
            1);
    }
    else if (outputSelect == 2 || outputSelect == 3)
    {
        /* Enable bypass */
        regValue = SET_FIELD(HDMI_VP_CONF_BYPASS_EN_START,
            HDMI_VP_CONF_BYPASS_EN_MASK,
            1) |
	    SET_FIELD(HDMI_VP_CONF_BYPASS_SELECT_START,
	        HDMI_VP_CONF_BYPASS_SELECT_MASK,
	        1);

    }

    regValue |= SET_FIELD(HDMI_VP_CONF_OUTPUT_SELECTOR_START,
        HDMI_VP_CONF_OUTPUT_SELECTOR_MASK,
        outputSelect);

    #if defined(EMULATOR)
    regValue |= SET_FIELD(HDMI_VP_CONF_BYPASS_EN_START,
        HDMI_VP_CONF_BYPASS_EN_MASK,
        1) |
    SET_FIELD(HDMI_VP_CONF_BYPASS_SELECT_START,
        HDMI_VP_CONF_BYPASS_SELECT_MASK,
        1);
    #endif
    /* pixel packing */
    regValue |= SET_FIELD(HDMI_VP_CONF_PP_EN_START,
        HDMI_VP_CONF_PP_EN_MASK,
        1);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_VP_CONF_OFFSET, regValue);

    /* YCC422 and pixel packing stuffing */
    regValue = SET_FIELD(HDMI_VP_STUFF_IDEFAULT_PHASE_START,
        HDMI_VP_STUFF_IDEFAULT_PHASE_MASK,
        pParams->mPixelPackingDefaultPhase) |
    SET_FIELD(HDMI_VP_STUFF_YCC422_STUFFING_START,
        HDMI_VP_STUFF_YCC422_STUFFING_MASK,
        1) |
    SET_FIELD(HDMI_VP_STUFF_PP_STUFFING_START,
        HDMI_VP_STUFF_PP_STUFFING_MASK,
        1);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_VP_STUFF_OFFSET, regValue);

    return true;
}

/*

Sets up Video Sampler registers

*/
static bool VideoConfigureVS(HDMI_DEVICE * pvDevice)
{
    VIDEO_PARAMS * pParams = &pvDevice->videoParams;
    IMG_UINT8 mapCode = 0;
    IMG_UINT8 regValue = 0;

    HDMI_CHECKPOINT;

	if (pParams->mEncodingIn == ENCODING_RGB ||
        pParams->mEncodingIn == ENCODING_YCC444 ||
        pParams->mEncodingIn == ENCODING_YCC420)
	{
		if (pParams->mColorResolution == 8 || pParams->mColorResolution == 0)
			mapCode = 1;
		else if (pParams->mColorResolution == 10)
			mapCode = 3;
		else if (pParams->mColorResolution == 12)
			mapCode = 5;
		else if (pParams->mColorResolution == 16)
			mapCode = 7;
		else
		{
			HDMI_ERROR_PRINT("- %s: Invalid color depth\n", __func__);
			return false;
		}
		mapCode += (pParams->mEncodingIn  == ENCODING_YCC444) ? 8 : 0;
	}
	else if (pParams->mEncodingIn == ENCODING_YCC422)
	{
		/* YCC422 mapping is discontinued - only map 1 is supported */
		if (pParams->mColorResolution == 12)
			mapCode = 18;
		else if (pParams->mColorResolution == 10)
			mapCode = 20;
		else if ((pParams->mColorResolution == 8)
				|| (pParams->mColorResolution == 0))
			mapCode = 22;
		else
		{
			HDMI_ERROR_PRINT("- %s: Invalid color remap size: %d", __func__, pParams->mColorResolution);
			return false;
		}
	}
	else
	{
		HDMI_ERROR_PRINT("- %s: Invalid input encoding type: %d", __func__, pParams->mEncodingOut);
		return false;
    }

    regValue = SET_FIELD(HDMI_TX_INVID0_INTERNAL_DE_GEN_START, HDMI_TX_INVID0_INTERNAL_DE_GEN_MASK, 0);
    regValue |= SET_FIELD(HDMI_TX_INVID0_VIDEO_MAPPING_START, HDMI_TX_INVID0_VIDEO_MAPPING_MASK, mapCode);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_TX_INVID0_OFFSET, regValue);

    #if !defined(EMULATOR)
    /* The following is done in the ref driver */

    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_TX_GYDATA0_OFFSET, 0);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_TX_GYDATA1_OFFSET, 0);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_TX_RCRDATA0_OFFSET, 0);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_TX_RCRDATA1_OFFSET, 0);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_TX_BCBDATA0_OFFSET, 0);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_TX_BCBDATA1_OFFSET, 0);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_TX_INSTUFFING_OFFSET, 0x7);

    #endif

    return true;
}

static bool VideoConfigureFC(HDMI_DEVICE * pvDevice)
{
    IMG_UINT32 regValue = 0;
    VIDEO_PARAMS * pParams = &pvDevice->videoParams;
    DTD * pDtd = &pParams->mDtdList[pParams->mDtdActiveIndex];

    /* Input video configuration */
    regValue = SET_FIELD(HDMI_FC_INVIDCONF_VSYNC_IN_POLARITY_START,
            HDMI_FC_INVIDCONF_VSYNC_IN_POLARITY_MASK,
            pDtd->mVSyncPolarity) |
        SET_FIELD(HDMI_FC_INVIDCONF_HSYNC_IN_POLARITY_START,
            HDMI_FC_INVIDCONF_HSYNC_IN_POLARITY_MASK,
            pDtd->mHSyncPolarity) |
        SET_FIELD(HDMI_FC_INVIDCONF_DE_IN_POLARITY_START,
            HDMI_FC_INVIDCONF_DE_IN_POLARITY_MASK,
            pParams->mDataEnablePolarity) |
        SET_FIELD(HDMI_FC_INVIDCONF_R_V_BLANK_IN_OSC_START,
            HDMI_FC_INVIDCONF_R_V_BLANK_IN_OSC_MASK,
            pDtd->mInterlaced) | // Need to check this out, ref driver sets interlaced for both
        SET_FIELD(HDMI_FC_INVIDCONF_IN_I_P_START,
            HDMI_FC_INVIDCONF_IN_I_P_MASK,
            pDtd->mInterlaced) |
        SET_FIELD(HDMI_FC_INVIDCONF_DVI_MODEZ_START,
            HDMI_FC_INVIDCONF_DVI_MODEZ_MASK,
            pParams->mHdmi);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr,
        HDMI_FC_INVIDCONF_OFFSET,
        regValue);

    /*
    Input HActive Pixels
    */
    regValue = SET_FIELD(HDMI_FC_INHACTIV0_H_IN_ACTIV_START,
        HDMI_FC_INHACTIV0_H_IN_ACTIV_MASK,
        pDtd->mHActive & 0xFF);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_FC_INHACTIV0_OFFSET, regValue);

    regValue = SET_FIELD(HDMI_FC_INHACTIV1_H_IN_ACTIV_UPPER_START,
        HDMI_FC_INHACTIV1_H_IN_ACTIV_UPPER_MASK,
        pDtd->mHActive >> 8);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_FC_INHACTIV1_OFFSET, regValue);

    /*
    Input VActive Pixels
    */
    regValue = SET_FIELD(HDMI_FC_INVACTIV0_V_IN_ACTIV_START,
        HDMI_FC_INVACTIV0_V_IN_ACTIV_MASK,
        pDtd->mVActive & 0xFF);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_FC_INVACTIV0_OFFSET, regValue);

    regValue = SET_FIELD(HDMI_FC_INVACTIV1_V_IN_ACTIV_UPPER_START,
        HDMI_FC_INVACTIV1_V_IN_ACTIV_UPPER_MASK,
        pDtd->mVActive >> 8);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_FC_INVACTIV1_OFFSET, regValue);

    /*
    Input HBlank Pixels
    */
    regValue = SET_FIELD(HDMI_FC_INHBLANK0_H_IN_BLANK_START,
        HDMI_FC_INHBLANK0_H_IN_BLANK_MASK,
        pDtd->mHBlanking & 0xFF);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_FC_INHBLANK0_OFFSET, regValue);

    regValue = SET_FIELD(HDMI_FC_INHBLANK1_H_IN_BLANK_UPPER_START,
        HDMI_FC_INHBLANK1_H_IN_BLANK_UPPER_MASK,
        pDtd->mHBlanking >> 8);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_FC_INHBLANK1_OFFSET, regValue);

    /*
    Input VBlank Pixels
    */
    regValue = SET_FIELD(HDMI_FC_INVBLANK_V_IN_BLANK_START,
        HDMI_FC_INVBLANK_V_IN_BLANK_MASK,
        pDtd->mVBlanking & 0xFF);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_FC_INVBLANK_OFFSET, regValue);

    /*
    Input HSync Front Porch (pixel clock cycles from "de" non active edge of the last "de" valid period)
    */
    regValue = SET_FIELD(HDMI_FC_HSYNCINDELAY0_H_IN_DELAY_START,
        HDMI_FC_HSYNCINDELAY0_H_IN_DELAY_MASK,
        pDtd->mHFrontPorchWidth & 0xFF);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_FC_HSYNCINDELAY0_OFFSET, regValue);

    regValue = SET_FIELD(HDMI_FC_HSYNCINDELAY1_H_IN_DELAY_UPPER_START,
        HDMI_FC_HSYNCINDELAY1_H_IN_DELAY_UPPER_MASK,
        pDtd->mHFrontPorchWidth >> 8);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_FC_HSYNCINDELAY1_OFFSET, regValue);

    /*
    Input VSync Front porch
    */
    regValue = SET_FIELD(HDMI_FC_VSYNCINDELAY_V_IN_DELAY_START,
        HDMI_FC_VSYNCINDELAY_V_IN_DELAY_MASK,
        pDtd->mVFrontPorchWidth & 0xFF);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_FC_VSYNCINDELAY_OFFSET, regValue);

    /*
    Input HSync pulse width
    */
    regValue = SET_FIELD(HDMI_FC_HSYNCINWIDTH0_H_IN_WIDTH_START,
        HDMI_FC_HSYNCINWIDTH0_H_IN_WIDTH_MASK,
        pDtd->mHSyncPulseWidth & 0xFF);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_FC_HSYNCINWIDTH0_OFFSET, regValue);

    regValue = SET_FIELD(HDMI_FC_HSYNCINWIDTH1_H_IN_WIDTH_UPPER_START,
        HDMI_FC_HSYNCINWIDTH1_H_IN_WIDTH_UPPER_MASK,
        pDtd->mHSyncPulseWidth >> 8);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_FC_HSYNCINWIDTH1_OFFSET, regValue);

    /*
    Input Vsync pulse width
    */
    regValue = SET_FIELD(HDMI_FC_VSYNCINWIDTH_V_IN_WIDTH_START,
        HDMI_FC_VSYNCINWIDTH_V_IN_WIDTH_MASK,
        pDtd->mVSyncPulseWidth & 0xFF);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_FC_VSYNCINWIDTH_OFFSET, regValue);

    return true;

}


static void VideoParamsInit(VIDEO_PARAMS * pParams)
{
    #if defined(EMULATOR)
    pParams->mHdmi = 1;
    pParams->mEncodingOut = ENCODING_RGB;
    pParams->mEncodingIn = ENCODING_RGB;
    pParams->mColorResolution = 10;
    pParams->mPixelRepetitionFactor = 0;
    pParams->mRgbQuantizationRange = 0;
    pParams->mPixelPackingDefaultPhase = 0;
    pParams->mColorimetry = 0;
    pParams->mScanInfo = 0;
    pParams->mActiveFormatAspectRatio = 8;
    pParams->mNonUniformScaling = 0;
    pParams->mExtColorimetry = ~0;
    pParams->mItContent = 0;
    pParams->mEndTopBar = ~0;
    pParams->mStartBottomBar = ~0;
    pParams->mEndLeftBar = ~0;
    pParams->mStartRightBar = ~0;
    pParams->mCscFilter = 0;
    pParams->mHdmiVideoFormat = 0;
    pParams->m3dStructure = 0;
    pParams->m3dExtData = 0;
    pParams->mHdmiVic = 0;
    pParams->mDtdIndex = 0;
    pParams->mDtdActiveIndex = 0;
    pParams->mDataEnablePolarity = 1;
    #else
    pParams->mEncodingOut = ENCODING_RGB;
    pParams->mEncodingIn = ENCODING_RGB;
    #if 1
    pParams->mHdmi = 0;
    pParams->mColorResolution = 8;
    #else
    pParams->mColorResolution = 10;
    pParams->mHdmi = 1;
    #endif
    pParams->mPixelRepetitionFactor = 0;
    pParams->mRgbQuantizationRange = 0;
    pParams->mPixelPackingDefaultPhase = 0;
    pParams->mColorimetry = 0;
    pParams->mScanInfo = 0;
    pParams->mActiveFormatAspectRatio = 8;
    pParams->mNonUniformScaling = 0;
    pParams->mExtColorimetry = ~0;
    pParams->mItContent = 0;
    pParams->mEndTopBar = ~0;
    pParams->mStartBottomBar = ~0;
    pParams->mEndLeftBar = ~0;
    pParams->mStartRightBar = ~0;
    pParams->mCscFilter = 0;
    pParams->mHdmiVideoFormat = 0;
    pParams->m3dStructure = 0;
    pParams->m3dExtData = 0;
    pParams->mHdmiVic = 0;
    pParams->mDtdIndex = 0;
    pParams->mDtdActiveIndex = 0;
    pParams->mDataEnablePolarity = 1;
    #endif
}

/*

Steps for Configuring video mode:

1. Power down Tx PHY
2. Set desired video mode (based on EDID sink information)
3. Power-on, PLL configuration, and reset Tx PHY
4. Wait for PHY lock to assert

*/
PVRSRV_ERROR VideoConfigureMode(HDMI_DEVICE * pvDevice)
{
    PVRSRV_ERROR status = PVRSRV_OK;

    HDMI_CHECKPOINT;

    HDMI_DEBUG_PRINT(" - %s: Configuring video mode for VIC %d\n", __func__,
        pvDevice->videoParams.mDtdList[pvDevice->videoParams.mDtdActiveIndex].mCode);

    VideoEnablePixelClock(pvDevice, pvDevice->videoParams.mDtdList[pvDevice->videoParams.mDtdActiveIndex].mPixelClock);

    /*

    Step D.2: Set desired video mode based on current VideoParams structure

    */
    if (!VideoConfigureVP(pvDevice))
    {
        status = PVRSRV_ERROR_INIT_FAILURE;
        goto EXIT;
    }

    if (!VideoConfigureVS(pvDevice))
    {
        status = PVRSRV_ERROR_INIT_FAILURE;
        goto EXIT;
    }

    if (!VideoConfigureFC(pvDevice))
    {
        status = PVRSRV_ERROR_INIT_FAILURE;
        goto EXIT;
    }

    /* Place phy in reset */
    PhyPowerDown(pvDevice);

    /* Audio N and CTS value */
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_AUD_N1_OFFSET, 0xE5);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_AUD_N2_OFFSET, 0x0F);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_AUD_N3_OFFSET, 0x00);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_AUD_CTS1_OFFSET, 0x19);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_AUD_CTS2_OFFSET, 0xD5);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_AUD_CTS3_OFFSET, 0x02);

    /* Setup PHY for intended video mode */
    PhyConfigureMode(pvDevice);

    /* Wait for PHY lock */
    status = PhyWaitLock(pvDevice);
    if (status != PVRSRV_OK)
    {
		goto EXIT;
    }

    /* From pdump */
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_FC_CTRLDUR_OFFSET, 0x0C);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_FC_EXCTRLDUR_OFFSET, 0x20);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_FC_EXCTRLSPAC_OFFSET, 0x01);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_FC_CH0PREAM_OFFSET, 0x0B);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_FC_CH1PREAM_OFFSET, 0x16);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_FC_CH2PREAM_OFFSET, 0x21);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_MC_CLKDIS_OFFSET,
        SET_FIELD(HDMI_MC_CLKDIS_HDCPCLK_DIS_START, HDMI_MC_CLKDIS_HDCPCLK_DIS_MASK, 1));

    EXIT:
    return status;
}

/*

See HDMI ref driver packets.c

*/
PVRSRV_ERROR VideoConfigureInfoframes(HDMI_DEVICE * pvDevice)
{
    VIDEO_PARAMS * pVideoParams = &pvDevice->videoParams;
    DTD * pDtd = &pVideoParams->mDtdList[pVideoParams->mDtdActiveIndex];
    IMG_UINT32 regValue;

    HDMI_CHECKPOINT;

    #if defined(EMULATOR)
    return PVRSRV_OK; // This breaks on emu
    #endif

    /* Only relevant for HDMI */
    if (pVideoParams->mHdmi == 0)
    {
        HDMI_DEBUG_PRINT("- %s: mHdmi not set, not configuring infoframes\n", __func__);
        return PVRSRV_OK;
    }

    /* AVI CONF0 setup */
    regValue = SET_FIELD(HDMI_FC_AVICONF0_RGBYCC_START, HDMI_FC_AVICONF0_RGBYCC_MASK, 0);
    regValue |= SET_FIELD(HDMI_FC_AVICONF0_SCAN_INFO_START,
        HDMI_FC_AVICONF0_SCAN_INFO_MASK,
        pVideoParams->mScanInfo);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_FC_AVICONF0_OFFSET, regValue);

    /* AVI CONF1 setup */
    regValue = SET_FIELD(HDMI_FC_AVICONF1_PIC_ASPECT_RATIO_START,
        HDMI_FC_AVICONF1_PIC_ASPECT_RATIO_START,
        (((pDtd->mHImageSize * 10) % pDtd->mVImageSize) > 5) ? 2 : 1); /* 16:9 or 4:3 */
    regValue |= SET_FIELD(HDMI_FC_AVICONF1_COLORIMETRY_START,
        HDMI_FC_AVICONF1_COLORIMETRY_MASK,
        pVideoParams->mColorimetry);
    regValue |= SET_FIELD(HDMI_FC_AVICONF1_ACTIVE_ASPECT_RATIO_START,
        HDMI_FC_AVICONF1_ACTIVE_ASPECT_RATIO_MASK,
        pVideoParams->mActiveFormatAspectRatio);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_FC_AVICONF1_OFFSET, regValue);

    /* Go back and set active format valid bit */
    regValue = SET_FIELD(HDMI_FC_AVICONF0_ACTIVE_FORMAT_PRESENT_START,
        HDMI_FC_AVICONF0_ACTIVE_FORMAT_PRESENT_MASK,
        1);
    HDMI_READ_MOD_WRITE(pvDevice->pvHDMIRegCpuVAddr, HDMI_FC_AVICONF0_OFFSET, regValue);

    /* AVI CONF2 setup */
    regValue = SET_FIELD(HDMI_FC_AVICONF2_IT_CONTENT_START,
        HDMI_FC_AVICONF2_IT_CONTENT_MASK,
        pVideoParams->mItContent);
    regValue |= SET_FIELD(HDMI_FC_AVICONF2_QUANTIZATION_RANGE_START,
        HDMI_FC_AVICONF2_QUANTIZATION_RANGE_MASK,
        pVideoParams->mRgbQuantizationRange);
    regValue |= SET_FIELD(HDMI_FC_AVICONF2_NON_UNIFORM_PIC_SCALING_START,
        HDMI_FC_AVICONF2_NON_UNIFORM_PIC_SCALING_MASK,
        pVideoParams->mNonUniformScaling);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_FC_AVICONF2_OFFSET, regValue);

    /* AVI VID setup */
    if (pDtd->mCode != -1)
    {
    	regValue = SET_FIELD(HDMI_FC_AVIVID_START, HDMI_FC_AVIVID_MASK, pDtd->mCode);
    }
    else
    {
    	regValue = 0;
    }
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_FC_AVIVID_OFFSET, regValue);

    /* Set horizontal bars to 0 */
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_FC_AVIETB0_OFFSET, 0);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_FC_AVIETB1_OFFSET, 0);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_FC_AVISBB0_OFFSET, 0);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_FC_AVISBB1_OFFSET, 0);
    HDMI_READ_MOD_WRITE(pvDevice->pvHDMIRegCpuVAddr, HDMI_FC_AVICONF0_OFFSET,
        SET_FIELD(HDMI_FC_AVICONF0_HBAR_VALID_START, HDMI_FC_AVICONF0_HBAR_VALID_MASK, 1));

    /* Set vertical bars to 0 */
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_FC_AVIELB0_OFFSET, 0);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_FC_AVIELB1_OFFSET, 0);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_FC_AVISRB0_OFFSET, 0);
    HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_FC_AVISRB1_OFFSET, 0);
    HDMI_READ_MOD_WRITE(pvDevice->pvHDMIRegCpuVAddr, HDMI_FC_AVICONF0_OFFSET,
        SET_FIELD(HDMI_FC_AVICONF0_VBAR_VALID_START, HDMI_FC_AVICONF0_VBAR_VALID_MASK, 1));

    /* Set out pixel repetition factor */
    HDMI_READ_MOD_WRITE(pvDevice->pvHDMIRegCpuVAddr, HDMI_FC_PRCONF_OFFSET,
        SET_FIELD(HDMI_FC_PRCONF_OUTPUT_PR_FACTOR_START,
            HDMI_FC_PRCONF_OUTPUT_PR_FACTOR_MASK,
            ((pDtd->mPixelRepetitionInput + 1) * (pVideoParams->mPixelRepetitionFactor + 1) - 1)));

    return PVRSRV_OK;
}

/*

Initialize routine will setup video mode for VGA DVI mode to
start EDID communication

*/
PVRSRV_ERROR VideoInitialize(HDMI_DEVICE * pvDevice)
{
    PVRSRV_ERROR status = PVRSRV_OK;

    HDMI_CHECKPOINT;

    VideoParamsInit(&pvDevice->videoParams);

    /* Configure based on video parameters in pvDevice (VGA/DVI on init) */
    if (VideoConfigureMode(pvDevice) != PVRSRV_OK)
    {
        status = PVRSRV_ERROR_INIT_FAILURE;
        goto EXIT;
    }

    /* Force video output on init */
    {
        HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_FC_DBGTMDS_2_OFFSET, 0x00); /* R */
        HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_FC_DBGTMDS_1_OFFSET, 0xFF); /* G */
        HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_FC_DBGTMDS_0_OFFSET, 0x00); /* B */

        HDMI_WRITE_CORE_REG(pvDevice->pvHDMIRegCpuVAddr, HDMI_FC_DBGFORCE_OFFSET,
            SET_FIELD(HDMI_FC_DBGFORCE_FORCE_VIDEO_START, HDMI_FC_DBGFORCE_FORCE_VIDEO_MASK, 0));
    }

    EXIT:

    return status;
}


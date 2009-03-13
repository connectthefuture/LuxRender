/***************************************************************************
 *   Copyright (C) 1998-2008 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of LuxRender.                                       *
 *                                                                         *
 *   Lux Renderer is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   Lux Renderer is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 *   This project is based on PBRT ; see http://www.pbrt.org               *
 *   Lux Renderer website : http://www.luxrender.net                       *
 ***************************************************************************/

// lampspectrum.cpp*
#include "lampspectrum.h"
#include "irregulardata.h"
#include "error.h"
#include "dynload.h"

#include "data/lamp_spect.h"

using namespace lux;

// LampSpectrumTexture Method Definitions
Texture<float> * LampSpectrumTexture::CreateFloatTexture(const Transform &tex2world,
		const TextureParams &tp) {
	return new IrregularDataFloatTexture<float>(1.f);
}

Texture<SWCSpectrum> * LampSpectrumTexture::CreateSWCSpectrumTexture(const Transform &tex2world,
		const TextureParams &tp) {
	std::string name = tp.FindString("name");
	const int wlcount = 250;

	float *wl;
	float *data;

	if(name == "Alcohol") {
		wl = lampspectrum_Alcohol_WL;
		data = lampspectrum_Alcohol_AP;
	}
	if(name == "AntiInsect") {
		wl = lampspectrum_AntiInsect_WL;
		data = lampspectrum_AntiInsect_AP;
	}
	if(name == "Ar") {
		wl = lampspectrum_Ar_WL;
		data = lampspectrum_Ar_AP;
	}
	if(name == "BLAU") {
		wl = lampspectrum_BLAU_WL;
		data = lampspectrum_BLAU_AP;
	}
	if(name == "BLNG") {
		wl = lampspectrum_BLNG_WL;
		data = lampspectrum_BLNG_AP;
	}
	if(name == "BLP") {
		wl = lampspectrum_BLP_WL;
		data = lampspectrum_BLP_AP;
	}
	if(name == "Butane") {
		wl = lampspectrum_Butane_WL;
		data = lampspectrum_Butane_AP;
	}
	if(name == "Candle") {
		wl = lampspectrum_Candle_WL;
		data = lampspectrum_Candle_AP;
	}
	if(name == "CarbonArc") {
		wl = lampspectrum_CarbonArc_WL;
		data = lampspectrum_CarbonArc_AP;
	}
	if(name == "Cd") {
		wl = lampspectrum_Cd_WL;
		data = lampspectrum_Cd_AP;
	}
	if(name == "CFL27K") {
		wl = lampspectrum_CFL27K_WL;
		data = lampspectrum_CFL27K_AP;
	}
	if(name == "CFL4K") {
		wl = lampspectrum_CFL4K_WL;
		data = lampspectrum_CFL4K_AP;
	}
	if(name == "CFL6K") {
		wl = lampspectrum_CFL6K_WL;
		data = lampspectrum_CFL6K_AP;
	}
	if(name == "CL42053") {
		wl = lampspectrum_CL42053_WL;
		data = lampspectrum_CL42053_AP;
	}
	if(name == "CobaltGlass") {
		wl = lampspectrum_CobaltGlass_WL;
		data = lampspectrum_CobaltGlass_AP;
	}
	if(name == "Daylight") {
		wl = lampspectrum_Daylight_WL;
		data = lampspectrum_Daylight_AP;
	}
	if(name == "FeCo") {
		wl = lampspectrum_FeCo_WL;
		data = lampspectrum_FeCo_AP;
	}
	if(name == "FL37K") {
		wl = lampspectrum_FL37K_WL;
		data = lampspectrum_FL37K_AP;
	}
	if(name == "FLAV17K") {
		wl = lampspectrum_FLAV17K_WL;
		data = lampspectrum_FLAV17K_AP;
	}
	if(name == "FLAV8K") {
		wl = lampspectrum_FLAV8K_WL;
		data = lampspectrum_FLAV8K_AP;
	}
	if(name == "FLBL") {
		wl = lampspectrum_FLBL_WL;
		data = lampspectrum_FLBL_AP;
	}
	if(name == "FLBLB") {
		wl = lampspectrum_FLBLB_WL;
		data = lampspectrum_FLBLB_AP;
	}
	if(name == "FLD2") {
		wl = lampspectrum_FLD2_WL;
		data = lampspectrum_FLD2_AP;
	}
	if(name == "GaPb") {
		wl = lampspectrum_GaPb_WL;
		data = lampspectrum_GaPb_AP;
	}
	if(name == "GreenLaser") {
		wl = lampspectrum_GreenLaser_WL;
		data = lampspectrum_GreenLaser_AP;
	}
	if(name == "GroLux") {
		wl = lampspectrum_GroLux_WL;
		data = lampspectrum_GroLux_AP;
	}
	if(name == "GRUN") {
		wl = lampspectrum_GRUN_WL;
		data = lampspectrum_GRUN_AP;
	}
	if(name == "HPM2") {
		wl = lampspectrum_HPM2_WL;
		data = lampspectrum_HPM2_AP;
	}
	if(name == "HPMFL1") {
		wl = lampspectrum_HPMFL1_WL;
		data = lampspectrum_HPMFL1_AP;
	}
	if(name == "HPMFL2") {
		wl = lampspectrum_HPMFL2_WL;
		data = lampspectrum_HPMFL2_AP;
	}
	if(name == "HPMFL2Glow") {
		wl = lampspectrum_HPMFL2Glow_WL;
		data = lampspectrum_HPMFL2Glow_AP;
	}
	if(name == "HPMFLCL42053") {
		wl = lampspectrum_HPMFLCL42053_WL;
		data = lampspectrum_HPMFLCL42053_AP;
	}
	if(name == "HPMFLCobaltGlass") {
		wl = lampspectrum_HPMFLCobaltGlass_WL;
		data = lampspectrum_HPMFLCobaltGlass_AP;
	}
	if(name == "HPMFLRedGlass") {
		wl = lampspectrum_HPMFLRedGlass_WL;
		data = lampspectrum_HPMFLRedGlass_AP;
	}
	if(name == "HPMSB") {
		wl = lampspectrum_HPMSB_WL;
		data = lampspectrum_HPMSB_AP;
	}
	if(name == "HPMSBFL") {
		wl = lampspectrum_HPMSBFL_WL;
		data = lampspectrum_HPMSBFL_AP;
	}
	if(name == "HPS") {
		wl = lampspectrum_HPS_WL;
		data = lampspectrum_HPS_AP;
	}
	if(name == "HPX") {
		wl = lampspectrum_HPX_WL;
		data = lampspectrum_HPX_AP;
	}
	if(name == "Incadescent1") {
		wl = lampspectrum_Incadescent1_WL;
		data = lampspectrum_Incadescent1_AP;
	}
	if(name == "Incadescent2") {
		wl = lampspectrum_Incadescent2_WL;
		data = lampspectrum_Incadescent2_AP;
	}
	if(name == "LCDS") {
		wl = lampspectrum_LCDS_WL;
		data = lampspectrum_LCDS_AP;
	}
	if(name == "LEDB") {
		wl = lampspectrum_LEDB_WL;
		data = lampspectrum_LEDB_AP;
	}
	if(name == "LPM2") {
		wl = lampspectrum_LPM2_WL;
		data = lampspectrum_LPM2_AP;
	}
	if(name == "LPS") {
		wl = lampspectrum_LPS_WL;
		data = lampspectrum_LPS_AP;
	}
	if(name == "MHD") {
		wl = lampspectrum_MHD_WL;
		data = lampspectrum_MHD_AP;
	}
	if(name == "MHN") {
		wl = lampspectrum_MHN_WL;
		data = lampspectrum_MHN_AP;
	}
	if(name == "MHSc") {
		wl = lampspectrum_MHSc_WL;
		data = lampspectrum_MHSc_AP;
	}
	if(name == "MHWWD") {
		wl = lampspectrum_MHWWD_WL;
		data = lampspectrum_MHWWD_AP;
	}
	if(name == "MPS") {
		wl = lampspectrum_MPS_WL;
		data = lampspectrum_MPS_AP;
	}
	if(name == "Ne") {
		wl = lampspectrum_Ne_WL;
		data = lampspectrum_Ne_AP;
	}
	if(name == "NeKrFL") {
		wl = lampspectrum_NeKrFL_WL;
		data = lampspectrum_NeKrFL_AP;
	}
	if(name == "NeXeFL1") {
		wl = lampspectrum_NeXeFL1_WL;
		data = lampspectrum_NeXeFL1_AP;
	}
	if(name == "NeXeFL2") {
		wl = lampspectrum_NeXeFL2_WL;
		data = lampspectrum_NeXeFL2_AP;
	}
	if(name == "OliveOil") {
		wl = lampspectrum_OliveOil_WL;
		data = lampspectrum_OliveOil_AP;
	}
	if(name == "PLANTA") {
		wl = lampspectrum_PLANTA_WL;
		data = lampspectrum_PLANTA_AP;
	}
	if(name == "Rb") {
		wl = lampspectrum_Rb_WL;
		data = lampspectrum_Rb_AP;
	}
	if(name == "RedGlass") {
		wl = lampspectrum_RedGlass_WL;
		data = lampspectrum_RedGlass_AP;
	}
	if(name == "RedLaser") {
		wl = lampspectrum_RedLaser_WL;
		data = lampspectrum_RedLaser_AP;
	}
	if(name == "SHPS") {
		wl = lampspectrum_SHPS_WL;
		data = lampspectrum_SHPS_AP;
	}
	if(name == "SS1") {
		wl = lampspectrum_SS1_WL;
		data = lampspectrum_SS1_AP;
	}
	if(name == "SS2") {
		wl = lampspectrum_SS2_WL;
		data = lampspectrum_SS2_AP;
	}
	if(name == "TV") {
		wl = lampspectrum_TV_WL;
		data = lampspectrum_TV_AP;
	}
	if(name == "UVA") {
		wl = lampspectrum_UVA_WL;
		data = lampspectrum_UVA_AP;
	}
	if(name == "Welsbach") {
		wl = lampspectrum_Welsbach_WL;
		data = lampspectrum_Welsbach_AP;
	}
	if(name == "Xe") {
		wl = lampspectrum_Xe_WL;
		data = lampspectrum_Xe_AP;
	}
	if(name == "XeI") {
		wl = lampspectrum_XeI_WL;
		data = lampspectrum_XeI_AP;
	}
	if(name == "Zn") {
		wl = lampspectrum_Zn_WL;
		data = lampspectrum_Zn_AP;
	}
	return new IrregularDataSpectrumTexture<SWCSpectrum>(wlcount, wl, data, 0.1);
}

static DynamicLoader::RegisterFloatTexture<LampSpectrumTexture> r1("lampspectrum");
static DynamicLoader::RegisterSWCSpectrumTexture<LampSpectrumTexture> r2("lampspectrum");

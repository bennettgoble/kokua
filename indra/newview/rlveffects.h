/**
 *
 * Copyright (c) 2021, Kitty Barnett
 *
 * The source code in this file is provided to you under the terms of the
 * GNU Lesser General Public License, version 2.1, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. Terms of the LGPL can be found in doc/LGPL-licence.txt
 * in this distribution, or online at http://www.gnu.org/licenses/lgpl-2.1.txt
 *
 * By copying, modifying or distributing this software, you acknowledge that
 * you have read and understood your obligations described above, and agree to
 * abide by those obligations.
 *
 */

#pragma once

#include "llvisualeffect.h"

#include <boost/variant.hpp>

 //
 // RLVa-specific
 //
typedef boost::variant<int, float, bool, LLVector3, LLVector3d, LLVector4, LLUUID> RlvBehaviourModifierValue;

// ============================================================================
// Forward declarations
//

class LLViewerFetchedTexture;

// ====================================================================================
// RlvSphereEffect class
//

class RlvSphereEffect : public LLVisualEffect
{
public:
    RlvSphereEffect(const LLUUID& idRlvObj);
    ~RlvSphereEffect();

public:
    void run(const LLVisualEffectParams* pParams) override;

//MK
    enum class ESphereMode { Blend = 0, Blur, BlurVariable, ChromaticAberration, Pixelate, Count };
    enum class ESphereOrigin { Avatar = 0, Camera, Count };
    enum class ESphereDistExtend { Max = 0x01, Min = 0x02, Both = 0x03 };

    void setActive(BOOL newval);
    void setMode(ESphereMode newval);
    void setOrigin(ESphereOrigin newval);
    void setParams(LLVector4 newval);
    void setDistanceMin(float newval);
    void setDistanceMax(float newval);
    void setDistExtend(ESphereDistExtend newval);
    void setValueMin(float newval);
    void setValueMax(float newval);
    void setTweenDuration(float newval);

    BOOL getActive();
    ESphereMode getMode();
    ESphereOrigin getOrigin();
    LLVector4 getParams();
    float getDistanceMin();
    float getDistanceMax();
    ESphereDistExtend getDistExtend();
    float getValueMin();
    float getValueMax();
    float getTweenDuration();

    // These booleans indicate if the corresponding member has been changed once since the creation of the object, this is useful for mixing values.
    BOOL m_nChangedOrigin;
    BOOL m_nChangedParams;
    BOOL m_nChangedDistExtend;
    BOOL m_nChangedDistanceMin;
    BOOL m_nChangedDistanceMax;
    BOOL m_nChangedValueMin;
    BOOL m_nChangedValueMax;
    BOOL m_nChangedTweenDuration;
//mk

protected:
    void renderPass(LLGLSLShader* pShader, const LLShaderEffectParams* pParams) const;
    void setShaderUniforms(LLGLSLShader* pShader);

    /*
     * Member variables
     */
protected:
//MK : moved the enums below to the public section above, plus add an "active" bit
    BOOL m_nActive;

//mk
////    enum class ESphereMode { Blend = 0, Blur, BlurVariable, ChromaticAberration, Pixelate, Count };
    ESphereMode   m_eMode;
////    enum class ESphereOrigin { Avatar = 0, Camera, Count };
    ESphereOrigin m_eOrigin;
    LLTweenableValueLerp<LLVector4> m_Params;
    LLTweenableValueLerp<float>     m_nDistanceMin;
    LLTweenableValueLerp<float>     m_nDistanceMax;
////    enum class ESphereDistExtend { Max = 0x01, Min = 0x02, Both = 0x03 };
    ESphereDistExtend m_eDistExtend;
    LLTweenableValueLerp<float>     m_nValueMin;
    LLTweenableValueLerp<float>     m_nValueMax;
    float                           m_nTweenDuration;
};

// ====================================================================================

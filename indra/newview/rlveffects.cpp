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

#include "llviewerprecompiledheaders.h"

#include "llagent.h"
#include "llfasttimer.h"
#include "llviewershadermgr.h"
#include "llviewertexturelist.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"
#include "pipeline.h"

#include "rlveffects.h"

// ====================================================================================
// RlvSphereEffect class
//

const int   c_SphereDefaultMode = 0;
const int   c_SphereDefaultOrigin = 0;
const float c_SphereDefaultColor[4] = { 0.0, 0.f, 0.f, 0.f };
const float c_SphereDefaultDistance = 0.0f;
const int   c_SphereDefaultDistanceExtend = 1;
const float c_SphereDefaultAlpha = 1.0f;

RlvSphereEffect::RlvSphereEffect(const LLUUID& idRlvObj)
    : LLVisualEffect(idRlvObj, EVisualEffect::RlvSphere, EVisualEffectType::PostProcessShader)
    , m_nActive(FALSE)
    , m_eMode((ESphereMode)c_SphereDefaultMode)
    , m_eOrigin((ESphereOrigin)c_SphereDefaultOrigin)
    , m_Params(LLVector4(c_SphereDefaultColor))
    , m_nDistanceMin(c_SphereDefaultDistance), m_nDistanceMax(c_SphereDefaultDistance)
    , m_eDistExtend((ESphereDistExtend)c_SphereDefaultDistanceExtend)
    , m_nValueMin(c_SphereDefaultAlpha), m_nValueMax(c_SphereDefaultAlpha)
    , m_nTweenDuration(0.f)
    , m_nChangedOrigin (FALSE)
    , m_nChangedParams(FALSE)
    , m_nChangedDistExtend(FALSE)
    , m_nChangedDistanceMin(FALSE)
    , m_nChangedDistanceMax(FALSE)
    , m_nChangedValueMin(FALSE)
    , m_nChangedValueMax(FALSE)
    , m_nChangedTweenDuration(FALSE)
{
}

RlvSphereEffect::~RlvSphereEffect()
{
}

//MK
const float epsilon = 0.00001f; // we need this to compare m_nTweenDuration with 0

void RlvSphereEffect::setActive(BOOL newval)
{
    m_nActive = newval;
}

void RlvSphereEffect::setMode(ESphereMode newval)
{
    m_eMode = newval;
}

void RlvSphereEffect::setOrigin(ESphereOrigin newval)
{
    m_eOrigin = newval;
    m_nChangedOrigin = TRUE;
}

void RlvSphereEffect::setParams(LLVector4 newval)
{
    if (std::abs (m_nTweenDuration) < epsilon) // is m_nTweenDuration equal to 0 ?
        m_Params = newval;
    else
        m_Params.start(newval, m_nTweenDuration);

    m_nChangedParams = TRUE;
}

void RlvSphereEffect::setDistanceMin(float newval)
{
    if (std::abs(m_nTweenDuration) < epsilon) // is m_nTweenDuration equal to 0 ?
        m_nDistanceMin = newval;
    else
        m_nDistanceMin.start(newval, m_nTweenDuration);

    m_nChangedDistanceMin = TRUE;
}

void RlvSphereEffect::setDistanceMax(float newval)
{
    if (std::abs(m_nTweenDuration) < epsilon) // is m_nTweenDuration equal to 0 ?
        m_nDistanceMax = newval;
    else
        m_nDistanceMax.start(newval, m_nTweenDuration);

    m_nChangedDistanceMax = TRUE;
}

void RlvSphereEffect::setDistExtend(ESphereDistExtend newval)
{
    m_eDistExtend = newval;
    m_nChangedDistExtend = TRUE;
}

void RlvSphereEffect::setValueMin(float newval)
{
    if (std::abs(m_nTweenDuration) < epsilon) // is m_nTweenDuration equal to 0 ?
        m_nValueMin = newval;
    else
        m_nValueMin.start(newval, m_nTweenDuration);

    m_nChangedValueMin = TRUE;
}

void RlvSphereEffect::setValueMax(float newval)
{
    if (std::abs(m_nTweenDuration) < epsilon) // is m_nTweenDuration equal to 0 ?
        m_nValueMax = newval;
    else
        m_nValueMax.start(newval, m_nTweenDuration);
    
    m_nChangedValueMax = TRUE;
}

void RlvSphereEffect::setTweenDuration(float newval)
{
    m_nTweenDuration = newval;
    m_nChangedTweenDuration = TRUE;

    // When we change the tween duration we need to reset all the values to make them use the new tween (that's for the specific case where a value is still tweening while the tween duration changes).
    setParams(getParams());
    setDistanceMin(getDistanceMin());
    setDistanceMax(getDistanceMax());
    setValueMin(getValueMin());
    setValueMax(getValueMax());
}

BOOL RlvSphereEffect::getActive()
{
    return m_nActive;
}

RlvSphereEffect::ESphereMode RlvSphereEffect::getMode()
{
    return m_eMode;
}

RlvSphereEffect::ESphereOrigin RlvSphereEffect::getOrigin()
{
    return m_eOrigin;
}

LLVector4 RlvSphereEffect::getParams()
{
    return m_Params.get();
}

float RlvSphereEffect::getDistanceMin()
{
    return m_nDistanceMin.get();
}

float RlvSphereEffect::getDistanceMax()
{
    return m_nDistanceMax.get();
}

RlvSphereEffect::ESphereDistExtend RlvSphereEffect::getDistExtend()
{
    return m_eDistExtend;
}

float RlvSphereEffect::getValueMin()
{
    return m_nValueMin.get();
}

float RlvSphereEffect::getValueMax()
{
    return m_nValueMax.get();
}

float RlvSphereEffect::getTweenDuration()
{
    return m_nTweenDuration;
}
//mk

void RlvSphereEffect::setShaderUniforms(LLGLSLShader* pShader)
{
    pShader->uniformMatrix4fv(LLShaderMgr::INVERSE_PROJECTION_MATRIX, 1, FALSE, get_current_projection().inverse().m);
    pShader->uniform2f(LLShaderMgr::DEFERRED_SCREEN_RES, gPipeline.mScreen.getWidth(), gPipeline.mScreen.getHeight());
    pShader->uniform1i(LLShaderMgr::RLV_EFFECT_MODE, llclamp((int)m_eMode, 0, (int)ESphereMode::Count));

    // Pass the sphere origin to the shader
    LLVector4 posSphereOrigin;
    switch (m_eOrigin)
    {
        case ESphereOrigin::Camera:
            posSphereOrigin.setVec(LLViewerCamera::instance().getOrigin(), 1.0f);
            break;
        case ESphereOrigin::Avatar:
        default:
//MK
            // If the origin is the avatar, center the sphere around the joint that is currently chosen by the user (head, pelvis, hand etc).
            // Unlike with @camdraw commands, here we do not need to restrict the joint to the head when in mouselook.
////            posSphereOrigin.setVec((isAgentAvatarValid()) ? gAgentAvatarp->getRenderPosition() : gAgent.getPositionAgent(), 1.0f);
            posSphereOrigin.setVec((isAgentAvatarValid()) ? gAgent.mRRInterface.getCamDistDrawFromJoint(FALSE)->getWorldPosition() : gAgent.getPositionAgent(), 1.0f);
//mk
            break;
    }
    glh::vec4f posSphereOriginGl(posSphereOrigin.mV);
    const glh::matrix4f& mvMatrix = gGLModelView;
    mvMatrix.mult_matrix_vec(posSphereOriginGl);
    pShader->uniform4fv(LLShaderMgr::RLV_EFFECT_PARAM1, 1, posSphereOriginGl.v);

    // Pack min/max distance and alpha together
    float nDistMin = m_nDistanceMin.get(), nDistMax = m_nDistanceMax.get();
    const glh::vec4f sphereParams(m_nValueMin.get(), nDistMin, m_nValueMax.get(), (nDistMax >= nDistMin) ? nDistMax : nDistMin);
    pShader->uniform4fv(LLShaderMgr::RLV_EFFECT_PARAM2, 1, sphereParams.v);

    // Pass dist extend
    int eDistExtend = (int)m_eDistExtend;
    pShader->uniform2f(LLShaderMgr::RLV_EFFECT_PARAM3, eDistExtend & (int)ESphereDistExtend::Min, eDistExtend & (int)ESphereDistExtend::Max);

    // Pass effect params
    const glh::vec4f effectParams(m_Params.get().mV);
    pShader->uniform4fv(LLShaderMgr::RLV_EFFECT_PARAM4, 1, effectParams.v);
}

void RlvSphereEffect::renderPass(LLGLSLShader* pShader, const LLShaderEffectParams* pParams) const
{
    if (pParams->m_pDstBuffer)
    {
        pParams->m_pDstBuffer->bindTarget();
    }
    else
    {
        gGLViewport[0] = gViewerWindow->getWorldViewRectRaw().mLeft;
        gGLViewport[1] = gViewerWindow->getWorldViewRectRaw().mBottom;
        gGLViewport[2] = gViewerWindow->getWorldViewRectRaw().getWidth();
        gGLViewport[3] = gViewerWindow->getWorldViewRectRaw().getHeight();
        glViewport(gGLViewport[0], gGLViewport[1], gGLViewport[2], gGLViewport[3]);
    }
    //RLV_ASSERT_DBG(pParams->m_pSrcBuffer);

    S32 nDiffuseChannel = pShader->enableTexture(LLShaderMgr::DEFERRED_DIFFUSE, pParams->m_pSrcBuffer->getUsage());
    if (nDiffuseChannel > -1)
    {
        pParams->m_pSrcBuffer->bindTexture(0, nDiffuseChannel);
        gGL.getTexUnit(nDiffuseChannel)->setTextureFilteringOption(LLTexUnit::TFO_POINT);
    }

    S32 nDepthChannel = pShader->enableTexture(LLShaderMgr::DEFERRED_DEPTH, gPipeline.mDeferredDepth.getUsage());
    if (nDepthChannel > -1)
    {
        gGL.getTexUnit(nDepthChannel)->bind(&gPipeline.mDeferredDepth, TRUE);
    }

    gGL.matrixMode(LLRender::MM_PROJECTION);
    gGL.pushMatrix();
    gGL.loadIdentity();
    gGL.matrixMode(LLRender::MM_MODELVIEW);
    gGL.pushMatrix();
    gGL.loadMatrix(gGLModelView);

    LLVector2 tc1(0, 0);
    LLVector2 tc2((F32)gPipeline.mScreen.getWidth() * 2, (F32)gPipeline.mScreen.getHeight() * 2);
    gGL.begin(LLRender::TRIANGLE_STRIP);
    gGL.texCoord2f(tc1.mV[0], tc1.mV[1]);
    gGL.vertex2f(-1, -1);
    gGL.texCoord2f(tc1.mV[0], tc2.mV[1]);
    gGL.vertex2f(-1, 3);
    gGL.texCoord2f(tc2.mV[0], tc1.mV[1]);
    gGL.vertex2f(3, -1);
    gGL.end();

    gGL.matrixMode(LLRender::MM_PROJECTION);
    gGL.popMatrix();
    gGL.matrixMode(LLRender::MM_MODELVIEW);
    gGL.popMatrix();

    pShader->disableTexture(LLShaderMgr::DEFERRED_DIFFUSE, pParams->m_pSrcBuffer->getUsage());
    pShader->disableTexture(LLShaderMgr::DEFERRED_DEPTH, gPipeline.mDeferredDepth.getUsage());

    if (pParams->m_pDstBuffer)
    {
        pParams->m_pDstBuffer->flush();
    }
}

LLTrace::BlockTimerStatHandle FTM_RLV_EFFECT_SPHERE("Post-process (RLVa sphere)");

void RlvSphereEffect::run(const LLVisualEffectParams* pParams)
{
    LL_RECORD_BLOCK_TIME(FTM_RLV_EFFECT_SPHERE);
    LLGLDepthTest depth(GL_FALSE, GL_FALSE);

    gRlvSphereProgram.bind();
    setShaderUniforms(&gRlvSphereProgram);

    const LLShaderEffectParams* pShaderParams = static_cast<const LLShaderEffectParams*>(pParams);
    switch (m_eMode)
    {
        case ESphereMode::Blend:
        case ESphereMode::ChromaticAberration:
        case ESphereMode::Pixelate:
            renderPass(&gRlvSphereProgram, pShaderParams);
            break;
        case ESphereMode::Blur:
        case ESphereMode::BlurVariable:
            gRlvSphereProgram.uniform2f(LLShaderMgr::RLV_EFFECT_PARAM5, 1.f, 0.f);
            renderPass(&gRlvSphereProgram, pShaderParams);
            gRlvSphereProgram.uniform2f(LLShaderMgr::RLV_EFFECT_PARAM5, 0.f, 1.f);
            renderPass(&gRlvSphereProgram, pShaderParams);
            break;
        default:
            llassert(true);
    }

    gRlvSphereProgram.unbind();
}

// ====================================================================================

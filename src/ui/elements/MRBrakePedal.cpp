//
// Created by lance on 2022/10/12.
//

#include "MRBrakePedal.h"

namespace morrow
{
MRBrakePedalSharedPtr
MRBrakePedal::create()
{
    return std::shared_ptr<MRBrakePedal>(new MRBrakePedal());
}

MRBrakePedal::MRBrakePedal()
{
    setWidgetType("MRBrakePedal");
    m_material->setShader("brake_pedal");
}

// void MRBrakePedal::draw(FrameStateSharedPtr frameState)
// {
//     m_brakePedalTexture->render(frameState);
//     m_grayTexture->render(frameState);
//     m_whiteTexture->render(frameState);
//     m_brakePedalTexture->bindTexture(0);
//     m_grayTexture->bindTexture(1);
//     m_whiteTexture->bindTexture(2);
//
//     auto defaultShader = getDefaultShader();
//     defaultShader->use();
//     defaultShader->setMat4("u_mvp", getModelViewProjection(frameState->camera->getProjectionView()));
//     defaultShader->setInt("u_brakePedalTexture", 0);
//     defaultShader->setInt("u_grayTexture", 1);
//     defaultShader->setInt("u_whiteTexture", 2);
//     defaultShader->setFloat("u_timeDelta", m_time);
//     defaultShader->setFloat("u_duration", m_options->duration);
//     defaultShader->setFloat("u_alpha", Math::clamp(frameState->screenAlpha * getAlpha(), 0.0f, 1.0f));
//     getDefaultVertexArray()->draw();
// }

}

#include "PressableButton.h"

#include <DirectXCollision.h>

#include <unordered_set>

using namespace Microsoft::MixedReality::HandUtils;
using namespace DirectX;
using namespace std;

PressableButton::PressableButton(
    DirectX::FXMVECTOR position, DirectX::FXMVECTOR orientation, float width, float height, 
    float maxPushDistance, float pressedDistance, float releasedDistance)
    : m_restPosition(position)
    , m_orientation(orientation)
    , m_width(width)
    , m_height(height)
    , m_maxPushDistance(maxPushDistance)
    , m_pressedDistance(std::min(pressedDistance, maxPushDistance))
    , m_releasedDistance(std::min(releasedDistance, pressedDistance))
{}

void PressableButton::Subscribe(IButtonHandler* handler)
{
    m_handlers.emplace_back(handler);
}

void PressableButton::Unsubscribe(IButtonHandler* handler)
{
    for (auto& handlerPtr : m_handlers)
    {
        if (handlerPtr == handler)
        {
            // Set pointer to null instead of removing it from the list of handlers to avoid issues in case this is called from within a handler.
            handlerPtr = nullptr;
            break;
        }
    }
}

float PressableButton::CalculatePushDistance(const TouchPointer& pointer)
{
    TouchPointer* cachedPointer = nullptr;

	// Look for pointer in our list
    for (size_t i = 0; i < m_pointers.size(); ++i)
    {
        if (m_pointers[i].m_id == pointer.m_id)
        {
            cachedPointer = &m_pointers[i];
            break;
        }
    }

    if (!cachedPointer)
    {
		// If we didn't know about this pointer cache its position and ignore it as we don't know if it's 
		// coming from the front or the back.
        m_pointers.emplace_back(TouchPointer{ pointer.m_position, pointer.m_id });
        return 0;
    }

	XMVECTOR rayStartLocal;
	XMVECTOR rayEndLocal;

	// Calculate ray connecting previous and current pointer positions in local space
	{
		const auto invOrientation = XMQuaternionInverse(m_orientation);
		rayStartLocal = XMVector3Rotate(cachedPointer->m_position - m_restPosition, invOrientation);
		rayEndLocal = XMVector3Rotate(pointer.m_position - m_restPosition, invOrientation);
	}

	// Update cached position
    cachedPointer->m_position = pointer.m_position;

    const auto startDistance = -XMVectorGetZ(rayStartLocal);
    const auto endDistance = -XMVectorGetZ(rayEndLocal);
    const auto extents = 0.5f * XMVectorSet(m_width, m_height, m_maxPushDistance, 2.0f);

    float newPushDistance = 0;

    if (endDistance > m_currentPushDistance)
    {
        if (startDistance <= m_currentPushDistance)
        {
            // Check if the pointer went through the front face
            const auto frontFacePlane = XMVectorSet(0, 0, 1, m_currentPushDistance);
            auto intersectionPoint = XMPlaneIntersectLine(frontFacePlane, rayStartLocal, rayEndLocal);

            if (!XMVector3IsNaN(intersectionPoint) && XMVector2InBounds(intersectionPoint, extents))
            {
                newPushDistance = std::min(endDistance, m_maxPushDistance);
            }
        }
    }
    else if (endDistance > 0)
    {
        BoundingBox pushVolume;
        pushVolume.Center = XMFLOAT3(0, 0, -0.5f * m_maxPushDistance);
        XMStoreFloat3(&pushVolume.Extents, extents);

        if (pushVolume.Contains(rayEndLocal))
        {
            newPushDistance = endDistance;
        }
    }

    return newPushDistance;
}

XMVECTOR PressableButton::GetCurrentPosition() const
{
    const auto front = XMVector3Rotate(g_XMIdentityR2, m_orientation);
    return m_restPosition - front * XMVectorReplicate(m_currentPushDistance);
}

void PressableButton::SetRestTransform(FXMVECTOR position, FXMVECTOR orientation)
{
	m_restPosition = position;
	m_orientation = orientation;
}

void PressableButton::Update(float timeDelta, gsl::span<const TouchPointer> touchPointers)
{
    m_touchingPointerId = TouchPointer::InvalidPointerId;
    float targetDistance = 0;

    for (const auto& pointer : touchPointers)
    {
        float pushDistance = CalculatePushDistance(pointer);
        if (pushDistance > targetDistance)
        {
            m_touchingPointerId = pointer.m_id;
            targetDistance = pushDistance;
        }
    }

    assert(targetDistance >= 0 && targetDistance <= m_maxPushDistance);

    // Remove cached pointers we haven't seen this frame
    {
        auto newEnd = std::remove_if(m_pointers.begin(), m_pointers.end(), [touchPointers](const TouchPointer& pointer)
            {
                return std::find(touchPointers.begin(), touchPointers.end(), pointer) == touchPointers.end();
            });
        m_pointers.erase(newEnd, m_pointers.end());
    }

    const auto previousPushDistance = m_currentPushDistance;

    // Update push distance and raise events
    if (targetDistance > m_currentPushDistance)
    {
        m_currentPushDistance = targetDistance;

        if (!m_isPressed && m_currentPushDistance >= m_pressedDistance && previousPushDistance < m_pressedDistance)
        {
            m_isPressed = true;

            const auto numHandlers = m_handlers.size();
            for (size_t i = 0; i < numHandlers; ++i)
            {
                if (m_handlers[i])
                {
                    m_handlers[i]->OnButtonPressed(*this, m_touchingPointerId, XMVectorZero());
                }
            }
        }
    }
    else
    {
        m_currentPushDistance = std::max(targetDistance, m_currentPushDistance - timeDelta * m_recoverySpeed);

        if (m_isPressed && m_currentPushDistance <= m_releasedDistance && previousPushDistance > m_releasedDistance)
        {
            m_isPressed = false;

            const auto numHandlers = m_handlers.size();
            for (size_t i = 0; i < numHandlers; ++i)
            {
                if (m_handlers[i])
                {
                    m_handlers[i]->OnButtonReleased(*this, m_touchingPointerId);
                }
            }
        }
    }

    // Remove null handlers that may have been introduced via Unsubsribe()
    {
        auto newEnd = std::remove(m_handlers.begin(), m_handlers.end(), nullptr);
        m_handlers.erase(newEnd, m_handlers.end());
    }
}



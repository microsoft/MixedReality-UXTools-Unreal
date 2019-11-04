#pragma once

#include <DirectXMath.h>
#include <gsl/span>
#include <vector>

namespace Microsoft
{
    namespace MixedReality
    {
        namespace HandUtils
        {
            class PressableButton;

            using PointerId = uintptr_t;

            //struct ButtonEvent
            //{
            //    DirectX::XMVECTOR m_touchPoint;
            //    PointerId m_pointerId;
            //};

            /// Interface used to handle button events.
            struct IButtonHandler
            {
				/// Raised when the first pointer is seen.
				virtual void OnButtonHoverStart(PressableButton& button, PointerId pointerId) {};

				/// Raised when the last pointer is lost.
				virtual void OnButtonHoverEnd(PressableButton& button, PointerId pointerId) {};

                /// Raised when a pointer becomes in contact with a button.
                virtual void OnButtonTouchStart(PressableButton& button, PointerId pointerId, DirectX::FXMVECTOR touchPoint) {};

                /// Raised when a pointer stops being in contact with a button.
                virtual void OnButtonTouchEnd(PressableButton& button, PointerId pointerId) {};

                /// Raised when an unpressed button reaches the pressed distance.
                virtual void OnButtonPressed(PressableButton& button, PointerId pointerId, DirectX::FXMVECTOR touchPoint) {};

                /// Raised when a pressed button reaches the released distance.
                virtual void OnButtonReleased(PressableButton& button, PointerId pointerId) {};
            };

            /// Information about a pointer that can interact with a pressable button.
            struct TouchPointer
            {
                static constexpr PointerId InvalidPointerId = -1;

                bool operator==(const TouchPointer& other) const { return m_id == other.m_id; }

                /// Pointer position.
                DirectX::XMVECTOR m_position;

                /// Unique pointer id.
                PointerId m_id;
            };

            /// This class represents a flat, rectangular button that can be pushed via touch pointers.
            class PressableButton
            {
            public:

                /// Create a button with the given parameters.
                /// - rest position: position at rest.
                /// - orientation: the +Z axis is the front normal, the button will travel along its Z axis when pushed/released.
                /// - width: button width, along its X axis.
                /// - height: button height, along its Y axis.
                /// - maxPushDistance: maximum distance from the rest position the button will travel when pushed.
                /// - pressedDistance: distance from the rest position the button has to be to raise a pressed event.
                /// - releasedDistance: distance from the rest position the button has to be to raise a released event.
                PressableButton(
                    DirectX::FXMVECTOR restPosition, DirectX::FXMVECTOR orientation, float width, float height, 
                    float maxPushDistance, float pressedDistance, float releasedDistance );

                /// Subscribe handler to button events.
                void Subscribe(IButtonHandler* handler);

                /// Unsubscribe handler from button events.
                void Unsubscribe(IButtonHandler* handler);

				/// Returns the button position at rest, i.e when not pushed.
				DirectX::XMVECTOR GetRestPosition() const { return m_restPosition; }

                /// Returns the current button position, i.e. rest position + push distance along push direction.
                DirectX::XMVECTOR GetCurrentPosition() const;

                /// Returns the button orientation.
                DirectX::XMVECTOR GetOrientation() const { return m_orientation; }

				/// Sets the button transform at rest. 
				/// This modifies the current transform while keeping the current push distance.
				void SetRestTransform(DirectX::FXMVECTOR position, DirectX::FXMVECTOR orientation);

                /// Returns the button width.
                float GetWidth() const { return m_width; }

                /// Returns the button height.
                float GetHeight() const { return m_height; }
                
                /// Update the button state with the latest state of all relevant touch pointers. Time delta indicates the time since the last call to Update().
                void Update(float timeDelta, gsl::span<const TouchPointer> touchPointers);

            private:

                /// Returns the distance a given pointer is pushing the button to.
                float CalculatePushDistance(const TouchPointer& pointer);

            public:

                /// Button movement speed on release in meters per second.
                float m_recoverySpeed = 0.5f;

            private:

                DirectX::XMVECTOR m_restPosition;
                DirectX::XMVECTOR m_orientation;

                float m_width;
                float m_height;

                float m_maxPushDistance;
                float m_pressedDistance;
                float m_releasedDistance;
                float m_currentPushDistance = 0;

                /// Id of the pointer currently touching the button, if any.
                PointerId m_touchingPointerId = TouchPointer::InvalidPointerId;

                /// True if the button is currently pressed.
                bool m_isPressed = false;

                /// Pointers we're currently aware of.
                std::vector<TouchPointer> m_pointers;

                /// Subscribed event handlers.
                std::vector<IButtonHandler*> m_handlers;
            };
        }
    }
}
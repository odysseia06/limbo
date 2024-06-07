#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "core/event.h"

using namespace Limbo;
class MockEventListener : public ConcreteEventListener {
public:
	MOCK_METHOD1(onEvent, bool(Event&));
};
class EventDispatcherTest : public ::testing::Test {
protected:
	EventDispatcher dispatcher;
	MockEventListener mockListener;
	template<typename EventType>
	void testDispatchEvent(EventType& event) {
		dispatcher.registerListener(EventType::getStaticType(), &mockListener);
		EXPECT_CALL(mockListener, onEvent(::testing::Ref(event)))
			.Times(1)
			.WillOnce(::testing::Return(true));
		dispatcher.dispatch(event);
		EXPECT_TRUE(event.isHandled());
	}
};
TEST_F(EventDispatcherTest, DispatchWindowCloseEvent) {
	WindowCloseEvent event;
	testDispatchEvent(event);
}
TEST_F(EventDispatcherTest, DispatchWindowResizeEvent) {
	WindowResizeEvent event(1280, 720);
	testDispatchEvent(event);
}
TEST_F(EventDispatcherTest, DispatchKeyPressedEvent) {
	KeyPressedEvent event(32, 0);
	testDispatchEvent(event);
}
TEST_F(EventDispatcherTest, DispatchKeyReleasedEvent) {
	KeyReleasedEvent event(32);
	testDispatchEvent(event);
}
TEST_F(EventDispatcherTest, DispatchMouseMovedEvent) {
	MouseMovedEvent event(1280, 720);
	testDispatchEvent(event);
}
TEST_F(EventDispatcherTest, DispatchMouseScrolledEvent) {
	MouseScrolledEvent event(0, 0);
	testDispatchEvent(event);
}
TEST_F(EventDispatcherTest, DispatchMouseButtonPressedEvent) {
	MouseButtonPressedEvent event(0);
	testDispatchEvent(event);
}
TEST_F(EventDispatcherTest, DispatchMouseButtonReleasedEvent) {
	MouseButtonReleasedEvent event(0);
	testDispatchEvent(event);
}
TEST_F(EventDispatcherTest, DispatchKeyPressedEventWithNoListener) {
	KeyPressedEvent event(32, 0);
	dispatcher.dispatch(event);
	EXPECT_FALSE(event.isHandled());
}
TEST_F(EventDispatcherTest, DispatchWindowCloseEventWithNoListener) {
	WindowCloseEvent event;
	dispatcher.dispatch(event);
	EXPECT_FALSE(event.isHandled());
}
TEST_F(EventDispatcherTest, DispatchWindowResizeEventWithNoListener) {
	WindowResizeEvent event(1280, 720);
	dispatcher.dispatch(event);
	EXPECT_FALSE(event.isHandled());
}
TEST_F(EventDispatcherTest, DispatchMouseMovedEventWithNoListener) {
	MouseMovedEvent event(1280, 720);
	dispatcher.dispatch(event);
	EXPECT_FALSE(event.isHandled());
}
TEST_F(EventDispatcherTest, DispatchMouseScrolledEventWithNoListener) {
	MouseScrolledEvent event(0, 0);
	dispatcher.dispatch(event);
	EXPECT_FALSE(event.isHandled());
}
TEST_F(EventDispatcherTest, DispatchMouseButtonPressedEventWithNoListener) {
	MouseButtonPressedEvent event(0);
	dispatcher.dispatch(event);
	EXPECT_FALSE(event.isHandled());
}
TEST_F(EventDispatcherTest, DispatchMouseButtonReleasedEventWithNoListener) {
	MouseButtonReleasedEvent event(0);
	dispatcher.dispatch(event);
	EXPECT_FALSE(event.isHandled());
}
TEST_F(EventDispatcherTest, DispatchKeyPressedEventWithMultipleListeners) {
	KeyPressedEvent event(32, 0);
	MockEventListener mockListener2;
	dispatcher.registerListener(KeyPressedEvent::getStaticType(), &mockListener);
	dispatcher.registerListener(KeyPressedEvent::getStaticType(), &mockListener2);
	EXPECT_CALL(mockListener, onEvent(::testing::Ref(event)))
		.Times(1)
		.WillOnce(::testing::Return(true));
	EXPECT_CALL(mockListener2, onEvent(::testing::Ref(event)))
		.Times(1)
		.WillOnce(::testing::Return(true));
	dispatcher.dispatch(event, false);
	EXPECT_TRUE(event.isHandled());
}
TEST_F(EventDispatcherTest, DispatchWindowCloseEventWithMultipleListeners) {
	WindowCloseEvent event;
	MockEventListener mockListener2;
	dispatcher.registerListener(WindowCloseEvent::getStaticType(), &mockListener);
	dispatcher.registerListener(WindowCloseEvent::getStaticType(), &mockListener2);
	EXPECT_CALL(mockListener, onEvent(::testing::Ref(event)))
		.Times(1)
		.WillOnce(::testing::Return(true));
	EXPECT_CALL(mockListener2, onEvent(::testing::Ref(event)))
		.Times(1)
		.WillOnce(::testing::Return(true));
	dispatcher.dispatch(event, false);
	EXPECT_TRUE(event.isHandled());
}
TEST_F(EventDispatcherTest, DispatchWindowResizeEventWithMultipleListeners) {
	WindowResizeEvent event(1280, 720);
	MockEventListener mockListener2;
	dispatcher.registerListener(WindowResizeEvent::getStaticType(), &mockListener);
	dispatcher.registerListener(WindowResizeEvent::getStaticType(), &mockListener2);
	EXPECT_CALL(mockListener, onEvent(::testing::Ref(event)))
		.Times(1)
		.WillOnce(::testing::Return(true));
	EXPECT_CALL(mockListener2, onEvent(::testing::Ref(event)))
		.Times(1)
		.WillOnce(::testing::Return(true));
	dispatcher.dispatch(event, false);
	EXPECT_TRUE(event.isHandled());
}
TEST_F(EventDispatcherTest, DispatchMouseMovedEventWithMultipleListeners) {
	MouseMovedEvent event(1280, 720);
	MockEventListener mockListener2;
	dispatcher.registerListener(MouseMovedEvent::getStaticType(), &mockListener);
	dispatcher.registerListener(MouseMovedEvent::getStaticType(), &mockListener2);
	EXPECT_CALL(mockListener, onEvent(::testing::Ref(event)))
		.Times(1)
		.WillOnce(::testing::Return(true));
	EXPECT_CALL(mockListener2, onEvent(::testing::Ref(event)))
		.Times(1)
		.WillOnce(::testing::Return(true));
	dispatcher.dispatch(event, false);
	EXPECT_TRUE(event.isHandled());
}
TEST_F(EventDispatcherTest, DispatchMouseScrolledEventWithMultipleListeners) {
	MouseScrolledEvent event(0, 0);
	MockEventListener mockListener2;
	dispatcher.registerListener(MouseScrolledEvent::getStaticType(), &mockListener);
	dispatcher.registerListener(MouseScrolledEvent::getStaticType(), &mockListener2);
	EXPECT_CALL(mockListener, onEvent(::testing::Ref(event)))
		.Times(1)
		.WillOnce(::testing::Return(true));
	EXPECT_CALL(mockListener2, onEvent(::testing::Ref(event)))
		.Times(1)
		.WillOnce(::testing::Return(true));
	dispatcher.dispatch(event, false);
	EXPECT_TRUE(event.isHandled());
}
TEST_F(EventDispatcherTest, DispatchMouseButtonPressedEventWithMultipleListeners) {
	MouseButtonPressedEvent event(0);
	MockEventListener mockListener2;
	dispatcher.registerListener(MouseButtonPressedEvent::getStaticType(), &mockListener);
	dispatcher.registerListener(MouseButtonPressedEvent::getStaticType(), &mockListener2);
	EXPECT_CALL(mockListener, onEvent(::testing::Ref(event)))
		.Times(1)
		.WillOnce(::testing::Return(true));
	EXPECT_CALL(mockListener2, onEvent(::testing::Ref(event)))
		.Times(1)
		.WillOnce(::testing::Return(true));
	dispatcher.dispatch(event, false);
	EXPECT_TRUE(event.isHandled());
}
TEST_F(EventDispatcherTest, DispatchMouseButtonReleasedEventWithMultipleListeners) {
	MouseButtonReleasedEvent event(0);
	MockEventListener mockListener2;
	dispatcher.registerListener(MouseButtonReleasedEvent::getStaticType(), &mockListener);
	dispatcher.registerListener(MouseButtonReleasedEvent::getStaticType(), &mockListener2);
	EXPECT_CALL(mockListener, onEvent(::testing::Ref(event)))
		.Times(1)
		.WillOnce(::testing::Return(true));
	EXPECT_CALL(mockListener2, onEvent(::testing::Ref(event)))
		.Times(1)
		.WillOnce(::testing::Return(true));
	dispatcher.dispatch(event, false);
	EXPECT_TRUE(event.isHandled());
}
TEST_F(EventDispatcherTest, DispatchKeyPressedEventWithMultipleListenersNoneHandling) {
	KeyPressedEvent event(32, 0);
	MockEventListener mockListener2;
	dispatcher.registerListener(KeyPressedEvent::getStaticType(), &mockListener);
	dispatcher.registerListener(KeyPressedEvent::getStaticType(), &mockListener2);
	EXPECT_FALSE(event.isHandled());
}
TEST_F(EventDispatcherTest, DispatchWindowCloseEventWithMultipleListenersNoneHandling) {
	WindowCloseEvent event;
	MockEventListener mockListener2;
	dispatcher.registerListener(WindowCloseEvent::getStaticType(), &mockListener);
	dispatcher.registerListener(WindowCloseEvent::getStaticType(), &mockListener2);
	EXPECT_FALSE(event.isHandled());
}
TEST_F(EventDispatcherTest, DispatchWindowResizeEventWithMultipleListenersNoneHandling) {
	WindowResizeEvent event(1280, 720);
	MockEventListener mockListener2;
	dispatcher.registerListener(WindowResizeEvent::getStaticType(), &mockListener);
	dispatcher.registerListener(WindowResizeEvent::getStaticType(), &mockListener2);
	EXPECT_FALSE(event.isHandled());
}
TEST_F(EventDispatcherTest, DispatchMouseMovedEventWithMultipleListenersNoneHandling) {
	MouseMovedEvent event(1280, 720);
	MockEventListener mockListener2;
	dispatcher.registerListener(MouseMovedEvent::getStaticType(), &mockListener);
	dispatcher.registerListener(MouseMovedEvent::getStaticType(), &mockListener2);
	EXPECT_FALSE(event.isHandled());
}
TEST_F(EventDispatcherTest, DispatchMouseScrolledEventWithMultipleListenersNoneHandling) {
	MouseScrolledEvent event(0, 0);
	MockEventListener mockListener2;
	dispatcher.registerListener(MouseScrolledEvent::getStaticType(), &mockListener);
	dispatcher.registerListener(MouseScrolledEvent::getStaticType(), &mockListener2);
	EXPECT_FALSE(event.isHandled());
}
TEST_F(EventDispatcherTest, DispatchMouseButtonPressedEventWithMultipleListenersNoneHandling) {
	MouseButtonPressedEvent event(0);
	MockEventListener mockListener2;
	dispatcher.registerListener(MouseButtonPressedEvent::getStaticType(), &mockListener);
	dispatcher.registerListener(MouseButtonPressedEvent::getStaticType(), &mockListener2);
	EXPECT_FALSE(event.isHandled());
}
TEST_F(EventDispatcherTest, DispatchMouseButtonReleasedEventWithMultipleListenersNoneHandling) {
	MouseButtonReleasedEvent event(0);
	MockEventListener mockListener2;
	dispatcher.registerListener(MouseButtonReleasedEvent::getStaticType(), &mockListener);
	dispatcher.registerListener(MouseButtonReleasedEvent::getStaticType(), &mockListener2);
	EXPECT_FALSE(event.isHandled());
}

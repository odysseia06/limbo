#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "core/event.h"

using namespace Limbo;

class MockEventListener : public ConcreteEventListener {
public:
	MOCK_METHOD1(onEvent, bool(Event&));
};

TEST(EventDispatcherTest, DispatchMouseScrollEvent) {
	
	MockEventListener mockListener;
	EventDispatcher dispatcher;

	dispatcher.registerListener(EventType::MouseScrolled, &mockListener);

	MouseScrolledEvent event(10.0f, -5.0f);

	EXPECT_CALL(mockListener, onEvent(::testing::Ref(event)))
		.Times(1)
		.WillOnce(::testing::Return(true));
	dispatcher.dispatch(event);
	
	EXPECT_TRUE(event.isHandled());
}
#include <limbo/core/launch.hpp>
#include <limbo/core/log.hpp>
#include "game_host.hpp"

class HostApp : public limbo::Application
{
    GameHost host_;
    bool ok_ = false;

public:
    HostApp() { ok_ = host_.load("game"); }
    ~HostApp() override { host_.unload(); }

    void on_update(double dt) override
    {
        if (!ok_) return;
        host_.maybe_hot_reload();
        host_.tick(dt);
    }
    void on_event(const limbo::Event& e) override
    {
        if (e.type == limbo::EventType::KeyDown)
            limbo::log::info("Key {} pressed", e.key.key);
    }
};

static limbo::Application* CreateApplication()
{
    return new HostApp();
}

int main()
{
    limbo::log::init(limbo::log::Level::Info, limbo::log::Level::Trace);
    int rc = limbo::Launch({}, &CreateApplication);
    limbo::log::shutdown();
    return rc;
}

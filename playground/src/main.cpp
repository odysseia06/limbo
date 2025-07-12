#include <limbo/core/launch.hpp>
#include <limbo/core/log.hpp>

class SandboxApp : public limbo::Application {
    double acc_ = 0.0;
public:
    void on_update(double dt) override {
        acc_ += dt;
        if (acc_ >= 1.0) {
            limbo::log::info("FPS ≈ {}", 1.0 / dt);
            acc_ = 0.0;
        }
    }
};

static limbo::Application* CreateApplication() {
    return new SandboxApp();
}

int main()
{
    limbo::log::init(limbo::log::Level::Info, limbo::log::Level::Trace);
    int rc = limbo::Launch({}, &CreateApplication);
    limbo::log::shutdown();
    return rc;
}

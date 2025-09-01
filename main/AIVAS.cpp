#include "Application.hpp"
#include "Context.hpp"
#include "Display.hpp"
#include "SoftTimer.hpp"
#include "String.hpp"
#include "WiFi.hpp"

class Controller : public Component<Scope::singleton, Context, Display>
{
public:
    Controller(Context& context, Display& display)
        : context_{context},
          display_{display},
          timer_{context, "print", [this] { countUp(); }}
    {
        timer_.start(1000, true);
    }

private:
    void countUp()
    {
        display_.postText(str(context_.clientId(), ": ", ++count_));
    }

    Context& context_;
    Display& display_;
    SoftTimer timer_;
    int count_{};
};

auto app = make_application(
    context("Office-Aivas-Companion"),
    wiFi("VillaKunterbunt", "sacomoco02047781"),
    display(),
    component<Controller>()
);

extern "C" void app_main()
{
    app.run();
}

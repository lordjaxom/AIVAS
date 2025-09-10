#ifndef AIVAS_IOT_DISPLAY_HPP
#define AIVAS_IOT_DISPLAY_HPP

#include <misc/lv_types.h>

#include "Singleton.hpp"
#include "String.hpp"

class Display : public Singleton<Display>
{
public:
    Display();
    Display(Display const&) = delete;

    void brightness(int level);

    void showText(String const& text) const;

    void listen() const;
    void sleep() const;

private:
    lv_obj_t* img_sleep_{};
    lv_obj_t* img_listen_{};
};

#endif

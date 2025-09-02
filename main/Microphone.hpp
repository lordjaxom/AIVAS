#ifndef AIVAS_IOT_MICROPHONE_HPP
#define AIVAS_IOT_MICROPHONE_HPP

#include "Component.hpp"

class Microphone : public Component<Scope::singleton>
{
public:
    Microphone();
    Microphone(Microphone const&) = delete;
    ~Microphone();

    void read(void* buffer, std::size_t size) const;

private:
    void* handle_;
};

inline auto microphone()
{
    return component<Microphone>();
}

#endif
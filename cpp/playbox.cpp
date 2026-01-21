#include "playbox.hpp"
#include <stdexcept>
#include <cstring>

using namespace playbox;

App::App(const std::string& title, int targetFps){
    // Store title so desc_.title stays valid for the lifetime of App.
    title_storage_ = title;

    // desc_ is value-initialized in the header, but explicitly set fields here.
    desc_.title = title_storage_.c_str();
    desc_.target_fps = targetFps;

    desc_.on_init = &App::s_init;
    desc_.on_event = &App::s_event;
    desc_.on_update = &App::s_update;
    desc_.on_draw = &App::s_draw;
    desc_.on_shutdown = &App::s_shutdown;

    app_ = pb_app_create(&desc_, this);
    if(!app_) throw std::runtime_error("pb_app_create failed");
}

App::~App(){
    if(app_){
        pb_app_destroy(app_);
        app_ = nullptr;
    }
}

int App::run(){
    if(!app_) return 0;
    return pb_app_run(app_);
}

void App::s_init(pb_app* a, void* u){
    (void)a;
    App* self = (App*)u;
    if(self && self->onInit) self->onInit(*self);
}

void App::s_event(pb_app* a, void* u, const pb_event* ev){
    (void)a;
    App* self = (App*)u;
    if(self && ev && self->onEvent) self->onEvent(*self, *ev);
}

void App::s_update(pb_app* a, void* u, double dt){
    (void)a;
    App* self = (App*)u;
    if(self && self->onUpdate) self->onUpdate(*self, dt);
}

void App::s_draw(pb_app* a, void* u, pb_fb* fb){
    (void)a;
    App* self = (App*)u;
    if(self && self->onDraw && fb){
        Framebuffer f(fb);
        self->onDraw(*self, f);
    }
}

void App::s_shutdown(pb_app* a, void* u){
    (void)a;
    App* self = (App*)u;
    if(self && self->onShutdown) self->onShutdown(*self);
}

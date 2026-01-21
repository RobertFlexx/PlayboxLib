#ifndef PLAYBOXLIB_CPP_HPP
#define PLAYBOXLIB_CPP_HPP

#include <string>
#include <functional>
#include <cstdint>
#include "playbox/pb.h"

namespace playbox {

    struct Color {
        pb_color v;
        Color() : v{0,0,0} {}
        Color(uint8_t r,uint8_t g,uint8_t b){ v = pb_rgb(r,g,b); }
    };

    struct Cell {
        pb_cell v;
        Cell() { v = pb_cell_make(' ', pb_rgb(255,255,255), pb_rgb(0,0,0), 0); }
        Cell(uint32_t ch, Color fg, Color bg, uint16_t style=0){
            v = pb_cell_make(ch, fg.v, bg.v, style);
        }
    };

    struct Framebuffer {
        pb_fb* fb;
        explicit Framebuffer(pb_fb* raw=nullptr): fb(raw) {}

        int w() const { return fb ? fb->w : 0; }
        int h() const { return fb ? fb->h : 0; }

        void put(int x,int y,const Cell& c){ if(fb) pb_fb_put(fb,x,y,c.v); }

        void text(int x,int y,const std::string& s, Color fg, Color bg, uint16_t style=0){
            if(fb) pb_fb_text(fb,x,y,s.c_str(),fg.v,bg.v,style);
        }

        void box(int x,int y,int w,int h, Color fg, Color bg, uint16_t style=0){
            if(fb) pb_fb_box(fb,x,y,w,h,fg.v,bg.v,style);
        }

        void fill_rect(int x,int y,int w,int h,const Cell& c){
            if(fb) pb_fb_fill_rect(fb,x,y,w,h,c.v);
        }
    };

    class App {
    public:
        using OnInit = std::function<void(App&)>;
        using OnEvent = std::function<void(App&, const pb_event&)>;
        using OnUpdate = std::function<void(App&, double)>;
        using OnDraw = std::function<void(App&, Framebuffer&)>;
        using OnShutdown = std::function<void(App&)>;

        App(const std::string& title, int targetFps);
        ~App();

        App(const App&) = delete;
        App& operator=(const App&) = delete;

        App(App&&) = delete;
        App& operator=(App&&) = delete;

        int run();

        void quit(){ if(app_) pb_app_quit(app_); }
        int width()  const { return app_ ? pb_app_width(app_)  : 0; }
        int height() const { return app_ ? pb_app_height(app_) : 0; }

        void setTitle(const std::string& t){
            if(app_) pb_app_set_title(app_, t.c_str());
            title_storage_ = t; // keep storage in sync (nice for debugging / future)
        }

        OnInit onInit;
        OnEvent onEvent;
        OnUpdate onUpdate;
        OnDraw onDraw;
        OnShutdown onShutdown;

    private:
        pb_app* app_ = nullptr;
        pb_app_desc desc_{};            // zero-init
        std::string title_storage_;     // IMPORTANT: owns the title string

        static void s_init(pb_app* a, void* u);
        static void s_event(pb_app* a, void* u, const pb_event* ev);
        static void s_update(pb_app* a, void* u, double dt);
        static void s_draw(pb_app* a, void* u, pb_fb* fb);
        static void s_shutdown(pb_app* a, void* u);
    };

} // namespace playbox

#endif


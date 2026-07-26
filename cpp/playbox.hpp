#ifndef PLAYBOXLIB_CPP_HPP
#define PLAYBOXLIB_CPP_HPP

#include <string>
#include <functional>
#include <cstdint>
#include <vector>
#include "playbox/pb.h"

namespace playbox {

struct Color {
    pb_color v{};
    Color() = default;
    Color(uint8_t r, uint8_t g, uint8_t b){ v = pb_rgb(r,g,b); }
    explicit Color(pb_color c) : v(c) {}
    static Color lerp(Color a, Color b, float t){ return Color(pb_color_lerp(a.v, b.v, t)); }
    Color fade(float a) const { return Color(pb_color_fade(v, a)); }
};

struct Cell {
    pb_cell v{};
    Cell(){ v = pb_cell_make(' ', pb_rgb(255,255,255), pb_rgb(0,0,0), 0); }
    Cell(uint32_t ch, Color fg, Color bg, uint16_t style=0){
        v = pb_cell_make(ch, fg.v, bg.v, style);
    }
    explicit Cell(pb_cell c) : v(c) {}
};

struct Framebuffer {
    pb_fb* fb = nullptr;
    explicit Framebuffer(pb_fb* raw=nullptr): fb(raw) {}

    int w() const { return fb ? fb->w : 0; }
    int h() const { return fb ? fb->h : 0; }

    void put(int x,int y,const Cell& c){ if(fb) pb_fb_put(fb,x,y,c.v); }
    void putBlend(int x,int y,const Cell& c, float alpha, pb_blend_mode mode=PB_BLEND_ALPHA){
        if(fb) pb_fb_put_blend(fb,x,y,c.v,alpha,mode);
    }
    Cell get(int x,int y) const { return fb ? Cell(pb_fb_get(fb,x,y)) : Cell(); }

    void text(int x,int y,const std::string& s, Color fg, Color bg, uint16_t style=0){
        if(fb) pb_fb_text(fb,x,y,s.c_str(),fg.v,bg.v,style);
    }
    void textCentered(int y, const std::string& s, Color fg, Color bg, uint16_t style=0){
        if(fb) pb_fb_text_centered(fb, y, s.c_str(), fg.v, bg.v, style);
    }
    int textWrap(int x,int y,int maxW,int maxH, const std::string& s, Color fg, Color bg, uint16_t style=0){
        return fb ? pb_fb_text_wrap(fb,x,y,maxW,maxH,s.c_str(),fg.v,bg.v,style) : 0;
    }
    void textClipped(int x,int y,int maxW, const std::string& s, Color fg, Color bg, uint16_t style=0){
        if(fb) pb_fb_text_clipped(fb,x,y,maxW,s.c_str(),fg.v,bg.v,style);
    }
    static int measureText(const std::string& s){ return pb_fb_measure_text(s.c_str()); }

    void box(int x,int y,int w,int h, Color fg, Color bg, uint16_t style=0){
        if(fb) pb_fb_box(fb,x,y,w,h,fg.v,bg.v,style);
    }
    void boxEx(int x,int y,int w,int h, pb_box_style bs, Color fg, Color bg, uint16_t style=0){
        if(fb) pb_fb_box_ex(fb,x,y,w,h,bs,fg.v,bg.v,style);
    }
    void boxDouble(int x,int y,int w,int h, Color fg, Color bg, uint16_t style=0){
        if(fb) pb_fb_box_double(fb,x,y,w,h,fg.v,bg.v,style);
    }
    void panel(int x,int y,int w,int h, const std::string& title,
               Color border, Color titleFg, Color fill, uint16_t style=0){
        if(fb) pb_fb_panel(fb,x,y,w,h,title.c_str(),border.v,titleFg.v,fill.v,style);
    }
    void panelEx(int x,int y,int w,int h, const std::string& title, pb_box_style bs,
                 Color border, Color titleFg, Color fill, uint16_t style=0){
        if(fb) pb_fb_panel_ex(fb,x,y,w,h,title.c_str(),bs,border.v,titleFg.v,fill.v,style);
    }
    void shadow(int x,int y,int w,int h, Color sh, float alpha=0.45f){
        if(fb) pb_fb_shadow(fb,x,y,w,h,sh.v,alpha);
    }

    void fillRect(int x,int y,int w,int h,const Cell& c){ if(fb) pb_fb_fill_rect(fb,x,y,w,h,c.v); }
    void fillShade(int x,int y,int w,int h, Color fg, Color bg, int level){
        if(fb) pb_fb_fill_shade(fb,x,y,w,h,fg.v,bg.v,level);
    }
    void hline(int x,int y,int w,const Cell& c){ if(fb) pb_fb_hline(fb,x,y,w,c.v); }
    void vline(int x,int y,int h,const Cell& c){ if(fb) pb_fb_vline(fb,x,y,h,c.v); }
    void line(int x0,int y0,int x1,int y1,const Cell& c){ if(fb) pb_fb_line(fb,x0,y0,x1,y1,c.v); }
    void circle(int cx,int cy,int r,const Cell& c){ if(fb) pb_fb_circle(fb,cx,cy,r,c.v); }
    void fillCircle(int cx,int cy,int r,const Cell& c){ if(fb) pb_fb_fill_circle(fb,cx,cy,r,c.v); }
    void fillTriangle(int x0,int y0,int x1,int y1,int x2,int y2,const Cell& c){
        if(fb) pb_fb_fill_triangle(fb,x0,y0,x1,y1,x2,y2,c.v);
    }

    void blit(int dx,int dy,const pb_fb* src){ if(fb && src) pb_fb_blit(fb,dx,dy,src); }
    void blitMasked(int dx,int dy,const pb_fb* src, uint32_t transparent){
        if(fb && src) pb_fb_blit_masked(fb,dx,dy,src,transparent);
    }
    void blitBlend(int dx,int dy,const pb_fb* src, float alpha, pb_blend_mode mode=PB_BLEND_ALPHA){
        if(fb && src) pb_fb_blit_blend(fb,dx,dy,src,alpha,mode);
    }

    void plot(int px,int py, Color c){ if(fb) pb_fb_plot(fb,px,py,c.v); }
    void plotBlend(int px,int py, Color c, float a){ if(fb) pb_fb_plot_blend(fb,px,py,c.v,a); }
    void plotLine(int x0,int y0,int x1,int y1, Color c){ if(fb) pb_fb_plot_line(fb,x0,y0,x1,y1,c.v); }
    void plotFillCircle(int cx,int cy,int r, Color c){ if(fb) pb_fb_plot_fill_circle(fb,cx,cy,r,c.v); }

    void braillePlot(int px,int py, Color c){ if(fb) pb_fb_braille_plot(fb,px,py,c.v); }
    void brailleFillCircle(int cx,int cy,int r, Color c){ if(fb) pb_fb_braille_fill_circle(fb,cx,cy,r,c.v); }
    void brailleLine(int x0,int y0,int x1,int y1, Color c){ if(fb) pb_fb_braille_line(fb,x0,y0,x1,y1,c.v); }
    void brailleFillTriangle(int x0,int y0,int x1,int y1,int x2,int y2, Color c){
        if(fb) pb_fb_braille_fill_triangle(fb,x0,y0,x1,y1,x2,y2,c.v);
    }
    void brailleClear(Color bg){ if(fb) pb_fb_braille_clear(fb, bg.v); }

    void quadPlot(int px,int py, Color c){ if(fb) pb_fb_quad_plot(fb,px,py,c.v); }
    void quadFillCircle(int cx,int cy,int r, Color c){ if(fb) pb_fb_quad_fill_circle(fb,cx,cy,r,c.v); }

    void setCamera(int x,int y){ if(fb) pb_fb_set_camera(fb,x,y); }
    void getCamera(int& x, int& y) const { if(fb) pb_fb_get_camera(fb,&x,&y); }
    void setClip(int x,int y,int w,int h){ if(fb) pb_fb_set_clip(fb,x,y,w,h); }
    void resetClip(){ if(fb) pb_fb_reset_clip(fb); }

    void fillGradientV(int x,int y,int w,int h, Color top, Color bottom){
        if(fb) pb_fb_fill_gradient_v(fb,x,y,w,h,top.v,bottom.v);
    }
    void fillGradientH(int x,int y,int w,int h, Color left, Color right){
        if(fb) pb_fb_fill_gradient_h(fb,x,y,w,h,left.v,right.v);
    }
    void fillDither(int x,int y,int w,int h, Color a, Color b, int pattern){
        if(fb) pb_fb_fill_dither(fb,x,y,w,h,a.v,b.v,pattern);
    }
};

struct Sheet {
    pb_sheet s{};
    Sheet() = default;
    Sheet(int cols, int rows, int tw, int th){ s = pb_sheet_create(cols, rows, tw, th); }
    explicit Sheet(pb_fb atlas, int tw, int th){ s = pb_sheet_wrap(atlas, tw, th); }
    ~Sheet(){ pb_sheet_free(&s); }
    Sheet(const Sheet&) = delete;
    Sheet& operator=(const Sheet&) = delete;
    void setTile(int id, const pb_fb* src){ pb_sheet_set_tile(&s, id, src); }
    void blit(Framebuffer& dst, int dx, int dy, int id){ pb_fb_blit_tile(dst.fb, dx, dy, &s, id); }
    void blitMasked(Framebuffer& dst, int dx, int dy, int id, uint32_t t){
        pb_fb_blit_tile_masked(dst.fb, dx, dy, &s, id, t);
    }
};

struct Particles {
    pb_particles ps{};
    explicit Particles(int cap=256){ pb_particles_init(&ps, cap); }
    ~Particles(){ pb_particles_free(&ps); }
    Particles(const Particles&) = delete;
    Particles& operator=(const Particles&) = delete;
    void emit(float x,float y,float vx,float vy,float life, Color c){
        pb_particles_emit(&ps,x,y,vx,vy,life,c.v);
    }
    void update(double dt){ pb_particles_update(&ps, dt); }
    void drawBraille(Framebuffer& fb){ pb_particles_draw_braille(fb.fb, &ps); }
    void drawHalf(Framebuffer& fb){ pb_particles_draw_half(fb.fb, &ps); }
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

    int run();
    void quit(){ if(app_) pb_app_quit(app_); }
    int width()  const { return app_ ? pb_app_width(app_)  : 0; }
    int height() const { return app_ ? pb_app_height(app_) : 0; }
    int fps() const { return app_ ? pb_get_fps(app_) : 0; }
    double frameTime() const { return app_ ? pb_get_frame_time(app_) : 0.0; }

    void setTitle(const std::string& t){
        title_storage_ = t;
        if(app_) pb_app_set_title(app_, title_storage_.c_str());
    }
    void setClear(const Cell& c){ if(app_) pb_app_set_clear(app_, c.v); }
    void setTargetFps(int fps){ if(app_) pb_app_set_target_fps(app_, fps); }
    void setVsync(bool on){ if(app_) pb_app_set_vsync(app_, on ? 1 : 0); }
    bool vsync() const { return app_ && pb_app_get_vsync(app_); }
    void setRefreshHz(int hz){ if(app_) pb_app_set_refresh_hz(app_, hz); }
    int refreshHz() const { return app_ ? pb_app_get_refresh_hz(app_) : 0; }
    pb_frame_stats frameStats() const {
        pb_frame_stats st{};
        if(app_) pb_get_frame_stats(app_, &st);
        return st;
    }
    void requestResize(){ if(app_) pb_app_request_resize(app_); }

    bool isKeyDown(pb_key k) const { return app_ && pb_is_key_down(app_, k); }
    bool isKeyPressed(pb_key k) const { return app_ && pb_is_key_pressed(app_, k); }
    bool isKeyReleased(pb_key k) const { return app_ && pb_is_key_released(app_, k); }
    bool isCharDown(uint32_t cp) const { return app_ && pb_is_char_down(app_, cp); }
    bool isCharPressed(uint32_t cp) const { return app_ && pb_is_char_pressed(app_, cp); }
    int mouseX() const { return app_ ? pb_get_mouse_x(app_) : 0; }
    int mouseY() const { return app_ ? pb_get_mouse_y(app_) : 0; }
    bool isMouseDown(int button) const { return app_ && pb_is_mouse_button_down(app_, button); }
    bool isMousePressed(int button) const { return app_ && pb_is_mouse_button_pressed(app_, button); }
    bool isMouseReleased(int button) const { return app_ && pb_is_mouse_button_released(app_, button); }
    int mouseWheel() const { return app_ ? pb_get_mouse_wheel(app_) : 0; }
    bool focused() const { return app_ && pb_is_focused(app_); }
    bool isReplay() const { return app_ && pb_app_is_replay(app_); }
    uint32_t replaySeed() const { return app_ ? pb_app_replay_seed(app_) : 0; }

    pb_app* raw() { return app_; }
    static const char* version(){ return pb_version_string(); }

    OnInit onInit;
    OnEvent onEvent;
    OnUpdate onUpdate;
    OnDraw onDraw;
    OnShutdown onShutdown;

private:
    pb_app* app_ = nullptr;
    pb_app_desc desc_{};
    std::string title_storage_;
    static void s_init(pb_app* a, void* u);
    static void s_event(pb_app* a, void* u, const pb_event* ev);
    static void s_update(pb_app* a, void* u, double dt);
    static void s_draw(pb_app* a, void* u, pb_fb* fb);
    static void s_shutdown(pb_app* a, void* u);
};

} // namespace playbox

#endif

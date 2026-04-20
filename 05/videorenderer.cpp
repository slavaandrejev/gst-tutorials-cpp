#include <glbinding/glbinding.h>
#include <glbinding/getProcAddress.h>
#include <glbinding/gl/gl.h>

#include "gnamespaces.h"
#include "videorenderer.h"

using namespace gl;

bool VideoRenderer::render_(Gdk::GLContext context) noexcept {
    glClearColor(51 / 255.0f, 51 / 255.0f, 55 / 255.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    auto texture_id = get_texture.emit();
    if (0 <= texture_id) {
        auto video_width  = GLint{};
        auto video_height = GLint{};
        glGetTextureLevelParameteriv(texture_id, 0, GL_TEXTURE_WIDTH, &video_width);
        glGetTextureLevelParameteriv(texture_id, 0, GL_TEXTURE_HEIGHT, &video_height);

        auto video_aspect = float(video_width) / float(video_height);
        auto view_aspect  = float(get_width()) / float(get_height());

        auto sx = 1.0f;
        auto sy = 1.0f;
        if (video_aspect > view_aspect) {
            sy = view_aspect / video_aspect;
        } else {
            sx = video_aspect / view_aspect;
        }
        video_shader->set("scale", glm::vec2{sx, sy});

        video_shader->use();
        glBindTextureUnit(0, texture_id);

        glBindVertexArray(empty_vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    return true;
}

void VideoRenderer::realize_() noexcept {
    static bool bound = false;
    Gtk::impl::GLAreaImpl::realize_();
    make_current(); // for Windows only

    if (!bound) {
        glbinding::initialize(glbinding::getProcAddress, true);
        bound = true;
    }

    video_shader = std::make_unique<Shader>(
        "/video-vs.glsl", GL_VERTEX_SHADER,
        "/video-fs.glsl", GL_FRAGMENT_SHADER);

    glCreateVertexArrays(1, &empty_vao);

    if (0 == tick_callback_id) {
        tick_callback_id = add_tick_callback(gi::mem_fun(&VideoRenderer::timer_event, this));
    }
}

void VideoRenderer::unrealize_() noexcept {
    if (0 != tick_callback_id) {
        remove_tick_callback(tick_callback_id);
        tick_callback_id = 0;
    }

    Gtk::impl::GLAreaImpl::unrealize_();
}

bool VideoRenderer::timer_event(Gtk::Widget, Gdk::FrameClock frame_clock) {
    queue_draw();
    return true;
}

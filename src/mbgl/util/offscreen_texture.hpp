#pragma once

#include <mbgl/map/view.hpp>
#include <mbgl/gl/framebuffer.hpp>
#include <mbgl/gl/renderbuffer.hpp>
#include <mbgl/gl/texture.hpp>
#include <mbgl/util/optional.hpp>
#include <mbgl/util/image.hpp>

namespace mbgl {

namespace gl {
class Context;
} // namespace gl

class OffscreenTexture : public View {
public:
    OffscreenTexture(gl::Context&, Size size = { 256, 256 });

    void bind() override;
    void bindRenderbuffers();

    PremultipliedImage readStillImage();

    gl::Texture& getTexture();

public:
    const Size size;

private:
    gl::Context& context;
    optional<gl::Framebuffer> framebuffer;
    optional<gl::Texture> texture;
    optional<gl::Renderbuffer<gl::RenderbufferType::RGBA>> colorTarget;
    optional<gl::Renderbuffer<gl::RenderbufferType::DepthComponent>> depthTarget;
};

} // namespace mbgl

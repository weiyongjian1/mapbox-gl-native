#include <mbgl/util/offscreen_texture.hpp>
#include <mbgl/gl/context.hpp>

#include <cstring>
#include <cassert>

namespace mbgl {

OffscreenTexture::OffscreenTexture(gl::Context& context_, const Size size_)
    : size(std::move(size_)), context(context_) {
    assert(size);
}

void OffscreenTexture::bind() {
    if (!framebuffer) {
        texture = context.createTexture(size);
        framebuffer = context.createFramebuffer(*texture);
    } else {
        context.bindFramebuffer = framebuffer->framebuffer;
    }

    context.viewport = { 0, 0, size };
}

void OffscreenTexture::bindRenderbuffers() {
    if (!framebuffer) {
        texture = context.createTexture(size);
        colorTarget = context.createRenderbuffer<gl::RenderbufferType::RGBA>(size);
        depthTarget = context.createRenderbuffer<gl::RenderbufferType::DepthComponent>(size);
        framebuffer = context.createFramebuffer(*colorTarget, *depthTarget, *texture);
    } else {
        context.bindFramebuffer = framebuffer->framebuffer;
    }

    context.viewport = { 0, 0, size };
}

gl::Texture& OffscreenTexture::getTexture() {
    assert(texture);
    return *texture;
}

PremultipliedImage OffscreenTexture::readStillImage() {
    return context.readFramebuffer<PremultipliedImage>(size);
}


} // namespace mbgl

#include <mbgl/benchmark/util.hpp>
#include <mbgl/gl/offscreen_view.hpp>
#include <mbgl/gl/headless_display.hpp>


#include <mbgl/map/map.hpp>
#include <mbgl/map/view.hpp>
#include <mbgl/util/image.hpp>
#include <mbgl/util/run_loop.hpp>

namespace mbgl {
namespace benchmark {

void render(Map& map, OffscreenView& view) {
    PremultipliedImage result;
    map.renderStill(view, [&](std::exception_ptr) {
        result = view.readStillImage();
    });

    while (!result.valid()) {
        util::RunLoop::Get()->runOnce();
    }
}

std::shared_ptr<HeadlessDisplay> sharedDisplay() {
    static auto display = std::make_shared<HeadlessDisplay>();
    return display;
}

} // namespace benchmark
} // namespace mbgl

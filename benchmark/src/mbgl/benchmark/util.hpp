#pragma once

#include <memory>

namespace mbgl {

class Map;
class OffscreenView;
class HeadlessDisplay;

namespace benchmark {

void render(Map&, OffscreenView&);

std::shared_ptr<HeadlessDisplay> sharedDisplay();


} // namespace benchmark
} // namespace mbgl

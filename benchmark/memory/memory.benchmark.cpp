#include <mbgl/benchmark/stub_file_source.hpp>
#include <mbgl/benchmark/util.hpp>

#include <mbgl/map/map.hpp>
#include <mbgl/gl/offscreen_view.hpp>
#include <mbgl/gl/headless_backend.hpp>
#include <mbgl/util/default_thread_pool.hpp>
#include <mbgl/util/io.hpp>
#include <mbgl/util/run_loop.hpp>

#include <algorithm>
#include <iostream>
#include <iterator>
#include <string>
#include <unordered_map>
#include <utility>
#include <sstream>

#include <stdlib.h>
#include <unistd.h>

/*
 * Begin getRSS.c
 *
 * From http://nadeausoftware.com/articles/2012/07/c_c_tip_how_get_process_resident_set_size_physical_memory_use
 *
 * Author:  David Robert Nadeau
 * Site:    http://NadeauSoftware.com/
 * License: Creative Commons Attribution 3.0 Unported License
 *          http://creativecommons.org/licenses/by/3.0/deed.en_US
 */

#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>

#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#include <sys/resource.h>

#if defined(__APPLE__) && defined(__MACH__)
#include <mach/mach.h>
#include <mach/message.h>  // for mach_port_t
#include <mach/task_info.h>

#elif (defined(_AIX) || defined(__TOS__AIX__)) || (defined(__sun__) || defined(__sun) || defined(sun) && (defined(__SVR4) || defined(__svr4__)))
#include <fcntl.h>
#include <procfs.h>

#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
#include <stdio.h>

#endif

#else
#error "Cannot define getPeakRSS( ) or getCurrentRSS( ) for an unknown OS."
#endif

/**
 * Returns the peak (maximum so far) resident set size (physical
 * memory use) measured in bytes, or zero if the value cannot be
 * determined on this OS.
 */
size_t getPeakRSS( )
{
#if defined(_WIN32)
	/* Windows -------------------------------------------------- */
	PROCESS_MEMORY_COUNTERS info;
	GetProcessMemoryInfo( GetCurrentProcess( ), &info, sizeof(info) );
	return (size_t)info.PeakWorkingSetSize;

#elif (defined(_AIX) || defined(__TOS__AIX__)) || (defined(__sun__) || defined(__sun) || defined(sun) && (defined(__SVR4) || defined(__svr4__)))
	/* AIX and Solaris ------------------------------------------ */
	struct psinfo psinfo;
	int fd = -1;
	if ( (fd = open( "/proc/self/psinfo", O_RDONLY )) == -1 )
		return (size_t)0L;		/* Can't open? */
	if ( read( fd, &psinfo, sizeof(psinfo) ) != sizeof(psinfo) )
	{
		close( fd );
		return (size_t)0L;		/* Can't read? */
	}
	close( fd );
	return (size_t)(psinfo.pr_rssize * 1024L);

#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
	/* BSD, Linux, and OSX -------------------------------------- */
	struct rusage rusage;
	getrusage( RUSAGE_SELF, &rusage );
#if defined(__APPLE__) && defined(__MACH__)
	return (size_t)rusage.ru_maxrss;
#else
	return (size_t)(rusage.ru_maxrss * 1024L);
#endif

#else
	/* Unknown OS ----------------------------------------------- */
	return (size_t)0L;			/* Unsupported. */
#endif
}

/**
 * Returns the current resident set size (physical memory use) measured
 * in bytes, or zero if the value cannot be determined on this OS.
 */
size_t getCurrentRSS( )
{
#if defined(_WIN32)
	/* Windows -------------------------------------------------- */
	PROCESS_MEMORY_COUNTERS info;
	GetProcessMemoryInfo( GetCurrentProcess( ), &info, sizeof(info) );
	return (size_t)info.WorkingSetSize;

#elif defined(__APPLE__) && defined(__MACH__)
	/* OSX ------------------------------------------------------ */
	struct mach_task_basic_info info;
	mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
	if ( task_info( mach_task_self( ), MACH_TASK_BASIC_INFO,
		(task_info_t)&info, &infoCount ) != KERN_SUCCESS )
		return (size_t)0L;		/* Can't access? */
	return (size_t)info.resident_size;

#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
	/* Linux ---------------------------------------------------- */
	long rss = 0L;
	FILE* fp = NULL;
	if ( (fp = fopen( "/proc/self/statm", "r" )) == NULL )
		return (size_t)0L;		/* Can't open? */
	if ( fscanf( fp, "%*s%ld", &rss ) != 1 )
	{
		fclose( fp );
		return (size_t)0L;		/* Can't read? */
	}
	fclose( fp );
	return (size_t)rss * (size_t)sysconf( _SC_PAGESIZE);

#else
	/* AIX, BSD, Solaris, and Unknown OS ------------------------ */
	return (size_t)0L;			/* Unsupported. */
#endif
}

/** End getRSS.c */


using namespace mbgl;

namespace mbgl {
namespace benchmark {
class MemoryBenchmark {
public:
    MemoryBenchmark() {
        fileSource.styleResponse = [&](const Resource& res) { return response("style_" + getType(res) + ".json");};
        fileSource.tileResponse = [&](const Resource& res) { return response(getType(res) + ".tile"); };
        fileSource.sourceResponse = [&](const Resource& res) { return response("source_" + getType(res) + ".json"); };
        fileSource.glyphsResponse = [&](const Resource&) { return response("glyphs.pbf"); };
        fileSource.spriteJSONResponse = [&](const Resource&) { return response("sprite.json"); };
        fileSource.spriteImageResponse = [&](const Resource&) { return response("sprite.png"); };
    }

    util::RunLoop runLoop;
    HeadlessBackend backend { mbgl::benchmark::sharedDisplay() };
    OffscreenView view{ backend.getContext(), { 512, 512 } };
    StubFileSource fileSource;
    ThreadPool threadPool { 4 };

private:
    Response response(const std::string& path) {
        Response result;

        auto it = cache.find(path);
        if (it != cache.end()) {
            result.data = it->second;
        } else {
            auto data = std::make_shared<std::string>(
                util::read_file(std::string("test/fixtures/resources/") + path));

            cache.insert(it, std::make_pair(path, data));
            result.data = data;
        }

        return result;
    }

    std::string getType(const Resource& res) {
        if (res.url.find("satellite") != std::string::npos) {
            return "raster";
        } else {
            return "vector";
        }
    };

    std::unordered_map<std::string, std::shared_ptr<std::string>> cache;
};

bool assertWithinThreshold(std::string label, double actual, int expected) {
    bool pass = actual <= expected;
    std::cout << label << ": " << actual;
    if (pass) {
        std::cout << " OK" << std::endl;
    } else {
        std::cout << " FAILED (expected < " << expected << ")" << std::endl;
    }
    return pass;
}

bool memoryFootprintBenchmark() {
    MemoryBenchmark bench;

    auto renderMap = [&](Map& map, const char* style){
        map.setZoom(16);
        map.setStyleURL(style);
        render(map, bench.view);
    };
    
    // Warm up buffers and cache.
    for (unsigned i = 0; i < 10; ++i) {
        Map map(bench.backend, { 256, 256 }, 2, bench.fileSource, bench.threadPool, MapMode::Still);
        renderMap(map, "mapbox://streets");
        renderMap(map, "mapbox://satellite");
    };
    
    // Process close callbacks, mostly needed by
    // libuv runloop.
    bench.runLoop.runOnce();
    
    unsigned runs = 15;
    std::vector<std::unique_ptr<Map>> maps;
    
    long vectorInitialRSS = getCurrentRSS();
    for (unsigned i = 0; i < runs; ++i) {
        auto map = std::make_unique<Map>(bench.backend, Size{ 256, 256 }, 2, bench.fileSource,
                                         bench.threadPool, MapMode::Still);
        renderMap(*map, "mapbox://streets");
        maps.push_back(std::move(map));
    }
    
    double vectorFootprint = (getCurrentRSS() - vectorInitialRSS) / double(runs);
    
    long rasterInitialRSS = getCurrentRSS();
    for (unsigned i = 0; i < runs; ++i) {
        auto raster = std::make_unique<Map>(bench.backend, Size{ 256, 256 }, 2, bench.fileSource,
                                            bench.threadPool, MapMode::Still);
        renderMap(*raster, "mapbox://satellite");
        maps.push_back(std::move(raster));
    };
    
    double rasterFootprint = (getCurrentRSS() - rasterInitialRSS) / double(runs);
    
    return assertWithinThreshold("Vector", vectorFootprint, 65 * 1024 * 1024) &&
        assertWithinThreshold("Raster", rasterFootprint, 25 * 1024 * 1024);
    
    std::cout <<
        "Vector footprint: " << vectorFootprint << std::endl <<
        "Raster footprint: " << rasterFootprint << std::endl;
}

} // namespace benchmark
} // namespace mbgl

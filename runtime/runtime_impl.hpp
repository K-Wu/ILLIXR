#include <thread>
#include <cassert>
#include <cerrno>
#include <chrono>
#include "common/runtime.hpp"
#include "common/extended_window.hpp"
#include "common/dynamic_lib.hpp"
#include "common/plugin.hpp"
#include "switchboard_impl.hpp"
#include "stdout_record_logger.hpp"
#include "noop_record_logger.hpp"
#include "sqlite_record_logger.hpp"
#include "common/global_module_defs.hpp"
#include "common/error_util.hpp"

using namespace ILLIXR;

class runtime_impl : public runtime {
public:
	runtime_impl(GLXContext appGLCtx) {
		pb.register_impl<record_logger>(std::make_shared<sqlite_record_logger>());
		pb.register_impl<gen_guid>(std::make_shared<gen_guid>());
		pb.register_impl<switchboard>(create_switchboard(&pb));
		pb.register_impl<xlib_gl_extended_window>(std::make_shared<xlib_gl_extended_window>(ILLIXR::FB_WIDTH, ILLIXR::FB_HEIGHT, appGLCtx));
	}

	virtual void load_so(const std::vector<std::string>& so_paths) override {
        assert(errno == 0 && "Errno should not be set before creating any dynamic library");

		std::transform(so_paths.cbegin(), so_paths.cend(), std::back_inserter(libs), [](const auto& so_path) {
		    RAC_ERRNO_MSG("runtime_impl before creating the dynamic library");
			return dynamic_lib::create(so_path);
		});

        RAC_ERRNO_MSG("runtime_impl after creating the dynamic libraries");

		std::vector<plugin_factory> plugin_factories;
		std::transform(libs.cbegin(), libs.cend(), std::back_inserter(plugin_factories), [](const auto& lib) {
			return lib.template get<plugin* (*) (phonebook*)>("this_plugin_factory");
		});

        RAC_ERRNO_MSG("runtime_impl after generating plugin factories");

		std::transform(plugin_factories.cbegin(), plugin_factories.cend(), std::back_inserter(plugins), [this](const auto& plugin_factory) {
		    RAC_ERRNO_MSG("runtime_impl before building the plugin");
			return std::unique_ptr<plugin>{plugin_factory(&pb)};
		});

		std::for_each(plugins.cbegin(), plugins.cend(), [](const auto& plugin) {
			plugin->start();
		});
	}

	virtual void load_so(const std::string_view so) override {
		auto lib = dynamic_lib::create(so);
		plugin_factory this_plugin_factory = lib.get<plugin* (*) (phonebook*)>("this_plugin_factory");
		load_plugin_factory(this_plugin_factory);
		libs.push_back(std::move(lib));
	}

	virtual void load_plugin_factory(plugin_factory plugin_main) override {
		plugins.emplace_back(plugin_main(&pb));
		plugins.back()->start();
	}

	virtual void wait() override {
		while (!terminate.load()) {
			std::this_thread::sleep_for(std::chrono::milliseconds{10});
		}
	}

	virtual void stop() override {
		pb.lookup_impl<switchboard>()->stop();
		for (const std::unique_ptr<plugin>& plugin : plugins) {
			plugin->stop();
		}
		terminate.store(true);
	}

	virtual ~runtime_impl() override {
		if (!terminate.load()) {
            ILLIXR::abort("You didn't call stop() before destructing this plugin.");
		}
		// This will be re-enabled in #225
		// assert(errno == 0 && "errno was set during run. Maybe spurious?");
		/*
		  Note that this assertion can have false positives AND false negatives!
		  - False positive because the contract of some functions specifies that errno is only meaningful if the return code was an error [1].
		    - We will try to mitigate this by clearing errno on success in ILLIXR.
		  - False negative if some intervening call clears errno.
		    - We will try to mitigate this by checking for errors immediately after a call.

		  Despite these mitigations, the best way to catch errors is to check errno immediately after a calling function.

		  [1] https://cboard.cprogramming.com/linux-programming/119957-xlib-perversity.html
		 */
	}

private:
	// I have to keep the dynamic libs in scope until the program is dead
	std::vector<dynamic_lib> libs;
	phonebook pb;
	std::vector<std::unique_ptr<plugin>> plugins;
	std::atomic<bool> terminate {false};
};

extern "C" runtime* runtime_factory(GLXContext appGLCtx) {
    assert(errno == 0 && "Errno should not be set before creating the runtime");
	return new runtime_impl{appGLCtx};
}

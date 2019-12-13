/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/

 /// \file
 /// \brief sets up a basic mdl environment

#ifndef MDL_SDK_EXAMPLES_MDL_SDK_HELPER_H 
#define MDL_SDK_EXAMPLES_MDL_SDK_HELPER_H

// MDL SDK (and utils)
#include <mi/mdl_sdk.h>
#include "mdl_browser_command_line_options.h"


/// Custom logger 
class Mdl_browser_logger : public mi::base::Interface_implement<mi::base::ILogger>
{
public:
    Mdl_browser_logger(bool trace) : m_trace(trace) {}

    void message(mi::base::Message_severity level, const char* mc, const char* message) override
    {
        if ((m_trace) || (level == mi::base::MESSAGE_SEVERITY_ERROR))
            fprintf(stderr, "%s\n", message);
    }

private:
    bool m_trace;
};

mi::neuraylib::INeuray* load_mdl_sdk(const Mdl_browser_command_line_options& options)
{
    // Access the MDL SDK
    auto neuray = mi::base::Handle<mi::neuraylib::INeuray>(load_and_get_ineuray());
    if (!neuray.is_valid_interface()) 
        return nullptr;

    // Add MDL search paths
    auto compiler = mi::base::Handle<mi::neuraylib::IMdl_compiler>(
        neuray->get_api_component<mi::neuraylib::IMdl_compiler>());

    auto logger = mi::base::make_handle(new Mdl_browser_logger(false));
    compiler->set_logger(logger.get());

    // clear all search paths and add specified default ones
    compiler->clear_module_paths(); // contains the current working directory

    // use default paths if none are specified explicitly
    if (options.search_paths.empty())
    {
        // add admin space search paths before user space paths
        auto admin_space_paths = get_mdl_admin_space_search_paths();
        for (const auto& path : admin_space_paths)
            compiler->add_module_path(path.c_str());

        auto user_space_paths = get_mdl_user_space_search_paths();
        for (const auto& path : user_space_paths)
            compiler->add_module_path(path.c_str());
    }
    else
    {
        // add the paths specified on the command line 
        for (const auto& path : options.search_paths)
            compiler->add_module_path(path.c_str());
    }

    // print paths to check
    for (mi::Size i = 0, n = compiler->get_module_paths_length(); i < n; ++i)
        std::cout << "MDL Module Path: " << compiler->get_module_path(i)->get_c_str() << "\n";

    // Configure the MDL SDK
    // Load plugin required for loading textures
    if (0 != compiler->load_plugin_library("nv_freeimage" MI_BASE_DLL_FILE_EXT))
    {
        std::cerr << "[Mdl_sdk] start: failed to load 'nv_freeimage' library.\n";
        return nullptr;
    }

    // setup the locale if specified by the user
    if (!options.locale.empty())
    {
        mi::base::Handle<mi::neuraylib::IMdl_i18n_configuration> i18n_configuration(
            neuray->get_api_component<mi::neuraylib::IMdl_i18n_configuration>());

        if (!i18n_configuration.is_valid_interface())
        {
            std::cerr << "[Mdl_sdk] start: failed to setup locale.\n";
            return nullptr;
        }

        // set the local
        i18n_configuration->set_locale(options.locale.c_str());
    }

    // Start the MDL SDK
    check_start_success(neuray->start()); // NOTE: this kills the app in case of failure

    neuray->retain();
    return neuray.get();
}

mi::neuraylib::ITransaction* create_transaction(mi::neuraylib::INeuray* neuray)
{
    auto database = mi::base::Handle<mi::neuraylib::IDatabase>(
        neuray->get_api_component<mi::neuraylib::IDatabase>());

    auto scope = mi::base::Handle<mi::neuraylib::IScope>(database->get_global_scope());

    mi::base::Handle<mi::neuraylib::ITransaction> transaction(scope->create_transaction());
    transaction->retain();
    return transaction.get();
}


#endif

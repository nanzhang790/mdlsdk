/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/

/// \file
/// \brief Helper class for handling platform specific operations.


#ifndef MDL_SDK_EXAMPLES_MDL_BROWSER_PLATFORM_HELPER_H
#define MDL_SDK_EXAMPLES_MDL_BROWSER_PLATFORM_HELPER_H

#include <string>
#include <functional>
#include <vector>

class Platform_helper
{
public:

    // get the current working directory
    static std::string get_working_directory();

    // get the path of the executable of this application
    static std::string get_executable_directory();

    // get date of the last modification of a file
    static time_t get_file_change_time(const std::string& path);

    // get the current point in time for measuring time differences in seconds
    static double get_time();

    // measure the time required to run some function
    static double tic_toc(const std::function<void()>& action);

    // measure the time required to run some function and print it to the log
    static double tic_toc_log(const std::string& name, const std::function<void()>& action);

    // Ensures that the console with the log messages does not close immediately. On Windows, users
    // are asked to press enter. On other platforms, nothing is done as the examples are most likely
    // started from the console anyway.
    static void keep_console_open();

    // get the value of a given environment variable or an empty string if the variable is not
    // defined or empty
    static std::string get_environment_variable(const std::string& env_var);

    // returns the paths configured by material library installers (environment variable)
    // or if non is defined, the platform dependent standard folder
    static std::vector<std::string> get_mdl_admin_space_directories();

    // returns the paths configured by material library installers (environment variable)
    // or if non is defined the platform dependent standard folder
    static std::vector<std::string> get_mdl_user_space_directories();
};



#endif

/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/

 /// \file
 /// \brief external interface of the plug-in used by applications.

#ifndef MDL_SDK_EXAMPLES_MDL_QTPLUGIN_H
#define MDL_SDK_EXAMPLES_MDL_QTPLUGIN_H

#include <QApplication>
#include <QDebug>
#include <QPluginLoader>
#include <QQmlExtensionPlugin>
#include <thread>
#include "../view_model/view_model.h"

#include <mi/base/handle.h>
namespace mi
{
    namespace neuraylib
    {
        class INeuray;
        class ITransaction;
    }
}

class Mdl_qt_plugin_interface;
Q_DECLARE_INTERFACE(Mdl_qt_plugin_interface, "com.qt.mdl.plugin_interface")

class QQmlApplicationEngine;

struct Mdl_browser_callbacks
{
    std::function<void(const std::string&)> on_accepted;
    std::function<void()> on_rejected;
};

struct Mdl_qt_plugin_context
{
    // top level interface of the MDL SDK.
    mi::base::Handle<mi::neuraylib::INeuray> neuray;

    // transaction to use while generating the cache.
    mi::base::Handle<mi::neuraylib::ITransaction> transaction;

    // Force to cache to rebuild.
    bool rebuild_module_cache = false;

    // callbacks for mdl browser events.
    Mdl_browser_callbacks mdl_browser;
};

// used with non-qt applications
struct Mdl_qt_plguin_browser_handle
{
    // qualified name of the selected material or empty
    // is available after joining the thread.
    std::string result = "";

    // true if a material was selected, false if the interaction was aborted
    // is available after joining the thread.
    bool accepted = false;

    // thread in which the dialog window lives.
    // call join to wait for completion of the interaction (accept or abort)
    std::thread thread;
};

// application interface to the plugin
class Mdl_qt_plugin_interface
{
public:
    virtual ~Mdl_qt_plugin_interface() = default;

    // To be called from the application to load and initialize the plug-in
    static Mdl_qt_plugin_interface* load(
        const char* plugin_path = nullptr)  // optional path to look for the module folder
    {
        Mdl_qt_plugin_interface* plugin_interface = nullptr;

        #if defined(Q_OS_WIN)
            const QString fileSuffix = ".dll";
        #elif defined(Q_OS_MACOS)
            const QString fileSuffix = ".dylib";
        #else
            const QString fileSuffix = ".so";
        #endif

        QString path;
        if (plugin_path)
        {
            // user provided path that contains the module folder, 
            // which contains the plugin dll and the qmldir
            std::string plugin_path_s(plugin_path);
            std::replace(plugin_path_s.begin(), plugin_path_s.end(), '\\', '/');
            if (plugin_path_s.back() == '/')
                plugin_path_s = plugin_path_s.substr(0, plugin_path_s.length() - 1);

            path = QString(plugin_path_s.c_str()) + "/MdlQtPlugin/mdl_qt_plugin" + fileSuffix;
        }
        else
        {
            // assuming the working directory contains the module folder
            path = QString("MdlQtPlugin/mdl_qt_plugin") + fileSuffix;
        }
        
        QPluginLoader* loader = new QPluginLoader(path);

        loader->load();
        if (loader->isLoaded())
        {
            plugin_interface = qobject_cast<Mdl_qt_plugin_interface*>(loader->instance());
            if (!plugin_interface->initialize(loader))
            {
                qDebug() << "[error] Failed to init the Mdl_qt_plugin.";
                loader->unload();
                return nullptr;
            }

            qDebug() << "Plugin: loaded MdlQtPlugin v.1.0\n";
            qDebug() << "Location: " << loader->fileName() << "\n";
        }
        else
        {
            qDebug() << "[error] while opening plugin: " << loader->errorString() << "\n";
        }

        return plugin_interface;
    }

    // connect the plugin with the MDL SDK instances of the application.
    // meant to be used with Qt-based applications.
    virtual bool set_context(QQmlApplicationEngine* engine, Mdl_qt_plugin_context* context) = 0;

    // in case the application is not based on qt, the browser can be used as standalone window.
    // meant to be used for non-Qt-based applications.
    virtual void show_select_material_dialog(Mdl_qt_plugin_context* context, Mdl_qt_plguin_browser_handle& out_handle) = 0;

    // to be called from the application to unload the plugin and free it's resources.
    virtual void unload() = 0;

protected:
    // internal function that takes ownership of the loader in order to unload the plugin.
    virtual bool initialize(QPluginLoader* loader) = 0;

};

#endif

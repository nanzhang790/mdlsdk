/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/

#include "plugin.h"
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QtQuick>
#include <QtQuickControls2/QQuickStyle>
#include <iostream>
#include <thread>
#include <chrono>

#include "view_model/view_model.h"
#include "view_model/navigation/vm_nav_stack.h"
#include "view_model/navigation/vm_nav_stack_level_model.h"
#include "view_model/navigation/vm_nav_stack_level_proxy_model.h"
#include "view_model/navigation/vm_nav_package.h"

#include "view_model/selection/vm_sel_model.h"
#include "view_model/selection/vm_sel_proxy_model.h"
#include "view_model/selection/vm_sel_element.h"
#include "mdl_browser_settings.h"
#include "utilities/qt/mdl_archive_image_provider.h"
#include "utilities/platform_helper.h"

Mdl_qt_plugin::Mdl_qt_plugin()
    : m_engine(nullptr)
    , m_view_model(nullptr)
    , m_loader(nullptr)
{
}

void Mdl_qt_plugin::registerTypes(const char *uri)
{
    Q_ASSERT(uri == QLatin1String("MdlQtPlugin"));
    qmlRegisterType<View_model>(uri, 1, 0, "ViewModel");
    qmlRegisterType<VM_nav_stack>(uri, 1, 0, "NavStack");
    qmlRegisterType<VM_nav_stack_level_model>(uri, 1, 0, "NavStackLevelModel");
    qmlRegisterType<VM_nav_stack_level_proxy_model>(uri, 1, 0, "NavStackLevelProxyModel");
    qmlRegisterType<VM_nav_package>(uri, 1, 0, "NavPackage");
    qmlRegisterType<VM_sel_model>(uri, 1, 0, "SelModel");
    qmlRegisterType<VM_sel_proxy_model>(uri, 1, 0, "SelProxyModel");
    qmlRegisterType<VM_sel_element>(uri, 1, 0, "SelElement");
    qmlRegisterType<Mdl_browser_settings>(uri, 1, 0, "MdlBrowserSettings");
}

void Mdl_qt_plugin::initializeEngine(QQmlEngine* engine, const char * uri)
{
    QQmlExtensionPlugin::initializeEngine(engine, uri);
}

bool Mdl_qt_plugin::set_context(QQmlApplicationEngine* engine, Mdl_qt_plugin_context* context)
{
    m_engine = engine;

    // attach c++ back-end
    m_view_model = new View_model(
        context->neuray.get(),
        context->transaction.get(),
        &context->mdl_browser,
        context->rebuild_module_cache,
        Platform_helper::get_executable_directory().c_str());

    m_engine->rootContext()->setContextProperty("vm_mdl_browser", m_view_model);

    // image provided for mdl archive thumbnails (takes ownership)
    m_engine->addImageProvider(QLatin1String("mdl_archive"),
                                new Mdl_archive_image_provider(context->neuray.get()));
    return true;
}

bool Mdl_qt_plugin::initialize(QPluginLoader* loader)
{
    m_loader = loader;
    return true;
}

void Mdl_qt_plugin::show_select_material_dialog(Mdl_qt_plugin_context* context, Mdl_qt_plguin_browser_handle& out_handle)
{
    out_handle.result = "";
    out_handle.accepted = false;
    out_handle.thread = std::thread([&]()
    {
        // global Qt Setting that is used for the example
        QQuickStyle::setStyle("Material");
        #if defined(Q_OS_WIN)
            QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
        #endif

        // view model as connection between C++ and QML world
            View_model* view_model = new View_model(
                context->neuray.get(),
                context->transaction.get(),
                &context->mdl_browser,
                context->rebuild_module_cache,
            Platform_helper::get_executable_directory().c_str());

        // create and run an internal application
        {
            int argc = 0;
            QGuiApplication app(argc, nullptr);
            QQmlApplicationEngine engine;

            engine.rootContext()->setContextProperty("vm_mdl_browser", view_model);

            // image provided for mdl archive thumbnails (takes ownership)
            engine.addImageProvider(QLatin1String("mdl_archive"),
                                    new Mdl_archive_image_provider(context->neuray.get()));

            // setup callbacks to get the result
            context->mdl_browser.on_accepted = [&](const std::string& s)
            {
                out_handle.result = s;
                out_handle.accepted = true;
            };
            context->mdl_browser.on_rejected = [&]()
            {
                out_handle.result = "";
                out_handle.accepted = false;
            };

            // Run the Application
            app.setWindowIcon(QIcon(":/mdlqtplugin/graphics/mdl_icon.svg"));
            engine.load(":/mdlqtplugin/BrowserApp.qml");
            int exit_code = app.exec();
            if (exit_code != 0)
            {
                qDebug() << "[error] Qt application crashed.\n";
                out_handle.result = "";
                out_handle.accepted = false;
            }
            engine.removeImageProvider(QLatin1String("mdl_archive"));
        }
        delete view_model;
    });
}

void Mdl_qt_plugin::unload()
{
    if (m_view_model)
        delete m_view_model;

    m_loader->unload();
}

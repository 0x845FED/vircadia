#pragma once

#include <QString>
#include <QObject>

class PluginContainer;

class Plugin : public QObject {
public:
    virtual const QString& getName() const = 0;
    virtual bool isSupported() const;

    /// Called when plugin is initially loaded, typically at application start
    virtual void init();
    /// Called when application is shutting down
    virtual void deinit();

    /// Called when a plugin is being activated for use.  May be called multiple times.
    virtual void activate(PluginContainer* container) = 0;
    /// Called when a plugin is no longer being used.  May be called multiple times.
    virtual void deactivate(PluginContainer* container) = 0;

    /**
     * Called by the application during it's idle phase.  If the plugin needs to do
     * CPU intensive work, it should launch a thread for that, rather than trying to
     * do long operations in the idle call
     */
    virtual void idle();
};

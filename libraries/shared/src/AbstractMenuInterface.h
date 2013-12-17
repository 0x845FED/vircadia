//
//  AbstractMenuInterface.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/16/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//

#ifndef __hifi__AbstractMenuInterface__
#define __hifi__AbstractMenuInterface__

#include <QMenuBar>
//#include <QHash>
//#include <QKeySequence>

class AbstractMenuInterface {
public:
    virtual QMenu* getActiveScriptsMenu() = 0;
    virtual QAction* addActionToQMenuAndActionHash(QMenu* destinationMenu,
                                           const QString actionName,
                                           const QKeySequence& shortcut = 0,
                                           const QObject* receiver = NULL,
                                           const char* member = NULL,
                                           QAction::MenuRole role = QAction::NoRole) = 0;
    virtual void removeAction(QMenu* menu, const QString& actionName) = 0;
};

#endif /* defined(__hifi__AbstractMenuInterface__) */
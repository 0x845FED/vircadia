//
//  main.cpp
//  domain-server/src
//
//  Created by Philip Rosedale on 11/20/12.
//  Copyright 2012 High Fidelity, Inc.
//
//  The Domain Server keeps a list of nodes that have connected to it, and echoes that list of
//  nodes out to nodes when they check in.
//
//  The connection is stateless... the domain server will set you inactive if it does not hear from
//  you in LOGOFF_CHECK_INTERVAL milliseconds, meaning your info will not be sent to other users.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QCoreApplication>

#include <Logging.h>
#include <SharedUtil.h>

#include "DomainServer.h"

int main(int argc, char* argv[]) {
#ifndef WIN32
    setvbuf(stdout, NULL, _IOLBF, 0);
#endif
    
    qInstallMessageHandler(Logging::verboseMessageHandler);
    DomainServer domainServer(argc, argv);
    
    return domainServer.exec();
}


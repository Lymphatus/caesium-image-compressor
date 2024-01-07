#include "PostCompressionActions.h"

void PostCompressionActions::runAction(PostCompressionAction action)
{
    switch (action) {
    case PostCompressionAction::CLOSE_APPLICATION:
        closeApplication();
        break;
    case PostCompressionAction::SHUTDOWN:
        shutdownMachine();
        break;
    case PostCompressionAction::SLEEP:
        putMachineToSleep();
        break;
    default:
    case PostCompressionAction::OPEN_FOLDER:
    case PostCompressionAction::NO_ACTION:
        break;
    }
}

void PostCompressionActions::runAction(PostCompressionAction action, const QString& folder)
{
    switch (action) {
    case PostCompressionAction::OPEN_FOLDER:
        openOutputFolder(folder);
        break;
    default:
        return runAction(action);
    }
}

void PostCompressionActions::closeApplication()
{
    QCoreApplication::quit();
}

void PostCompressionActions::shutdownMachine()
{
#ifdef Q_OS_WIN
    system("shutdown /s");
#endif

#if defined(Q_OS_MAC) || defined(Q_OS_LINUX)
    system("shutdown -h now");
#endif
}

void PostCompressionActions::putMachineToSleep()
{
#ifdef Q_OS_WIN
    system("rundll32.exe powrprof.dll,SetSuspendState 0,1,0");
#endif

#ifdef Q_OS_MAC
    system("pmset sleepnow");
#endif

#ifdef Q_OS_LINUX
    system("systemctl suspend");
#endif
}

void PostCompressionActions::openOutputFolder(const QString& folder)
{
    showDirectoryInNativeFileManager(folder);
}

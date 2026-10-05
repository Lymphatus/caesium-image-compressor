#include <QtCore>
// Keep Windows' SIZE typedef separate from Utils.h's CompressionMode::SIZE.
#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#else
#include <unistd.h>
#endif

quint64 privateBytes()
{
#ifdef Q_OS_WIN
    PROCESS_MEMORY_COUNTERS_EX counters{};
    if (!GetProcessMemoryInfo(GetCurrentProcess(),
                             reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters), sizeof(counters)))
        return 0;
    return counters.PrivateUsage;
#else
    QFile statm("/proc/self/statm");
    if (!statm.open(QIODevice::ReadOnly)) return 0;
    const auto fields = statm.readAll().simplified().split(' ');
    return fields.size() > 1 ? fields[1].toULongLong() * quint64(sysconf(_SC_PAGESIZE)) : 0;
#endif
}

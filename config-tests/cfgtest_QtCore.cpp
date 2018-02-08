#include <sip.h>

#include <QCoreApplication>
#include <QFile>
#include <QLibraryInfo>
#include <QTextStream>


int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QFile outf(argv[1]);

    if (!outf.open(QIODevice::WriteOnly|QIODevice::Truncate|QIODevice::Text))
        return 1;

    QTextStream out(&outf);

    // This allows the version of sip.h to be checked against the code
    // generator.
    out << SIP_VERSION_STR << '\n';

    // This is not a feature and needs to be handled separately.
#if defined(QT_SHARED) || defined(QT_DLL)
    out << "shared\n";
#else
    out << "static\n";
#endif

    // Determine which features should be disabled.

#if defined(QT_NO_PROCESS)
    out << "PyQt_Process\n";
#endif

#if QT_VERSION < 0x050200
// This is the test used in qglobal.h in Qt prior to v5.2.  In v5.2 and later
// qreal is double unless QT_COORD_TYPE is defined.
#if defined(QT_NO_FPU) || defined(Q_PROCESSOR_ARM) || defined(Q_OS_WINCE)
    out << "PyQt_qreal_double\n";
#endif
#else
    if (sizeof (qreal) != sizeof (double))
        out << "PyQt_qreal_double\n";
#endif

    return 0;
}

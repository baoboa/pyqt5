#include <QCoreApplication>
#include <QFile>
#include <QLibraryInfo>
#include <QTextStream>

#include <qfont.h>


int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QFile outf(argv[1]);

    if (!outf.open(QIODevice::WriteOnly|QIODevice::Truncate|QIODevice::Text))
        return 1;

    QTextStream out(&outf);

    // The link test.
    new QFont();

    // Determine which features should be disabled.

#if defined(QT_NO_ACCESSIBILITY)
    out << "PyQt_Accessibility\n";
#endif

#if defined(QT_NO_OPENGL)
    out << "PyQt_OpenGL\n";
    out << "PyQt_Desktop_OpenGL\n";
#elif defined(QT_OPENGL_ES) || defined(QT_OPENGL_ES_2) || defined(QT_OPENGL_ES_3)
    out << "PyQt_Desktop_OpenGL\n";
#endif

#if defined(QT_NO_RAWFONT)
    out << "PyQt_RawFont\n";
#endif

#if defined(QT_NO_SESSIONMANAGER)
    out << "PyQt_SessionManager\n";
#endif

    return 0;
}

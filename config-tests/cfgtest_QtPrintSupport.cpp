#include <QCoreApplication>
#include <QFile>
#include <QLibraryInfo>
#include <QTextStream>

#include <qprinter.h>


int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QFile outf(argv[1]);

    if (!outf.open(QIODevice::WriteOnly|QIODevice::Truncate|QIODevice::Text))
        return 1;

    QTextStream out(&outf);

    // The link test.
    new QPrinter();

    // Determine which features should be disabled.

#if defined(QT_NO_PRINTDIALOG)
    out << "PyQt_PrintDialog\n";
#endif

#if defined(QT_NO_PRINTER)
    out << "PyQt_Printer\n";
#endif

#if defined(QT_NO_PRINTPREVIEWDIALOG)
    out << "PyQt_PrintPreviewDialog\n";
#endif

#if defined(QT_NO_PRINTPREVIEWWIDGET)
    out << "PyQt_PrintPreviewWidget\n";
#endif

    return 0;
}

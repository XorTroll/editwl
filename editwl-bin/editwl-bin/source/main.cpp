#include <ui/ui_MainWindow.hpp>
#include <mod/mod_Loader.hpp>
#include <QApplication>
#include <QTimer>
#include <QDebug>
#include <QFile>

namespace {

    int CliMain(QApplication &app) {
        auto args = QApplication::arguments();
        args.pop_front();

        const QString cmd = args.first();
        args.pop_front();

        auto ok = false;
        for(const auto &module: etwl::mod::GetModules()) {
            if(module.symbols.try_handle_cmd_fn(cmd, args)) {
                ok = true;
                break; 
            }
        }

        if(!ok) {
            qWarning() << "No match for command '" << cmd << "'... do you have the required module installed?";
        }

        QTimer::singleShot(0, &app, &QCoreApplication::quit);
        return app.exec();
    }

    int UiMain(QApplication &app, const QString &open_file = {}) {
        ui::MainWindow win;

        if(!open_file.isEmpty()) {
            win.Open(open_file);
        }

        win.show();

        return app.exec();
    }

}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    InitializeResults();
    etwl::mod::LoadModules();

    if(argc > 1) {
        const auto arg_1 = argv[1];
        if(QFile::exists(arg_1)) {
            // Argument is a file, open UI with it
            return UiMain(app, arg_1);
        }
        else {
            return CliMain(app);
        }
    }
    else {
        return UiMain(app);
    }
}

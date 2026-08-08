#include "rom/rom.hpp"
#include <algorithm>
#include <base_Include.hpp>
#include <map>
#include <string>
#include <ui/ui_MainWindow.hpp>
#include <tuple>
#include <QApplication>
#include <QTimer>
#include <QDebug>
#include <QFile>
#include <vector>

#include <base_Modules.hpp>

namespace {

    using CommandHandler = void(*)(const std::vector<std::string>&);

    constexpr std::tuple<const char*, CommandHandler> g_handlers[] = {
        { "bmg", cli::bmg::HandleCommand },
        { "rom", cli::rom::HandleCommand },
    };

}

namespace {

    inline std::vector<std::string> ConvertQStringListToVector(const QStringList &list) {
        const auto q_vec = list.toVector();

        std::vector<std::string> vec;
        vec.reserve(q_vec.size());
        for(const QString &qstr : q_vec) {
            vec.push_back(qstr.toStdString());
        }
        return vec;
    }

    int CliMain(QApplication &app) {
        auto args = QApplication::arguments();
        args.pop_front();

        const QString cmd = args.first();
        args.pop_front();

        auto handled = false;
        for(const auto &[handler_cmd, handler_fn]: g_handlers) {
            if(cmd == handler_cmd) {
                handler_fn(ConvertQStringListToVector(args));
                handled = true;
                break;
            }
        }
        if(!handled) {
            qCritical() << "No match for command '" << cmd << "'...";
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

#include "qobject.h"
#include <bmg/bmg.hpp>
#include <args.hxx>
#include <twl/fmt/fmt_BMG.hpp>
#include <twl/util/util_String.hpp>
#include <bmg/ui/ui_BmgSubWindow.hpp>
#include <QFileInfo>

namespace ui::bmg {

    bool HandleInputFile(MainWindow *win, const QString &path) {
        twl::fs::StdioFile rf(path.toStdString());
        auto rc = rf.OpenRead();
        if(rc.IsSuccess()) {
            twl::ScopeGuard close_f([&]() {
                rf.Close();
            });

            twl::fmt::BMG bmg;
            rc = bmg.ReadFrom(rf);
            if(rc.IsSuccess()) {
                auto subwin = new BmgSubWindow(win, std::move(bmg), rf.GetPath());

                QFileInfo file_info(QString::fromStdString(rf.GetPath()));
                subwin->setWindowTitle("BMG editor - " + file_info.fileName());

                win->ShowSubWindow(subwin);
                return true;
            }
        }

        return false;
    }

}

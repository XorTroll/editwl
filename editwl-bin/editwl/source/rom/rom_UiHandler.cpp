#include "qobject.h"
#include <rom/rom.hpp>
#include <args.hxx>
#include <twl/fmt/fmt_ROM.hpp>
#include <QFileInfo>

namespace ui::rom {

    bool HandleInputFile(MainWindow *win, const QString &path) {
        twl::fs::StdioFile rf(path.toStdString());
        auto rc = rf.OpenRead();
        if(rc.IsSuccess()) {
            twl::ScopeGuard close_f([&]() {
                rf.Close();
            });

            twl::fmt::ROM rom;
            rc = rom.ReadFrom(rf);
            if(rc.IsSuccess()) {
                // TODO
                return true;
            }
        }

        return false;
    }

}


#pragma once
#include <ui/ui_MainWindow.hpp>

namespace ui::rom {

    bool HandleInputFile(MainWindow *win, const QString &path);

}

namespace cli::rom {

    void HandleCommand(const std::vector<std::string> &args);

}

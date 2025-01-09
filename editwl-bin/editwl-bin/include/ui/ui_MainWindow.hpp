
#pragma once
#include <mod/mod_Module.hpp>
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
    class MainWindow;
}
QT_END_NAMESPACE

namespace ui {

    class MainWindow : public QMainWindow {
        Q_OBJECT

        public:
            MainWindow(QWidget *parent = nullptr);
            ~MainWindow();

            static constexpr int MessageTimeoutMs = 3000;
            static constexpr auto DefaultTitle = "editwl-bin";

            void Open(const QString &file_path);

        private:
            void OnActionOpenClick();
            void OnActionSaveClick();

            void OnActionImportClick();
            void OnActionExportClick();

            void OnActionAboutClick();

            inline void SetTitleBySubWindow(QMdiSubWindow *subwin) {
                this->setWindowTitle("editwl-bin | " + subwin->windowTitle());
            }

            bool SaveSubWindow(etwl::mod::SubWindow *subwin);

            void OnMdiAreaSubWindowActivated(QMdiSubWindow *subwin);
            static void OnSubWindowClosed(QMainWindow *win);
            static bool OnFocusedSubWindowSave(QMainWindow *win);

        private:
            etwl::mod::Context *ctx;
            Ui::MainWindow *win_ui;
    };

}

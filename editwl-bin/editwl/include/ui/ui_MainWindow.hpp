
#pragma once
#include <ui/ui_SubWindow.hpp>
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
            static constexpr auto DefaultTitle = "editwl GUI";

            void Open(const QString &file_path);
            void ShowSubWindow(SubWindow *subwin);

            void OnMdiAreaSubWindowActivated(QMdiSubWindow *subwin);
            void OnSubWindowClosed();
            bool OnFocusedSubWindowSave();

        private:
            void OnActionOpenClick();
            void OnActionSaveClick();

            void OnActionImportClick();
            void OnActionExportClick();

            void OnActionAboutClick();

            inline void SetTitleBySubWindow(QMdiSubWindow *subwin) {
                this->setWindowTitle("editwl GUI | " + subwin->windowTitle());
            }

            bool SaveSubWindow(SubWindow *subwin);

            

        private:
            Ui::MainWindow *win_ui;
    };

}

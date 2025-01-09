#include "./ui_ui_MainWindow.h"
#include <ui/ui_MainWindow.hpp>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QFileDialog>
#include <twl/util/util_String.hpp>
#include <twl/fs/fs_File.hpp>
#include <mod/mod_Loader.hpp>

namespace ui {

    MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), win_ui(new Ui::MainWindow) {
        this->win_ui->setupUi(this);

        this->ctx = new etwl::mod::Context(this->win_ui->mdiArea, this, MainWindow::OnSubWindowClosed, MainWindow::OnFocusedSubWindowSave);

        connect(this->win_ui->actionOpen, &QAction::triggered, this, &MainWindow::OnActionOpenClick);
        connect(this->win_ui->actionSave, &QAction::triggered, this, &MainWindow::OnActionSaveClick);
        connect(this->win_ui->actionImport, &QAction::triggered, this, &MainWindow::OnActionImportClick);
        connect(this->win_ui->actionExport, &QAction::triggered, this, &MainWindow::OnActionExportClick);
        connect(this->win_ui->actionAbout, &QAction::triggered, this, &MainWindow::OnActionAboutClick);

        connect(this->win_ui->mdiArea, &QMdiArea::subWindowActivated, this, &MainWindow::OnMdiAreaSubWindowActivated);

        // TODO: onclose, no cerrar si hay cositas sin guardar
    }

    MainWindow::~MainWindow() {
        delete this->ctx;
        delete this->win_ui;
    }

    void MainWindow::Open(const QString &file_path) {
        auto ok = false;
        for(const auto &mod_obj: etwl::mod::GetModules()) {
            if(mod_obj.symbols.try_handle_input_fn(file_path, this->ctx)) {
                ok = true;
                break;
            }
        }

        if(!ok) {
            QMessageBox::warning(this, DefaultTitle, "Unknown file format. Are you missing a module?");
        }
    }

    void MainWindow::OnActionOpenClick() {
        const auto file_path = QFileDialog::getOpenFileName(this, "Open file");
        if(!file_path.isEmpty()) {
            this->Open(file_path);
        }
    }

    void MainWindow::OnActionSaveClick() {
        auto subwin = reinterpret_cast<etwl::mod::SubWindow*>(this->win_ui->mdiArea->activeSubWindow());
        if(subwin != nullptr) {
            if(subwin->HasChildren()) {
                QMessageBox::warning(this, DefaultTitle, "Window still has unclosed child windows...");
            }
            else if(subwin->NeedsSaving()) {
                this->SaveSubWindow(subwin);
            }
            else {
                this->win_ui->statusBar->showMessage("No changes need to be saved...", MessageTimeoutMs);
            }
        }
    }

    void MainWindow::OnActionImportClick() {
        auto subwin = reinterpret_cast<etwl::mod::SubWindow*>(this->win_ui->mdiArea->activeSubWindow());
        if(subwin != nullptr) {
            if(subwin->HasChildren()) {
                QMessageBox::warning(this, "Nedit", "Window still has unclosed child windows...");
            }
            else {
                const auto rc = subwin->Import();
                if(rc.IsSuccess()) {
                    this->win_ui->statusBar->showMessage("IMport ok!", MessageTimeoutMs);
                }
                else { 
                    QMessageBox::critical(this, "Nedit", "Import fail: " + FormatResult(rc));
                }
            }
        }
    }

    void MainWindow::OnActionExportClick() {
        auto subwin = reinterpret_cast<etwl::mod::SubWindow*>(this->win_ui->mdiArea->activeSubWindow());
        if(subwin != nullptr) {
            if(subwin->HasChildren()) {
                QMessageBox::warning(this, "Nedit", "Window still has unclosed child windows...");
            }
            else {
                const auto rc = subwin->Export();
                if(rc.IsSuccess()) {
                    this->win_ui->statusBar->showMessage("EXport ok!", MessageTimeoutMs);
                }
                else {
                    QMessageBox::critical(this, "Nedit", "Export fail: " + FormatResult(rc));
                }
            }
        }
    }
    
    void MainWindow::OnActionAboutClick() {
        QMessageBox::information(this, "About", "editwlllllll");
    }

    void MainWindow::OnMdiAreaSubWindowActivated(QMdiSubWindow *subwin) {
        if(subwin != nullptr) {
            this->SetTitleBySubWindow(subwin);
        }
    }

    bool MainWindow::SaveSubWindow(etwl::mod::SubWindow *subwin) {
        const auto rc = subwin->Save();
        if(rc.IsSuccess()) {
            this->win_ui->statusBar->showMessage("Saved ok!", MessageTimeoutMs);
            return true;
        }
        else {
            QMessageBox::critical(this, DefaultTitle, "Saved fail: " + FormatResult(rc));
            return false;
        }
    }

    void MainWindow::OnSubWindowClosed(QMainWindow *win) {
        auto main_win = reinterpret_cast<MainWindow*>(win);
        if(main_win->win_ui->mdiArea->subWindowList().empty()) {
            main_win->setWindowTitle(DefaultTitle);
        }
        else {
            auto subwin = main_win->win_ui->mdiArea->activeSubWindow();
            if(subwin != nullptr) {
                main_win->SetTitleBySubWindow(subwin);
            }
        }
    }

    bool MainWindow::OnFocusedSubWindowSave(QMainWindow *win) {
        auto main_win = reinterpret_cast<MainWindow*>(win);
        if(!main_win->win_ui->mdiArea->subWindowList().empty()) {
            auto subwin = main_win->win_ui->mdiArea->activeSubWindow();
            if(subwin != nullptr) {
                return main_win->SaveSubWindow(reinterpret_cast<etwl::mod::SubWindow*>(subwin));
            }
        }

        // Really should not happen
        return false;
    }

}

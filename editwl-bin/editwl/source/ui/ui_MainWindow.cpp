#include "./ui_ui_MainWindow.h"
#include <ui/ui_MainWindow.hpp>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QFileDialog>
#include <twl/util/util_String.hpp>
#include <twl/fs/fs_File.hpp>

#include <base_Modules.hpp>

namespace {

    using FileInputHandler = bool(*)(ui::MainWindow*, const QString&);

    constexpr FileInputHandler g_handlers[] = {
        ui::bmg::HandleInputFile,
        ui::rom::HandleInputFile,
    };

}

namespace ui {

    MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), win_ui(new Ui::MainWindow) {
        this->win_ui->setupUi(this);

        connect(this->win_ui->actionOpen, &QAction::triggered, this, &MainWindow::OnActionOpenClick);
        connect(this->win_ui->actionSave, &QAction::triggered, this, &MainWindow::OnActionSaveClick);
        connect(this->win_ui->actionImport, &QAction::triggered, this, &MainWindow::OnActionImportClick);
        connect(this->win_ui->actionExport, &QAction::triggered, this, &MainWindow::OnActionExportClick);
        connect(this->win_ui->actionAbout, &QAction::triggered, this, &MainWindow::OnActionAboutClick);

        connect(this->win_ui->mdiArea, &QMdiArea::subWindowActivated, this, &MainWindow::OnMdiAreaSubWindowActivated);

        // TODO: onclose?
    }

    MainWindow::~MainWindow() {
        delete this->win_ui;
    }

    void MainWindow::Open(const QString &file_path) {
        auto ok = false;
        for(const auto &handler_fn: g_handlers) {
            if(handler_fn(this, file_path)) {
                ok = true;
                break;
            }
        }

        if(!ok) {
            QMessageBox::warning(this, DefaultTitle, "Unknown file format...");
        }
    }

    void MainWindow::ShowSubWindow(SubWindow *subwin) {
        this->win_ui->mdiArea->addSubWindow(subwin);
        subwin->show();
    }

    void MainWindow::OnActionOpenClick() {
        const auto file_path = QFileDialog::getOpenFileName(this, "Open file");
        if(!file_path.isEmpty()) {
            this->Open(file_path);
        }
    }

    void MainWindow::OnActionSaveClick() {
        auto subwin = reinterpret_cast<SubWindow*>(this->win_ui->mdiArea->activeSubWindow());
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
        auto subwin = reinterpret_cast<SubWindow*>(this->win_ui->mdiArea->activeSubWindow());
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
        auto subwin = reinterpret_cast<SubWindow*>(this->win_ui->mdiArea->activeSubWindow());
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

    bool MainWindow::SaveSubWindow(SubWindow *subwin) {
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

    void MainWindow::OnSubWindowClosed() {
        if(this->win_ui->mdiArea->subWindowList().empty()) {
            this->setWindowTitle(DefaultTitle);
        }
        else {
            auto subwin = this->win_ui->mdiArea->activeSubWindow();
            if(subwin != nullptr) {
                this->SetTitleBySubWindow(subwin);
            }
        }
    }

    bool MainWindow::OnFocusedSubWindowSave() {
        if(!this->win_ui->mdiArea->subWindowList().empty()) {
            auto subwin = this->win_ui->mdiArea->activeSubWindow();
            if(subwin != nullptr) {
                return this->SaveSubWindow(reinterpret_cast<SubWindow*>(subwin));
            }
        }

        // Really should not happen
        return false;
    }

}

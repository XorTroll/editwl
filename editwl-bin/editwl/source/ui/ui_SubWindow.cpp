#include <ui/ui_MainWindow.hpp>
#include <ui/ui_SubWindow.hpp>
#include <QCloseEvent>

namespace ui {

    #define _MAIN_WINDOW (reinterpret_cast<MainWindow*>(this->root))

    void SubWindow::ShowChildWindow(SubWindow *child) {
        child->AssignParent(this);
        this->children.push_back(child);
        _MAIN_WINDOW->ShowSubWindow(child);
    }

    bool SubWindow::CanClose() {
        for(auto &child: this->children) {
            if(child->NeedsSaving()) {
                QMessageBox::critical(this, "Window close", "Child window '" + child->windowTitle() + "' has unsaved changes...");
                return false;
            }
        }

        if(this->NeedsSaving()) {
            const auto reply = QMessageBox::question(this, "Window close", "This window needs saving...\nWould you like to save the new changes in the original source?\n\nUse the exporting functionality if you wish to save in a new location.", QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

            if(reply == QMessageBox::Yes) {
                return _MAIN_WINDOW->OnFocusedSubWindowSave();
            }
            else if(reply == QMessageBox::No) {
                return true;
            }
            else if(reply == QMessageBox::Cancel) {
                return false;
            }
        }

        return true;
    }

    void SubWindow::closeEvent(QCloseEvent *event) {
        if(!this->CanClose()) {
            event->ignore();
        }
        else {
            while(!this->children.empty()) {
                this->children.front()->close();
            }
            this->children.clear();

            if(this->parent != nullptr) {
                this->parent->NotifyChildClosed(this);
            }
            _MAIN_WINDOW->OnSubWindowClosed();

            event->accept();
        }
    }

}

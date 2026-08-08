
#pragma once
#include <base_Include.hpp>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMainWindow>
#include <QMessageBox>
#include <QEvent>

namespace ui {

    class SubWindow : public QMdiSubWindow {
        public:
            inline SubWindow(QMainWindow *root) : QMdiSubWindow(nullptr), root(root), parent(nullptr), children() {}
            inline SubWindow(SubWindow *parent) : QMdiSubWindow(nullptr), root(parent->root), parent(parent), children() {}

            virtual bool NeedsSaving() = 0;
            virtual twl::Result Save() = 0;

            virtual twl::Result Import() = 0;
            virtual twl::Result Export() = 0;

            void ShowChildWindow(SubWindow *child);

            inline bool HasChildren() {
                return !this->children.empty();
            }

            bool CanClose();

        protected:
            void closeEvent(QCloseEvent *event) override;

        protected:
            void AssignParent(SubWindow *parent_win) {
                this->parent = parent_win;
            }

            void NotifyChildClosed(SubWindow *child) {
                for(size_t i = 0; i < this->children.size(); i++) {
                    if(children.at(i) == child) {
                        this->children.erase(this->children.begin() + i);
                        return;
                    }
                }
            }

            QMainWindow *root;
            SubWindow *parent;
            std::vector<SubWindow*> children;
    };

}

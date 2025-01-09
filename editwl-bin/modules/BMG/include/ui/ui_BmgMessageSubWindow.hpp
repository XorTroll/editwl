
#pragma once
#include <base_Include.hpp>
#include <ui/ui_BmgSubWindow.hpp>

QT_BEGIN_NAMESPACE
namespace Ui {
    class BmgMessageSubWindow;
}
QT_END_NAMESPACE

namespace ui {

    class BmgMessageSubWindow : public etwl::mod::SubWindow {
        Q_OBJECT

        public:
            BmgMessageSubWindow(etwl::mod::Context *ctx, BmgSubWindow *parent_win, twl::fmt::BMG *bmg_ref, const size_t idx);
            ~BmgMessageSubWindow();

            bool NeedsSaving() override;
            twl::Result Save() override;

            twl::Result Import() override;
            twl::Result Export() override;

            inline size_t GetMessageIndex() {
                return this->msg_idx;
            }

        private:
            BmgSubWindow *parent_win;
            Ui::BmgMessageSubWindow *win_ui;
            twl::fmt::BMG *bmg_ref;
            size_t msg_idx;
    };

}


#pragma once
#include <base_Include.hpp>

QT_BEGIN_NAMESPACE
namespace Ui {
    class BmgSubWindow;
}
QT_END_NAMESPACE

namespace ui {

    class BmgSubWindow : public etwl::mod::SubWindow {
        Q_OBJECT

        public:
            BmgSubWindow(etwl::mod::Context *ctx, twl::fmt::BMG &&bmg, const std::string &read_path);
            ~BmgSubWindow();

            bool NeedsSaving() override;
            twl::Result Save() override;

            twl::Result Import() override;
            twl::Result Export() override;

            void NotifyMessageEdited();
            void ReloadPreviews();

        private:
            twl::fmt::BMG::Encoding GetCurrentEncoding();
            void ReloadFields();
            void OnMessageListViewDoubleClick(const QModelIndex &idx);

            Ui::BmgSubWindow *win_ui;
            twl::fmt::BMG bmg;
            std::string read_path;
            bool msg_edited;
    };

}

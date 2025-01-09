#include "./ui_ui_BmgSubWindow.h"
#include <ui/ui_BmgSubWindow.hpp>
#include <ui/ui_BmgMessageSubWindow.hpp>
#include <twl/fs/fs_File.hpp>
#include <QStringListModel>
#include <QFileDialog>
#include <QStringConverter>
#include <QRegularExpression>
#include <QFileInfo>

namespace ui {

    BmgSubWindow::BmgSubWindow(etwl::mod::Context *ctx, twl::fmt::BMG &&bmgq, const std::string &read_path) : SubWindow(ctx), win_ui(new Ui::BmgSubWindow), bmg(std::move(bmgq)), read_path(read_path), msg_edited(false) {
        this->win_ui->setupUi(this);

        this->win_ui->comboBoxEncoding->addItem(FormatEncoding(twl::fmt::BMG::Encoding::CP1252));
        this->win_ui->comboBoxEncoding->addItem(FormatEncoding(twl::fmt::BMG::Encoding::UTF16));
        this->win_ui->comboBoxEncoding->addItem(FormatEncoding(twl::fmt::BMG::Encoding::ShiftJIS));
        this->win_ui->comboBoxEncoding->addItem(FormatEncoding(twl::fmt::BMG::Encoding::UTF8));
        this->ReloadFields();

        this->win_ui->comboBoxEncoding->setToolTip("Change the encoding used with all messages");

        this->win_ui->lineEditFileId->setToolTip("Change the file ID");

        this->win_ui->listViewMessages->setToolTip("Double-click on a message to edit it");

        this->ReloadPreviews();
        connect(this->win_ui->listViewMessages, &QListView::doubleClicked, this, &BmgSubWindow::OnMessageListViewDoubleClick);
    }

    BmgSubWindow::~BmgSubWindow() {
        delete this->win_ui;
    }

    bool BmgSubWindow::NeedsSaving() {
        if(this->GetCurrentEncoding() != this->bmg.header.encoding) {
            return true;
        }

        twl::u32 cur_file_id;
        if(!ParseStringInteger(this->win_ui->lineEditFileId->text(), cur_file_id)) {
            return true;
        }
        if(cur_file_id != this->bmg.info.file_id) {
            return true;
        }

        if(this->msg_edited) {
            return true;
        }

        return false;
    }

    twl::Result BmgSubWindow::Save() {
        this->bmg.header.encoding = this->GetCurrentEncoding();
        
        if(!ParseStringInteger(this->win_ui->lineEditFileId->text(), this->bmg.info.file_id)) {
            TWL_R_FAIL(ResultBMGInvalidFileId);
        }

        twl::fs::StdioFile bmg_out_file(this->read_path);
        TWL_R_TRY(bmg_out_file.OpenWrite());

        twl::ScopeGuard close_f([&]() {
            bmg_out_file.Close();
        });

        TWL_R_TRY(this->bmg.WriteTo(bmg_out_file));

        this->msg_edited = false;
        TWL_R_SUCCEED();
    }

    twl::Result BmgSubWindow::Import() {
        QStringList filter_list;
        filter_list << NEDIT_MOD_BMG_FORMAT_BMG_FILTER;
        filter_list << NEDIT_MOD_BMG_FORMAT_XML_FILTER;
        const auto file = QFileDialog::getOpenFileName(this, "Import BMG from...", QString(), filter_list.join(";;"));

        if(!file.isEmpty()) {
            const auto file_ext = QFileInfo(file).suffix();
            if(file_ext == NEDIT_MOD_BMG_FORMAT_BMG_EXTENSION) {
                twl::fs::StdioFile bmg_in_file(file.toStdString());
                TWL_R_TRY(bmg_in_file.OpenRead());

                twl::ScopeGuard close_f([&]() {
                    bmg_in_file.Close();
                });

                TWL_R_TRY(this->bmg.ReadFrom(bmg_in_file));
            }
            else if(file_ext == NEDIT_MOD_BMG_FORMAT_XML_EXTENSION) {
                TWL_R_TRY(LoadBmgXml(file, this->bmg));
            }

            this->ReloadFields();
            this->ReloadPreviews();
            this->msg_edited = true;
        }

        TWL_R_SUCCEED();
    }

    twl::Result BmgSubWindow::Export() {
        QStringList filter_list;
        filter_list << NEDIT_MOD_BMG_FORMAT_BMG_FILTER;
        filter_list << NEDIT_MOD_BMG_FORMAT_XML_FILTER;
        const auto file = QFileDialog::getSaveFileName(this, "Export BMG as...", QString(), filter_list.join(";;"));

        if(!file.isEmpty()) {
            this->bmg.header.encoding = this->GetCurrentEncoding();

            const auto file_ext = QFileInfo(file).suffix();
            if(file_ext == NEDIT_MOD_BMG_FORMAT_BMG_EXTENSION) {
                twl::fs::StdioFile bmg_out_file(file.toStdString());
                TWL_R_TRY(bmg_out_file.OpenWrite());

                twl::ScopeGuard close_f([&]() {
                    bmg_out_file.Close();
                });

                TWL_R_TRY(this->bmg.WriteTo(bmg_out_file));
            }
            else if(file_ext == NEDIT_MOD_BMG_FORMAT_XML_EXTENSION) {
                TWL_R_TRY(SaveBmgXml(this->bmg, file));
            }
        }

        TWL_R_SUCCEED();
    }

    void BmgSubWindow::NotifyMessageEdited() {
        this->msg_edited = true;
        this->ReloadPreviews();
    }

    void BmgSubWindow::ReloadPreviews() {
        QStringList msg_preview_list;
        for(const auto &msg: this->bmg.messages) {
            QString preview_text = "<no text preview>";
            for(const auto &token: msg.msg) {
                if(token.type == twl::fmt::BMG::MessageTokenType::Text) {
                    preview_text = QString::fromStdU16String(token.text).split(QRegularExpression("[\r\n]"), Qt::SkipEmptyParts).front();
                    break;
                }
            }
            preview_text += "...";

            msg_preview_list << preview_text;
        }
        this->win_ui->listViewMessages->setModel(new QStringListModel(msg_preview_list));
    }

    twl::fmt::BMG::Encoding BmgSubWindow::GetCurrentEncoding() {
        return static_cast<twl::fmt::BMG::Encoding>(this->win_ui->comboBoxEncoding->currentIndex() + 1);
    }

    void BmgSubWindow::ReloadFields() {
        this->win_ui->comboBoxEncoding->setCurrentIndex(static_cast<size_t>(this->bmg.header.encoding) - 1);
        this->win_ui->lineEditFileId->setText(QString::number(this->bmg.info.file_id));
    }

    void BmgSubWindow::OnMessageListViewDoubleClick(const QModelIndex &idx) {
        const auto msg_idx = idx.row();

        for(auto &subwin: this->children) {
            auto msg_win = reinterpret_cast<BmgMessageSubWindow*>(subwin);
            if(msg_win->GetMessageIndex() == msg_idx) {
                return;
            }
        }

        QFileInfo file_info(QString::fromStdString(this->read_path));

        auto msg_win = new BmgMessageSubWindow(this->ctx, this, &this->bmg, msg_idx);
        msg_win->setWindowTitle(QString("BMG message editor - %1[%2]").arg(file_info.fileName()).arg(QString::number(msg_idx)));
        this->ShowChildWindow(msg_win);
    }

}

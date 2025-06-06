#include "./ui_ui_BmgMessageSubWindow.h"
#include <ui/ui_BmgMessageSubWindow.hpp>
#include <QDebug>
#include <QIntValidator>

namespace ui {

    BmgMessageSubWindow::BmgMessageSubWindow(etwl::mod::Context *ctx, BmgSubWindow *parent_win,twl::fmt::BMG *bmg_ref, const size_t idx) : SubWindow(ctx), parent_win(parent_win), win_ui(new Ui::BmgMessageSubWindow), bmg_ref(bmg_ref), msg_idx(idx) {
        this->win_ui->setupUi(this);

        const auto &msg = bmg_ref->messages.at(idx);
        QString msg_str;
        const auto rc = FormatMessage(msg, msg_str);

        if(bmg_ref->HasMessageIds()) {
            QIntValidator *validator = new QIntValidator(this->win_ui->lineEditMessageId);
            this->win_ui->lineEditMessageId->setValidator(validator);

            this->win_ui->lineEditMessageId->setDisabled(false);
            this->win_ui->lineEditMessageId->setText(QString::number(msg.id));
            this->win_ui->lineEditMessageId->setToolTip("BMG message ID (nonzero decimal integer)");
        }
        else {
            this->win_ui->lineEditMessageId->setDisabled(true);
            this->win_ui->lineEditMessageId->setToolTip("This BMG file does not make use of message IDs (you can change that on the BMG editor window)");
        }

        if(bmg_ref->info.GetAttributesSize() > 0) {
            this->win_ui->lineEditMessageAttributes->setDisabled(false);
            this->win_ui->lineEditMessageAttributes->setText(FormatHexByteArray(msg.attrs));
            this->win_ui->lineEditMessageAttributes->setToolTip(QString("BMG message attribute data (%1-byte hex array, like '01 A2 FF')").arg(QString::number(bmg_ref->info.GetAttributesSize())));
        }
        else {
            this->win_ui->lineEditMessageAttributes->setDisabled(true);
            this->win_ui->lineEditMessageAttributes->setToolTip("This BMG file does not make use of message attributes (you can change that on the BMG editor window)");
        }

        if(rc.IsSuccess()) {
            this->win_ui->textEditMessage->setText(msg_str);
        }
        else {
            QMessageBox::critical(this, "BMG message editor", QString::fromStdString("Unable to load BMG message:\n" + FormatResult(rc)));
        }
    }

    BmgMessageSubWindow::~BmgMessageSubWindow() {
        delete this->win_ui;
    }

    bool BmgMessageSubWindow::NeedsSaving() {
        const auto &msg = this->bmg_ref->messages.at(this->msg_idx);
        
        QString msg_str;
        const auto rc = FormatMessage(msg, msg_str);
        if(rc.IsFailure()) {
            // For a malformed BMG, better to save it
            return true;
        }

        if(msg_str != this->win_ui->textEditMessage->toPlainText()) {
            // Modified message text
            return true;
        }

        if(this->bmg_ref->HasMessageIds()) {
            twl::u32 cur_id;
            if(!ParseStringInteger(this->win_ui->lineEditMessageId->text(), cur_id)) {
                return true;
            }
            return cur_id != msg.id;
        }

        if(!msg.attrs.empty()) {
            std::vector<twl::u8> attrs;
            if(ParseHexByteArray(this->win_ui->lineEditMessageAttributes->text(), attrs)) {
                if(attrs.size() != msg.attrs.size()) {
                    // Different size on new attrs
                    return true;
                }
                
                for(size_t i = 0; i < attrs.size(); i++) {
                    if(attrs.at(i) != msg.attrs.at(i)) {
                        return true;
                    }
                }
            }
            else {
                // Unexpected, better to request saving
                return true;
            }
        }

        return false;
    }

    twl::Result BmgMessageSubWindow::Save() {
        auto &msg = this->bmg_ref->messages.at(this->msg_idx);

        const auto new_msg_raw = this->win_ui->textEditMessage->toPlainText();
        TWL_R_TRY(ParseMessage(new_msg_raw, msg));

        if(this->bmg_ref->HasMessageIds()) {
            twl::u32 new_id;
            if(ParseStringInteger(this->win_ui->lineEditMessageId->text(), new_id)) {
                msg.id = new_id;
            }
            else {
                TWL_R_FAIL(ResultBMGInvalidMessageId);
            }
        }

        if(this->bmg_ref->info.GetAttributesSize() > 0) {
            std::vector<twl::u8> attrs;
            if(ParseHexByteArray(this->win_ui->lineEditMessageAttributes->text(), attrs)) {
                msg.attrs = attrs;
            }
            else {
                TWL_R_FAIL(ResultBMGInvalidAttributes);
            }
        }

        this->parent_win->NotifyMessageEdited();

        TWL_R_SUCCEED();
    }

    twl::Result BmgMessageSubWindow::Import() {
        qDebug() << "Import dummy";

        TWL_R_SUCCEED();
    }

    twl::Result BmgMessageSubWindow::Export() {
        qDebug() << "Export dummy";

        TWL_R_SUCCEED();
    }

}

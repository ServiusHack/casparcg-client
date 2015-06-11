#include "InspectorHttpPostWidget.h"
#include "KeyValueDialog.h"

#include "Global.h"

#include "EventManager.h"
#include "Models/KeyValueModel.h"

#include <QtCore/QDebug>

#include <QtGui/QClipboard>
#include <QtGui/QCursor>
#include <QtGui/QKeyEvent>
#include <QtGui/QResizeEvent>

#include <QtWidgets/QApplication>

InspectorHttpPostWidget::InspectorHttpPostWidget(QWidget* parent)
    : QWidget(parent),
      model(NULL), command(NULL)
{
    setupUi(this);

    this->treeWidgetHttpData->setColumnWidth(1, 87);
    this->fieldCounter = this->treeWidgetHttpData->invisibleRootItem()->childCount();

    QObject::connect(&EventManager::getInstance(), SIGNAL(showAddHttpPostDataDialog(const ShowAddHttpPostDataDialogEvent&)), this, SLOT(showAddHttpPostDataDialog(const ShowAddHttpPostDataDialogEvent&)));
    QObject::connect(&EventManager::getInstance(), SIGNAL(rundownItemSelected(const RundownItemSelectedEvent&)), this, SLOT(rundownItemSelected(const RundownItemSelectedEvent&)));

    this->treeWidgetHttpData->installEventFilter(this);
}

bool InspectorHttpPostWidget::eventFilter(QObject* target, QEvent* event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = dynamic_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Delete)
            return removeRow();
        else if ((keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) && keyEvent->modifiers() == Qt::ShiftModifier)
            return editRow();
        else if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter)
            return addRow();
        else if (keyEvent->key() == Qt::Key_D && keyEvent->modifiers() == Qt::ControlModifier)
            return duplicateSelectedItem();
        else if (keyEvent->key() == Qt::Key_C && keyEvent->modifiers() == Qt::ControlModifier)
            return copySelectedItem();
        else if (keyEvent->key() == Qt::Key_V && keyEvent->modifiers() == Qt::ControlModifier)
            return pasteSelectedItem();
    }

    return QObject::eventFilter(target, event);
}

void InspectorHttpPostWidget::showAddHttpPostDataDialog(const ShowAddHttpPostDataDialogEvent& event)
{
    Q_UNUSED(event);

    int index = this->treeWidgetHttpData->invisibleRootItem()->childCount() - 1;
    this->treeWidgetHttpData->setCurrentItem(this->treeWidgetHttpData->invisibleRootItem()->child(index));

    KeyValueDialog* dialog = new KeyValueDialog(this);
    dialog->setTitle("New HTTP POST Data");
    if (dialog->exec() == QDialog::Accepted)
    {
        QTreeWidgetItem* treeItem = new QTreeWidgetItem();
        treeItem->setText(0, dialog->getKey());
        treeItem->setText(1, dialog->getValue());

        this->treeWidgetHttpData->invisibleRootItem()->insertChild(this->treeWidgetHttpData->currentIndex().row() + 1, treeItem);
        this->treeWidgetHttpData->setCurrentItem(treeItem);
    }
}

void InspectorHttpPostWidget::rundownItemSelected(const RundownItemSelectedEvent& event)
{
    this->command = nullptr;
    this->model = event.getLibraryModel();

    blockAllSignals(true);

    if (dynamic_cast<HttpPostCommand*>(event.getCommand()))
    {
        this->command = dynamic_cast<HttpPostCommand*>(event.getCommand());

        this->lineEditUrl->setText(this->command->getUrl());
        this->checkBoxTriggerOnNext->setChecked(this->command->getTriggerOnNext());

        for (int i = this->treeWidgetHttpData->invisibleRootItem()->childCount() - 1; i >= 0; i--)
            delete this->treeWidgetHttpData->invisibleRootItem()->child(i);

        foreach (KeyValueModel model, this->command->getHttpDataModels())
        {
            QTreeWidgetItem* treeItem = new QTreeWidgetItem();
            treeItem->setText(0, model.getKey());
            treeItem->setText(1, model.getValue());

            this->treeWidgetHttpData->invisibleRootItem()->addChild(treeItem);
        }
    }

    checkEmptyUrl();

    blockAllSignals(false);
}

void InspectorHttpPostWidget::checkEmptyUrl()
{
    if (this->lineEditUrl->text().isEmpty())
        this->lineEditUrl->setStyleSheet("border-color: firebrick;");
    else
        this->lineEditUrl->setStyleSheet("");
}

void InspectorHttpPostWidget::blockAllSignals(bool block)
{
    this->lineEditUrl->blockSignals(block);
    this->checkBoxTriggerOnNext->blockSignals(block);
    this->treeWidgetHttpData->blockSignals(block);
}

void InspectorHttpPostWidget::updateHttpDataModels()
{
    QList<KeyValueModel> models;
    for (int i = 0; i < this->treeWidgetHttpData->invisibleRootItem()->childCount(); i++)
        models.push_back(KeyValueModel(this->treeWidgetHttpData->invisibleRootItem()->child(i)->text(0),
                                       this->treeWidgetHttpData->invisibleRootItem()->child(i)->text(1)));

    this->command->setHttpDataModels(models);
}

bool InspectorHttpPostWidget::addRow()
{
    KeyValueDialog* dialog = new KeyValueDialog(this);
    dialog->move(QPoint(QCursor::pos().x() - dialog->width() + 40, QCursor::pos().y() - dialog->height() - 10));
    dialog->setTitle("New HTTP POST Data");
    if (dialog->exec() == QDialog::Accepted)
    {
        QTreeWidgetItem* treeItem = new QTreeWidgetItem();
        treeItem->setText(0, dialog->getKey());
        treeItem->setText(1, dialog->getValue());

        this->treeWidgetHttpData->invisibleRootItem()->insertChild(this->treeWidgetHttpData->currentIndex().row() + 1, treeItem);
        this->treeWidgetHttpData->setCurrentItem(treeItem);
    }

    return true;
}

bool InspectorHttpPostWidget::editRow()
{
    if (this->treeWidgetHttpData->currentItem() == NULL)
        return true;

    KeyValueDialog* dialog = new KeyValueDialog(this);
    dialog->move(QPoint(QCursor::pos().x() - dialog->width() + 40, QCursor::pos().y() - dialog->height() - 10));
    dialog->setTitle("Edit HTTP POST Data");
    dialog->setKey(this->treeWidgetHttpData->currentItem()->text(0));
    dialog->setValue(this->treeWidgetHttpData->currentItem()->text(1));
    if (dialog->exec() == QDialog::Accepted)
    {
        this->treeWidgetHttpData->currentItem()->setText(0, dialog->getKey());
        this->treeWidgetHttpData->currentItem()->setText(1, dialog->getValue());

        updateHttpDataModels();
    }

    return true;
}

bool InspectorHttpPostWidget::removeRow()
{
    if (this->treeWidgetHttpData->currentItem() == NULL)
        return true;

    delete this->treeWidgetHttpData->currentItem();
    updateHttpDataModels();

    return true;
}

bool InspectorHttpPostWidget::duplicateSelectedItem()
{
    if (!copySelectedItem())
        return true;

    if (!pasteSelectedItem())
        return true;

    return true;
}

bool InspectorHttpPostWidget::copySelectedItem()
{
    QString data;

    if (this->treeWidgetHttpData->selectedItems().count() == 0)
        return true;

    data = this->treeWidgetHttpData->selectedItems().at(0)->text(0);
    data += "#" + this->treeWidgetHttpData->selectedItems().at(0)->text(1);

    qApp->clipboard()->setText(data);

    return true;
}

bool InspectorHttpPostWidget::pasteSelectedItem()
{
    if (qApp->clipboard()->text().isEmpty())
        return true;

    if (qApp->clipboard()->text().split("#").count() < 2)
        return true;

    QTreeWidgetItem* treeItem = new QTreeWidgetItem();
    treeItem->setText(0, qApp->clipboard()->text().split("#").at(0));
    treeItem->setText(1, qApp->clipboard()->text().split("#").at(1));

    this->treeWidgetHttpData->invisibleRootItem()->insertChild(this->treeWidgetHttpData->currentIndex().row() + 1, treeItem);

    return true;
}

void InspectorHttpPostWidget::itemDoubleClicked(QTreeWidgetItem* current, int index)
{
    Q_UNUSED(current);
    Q_UNUSED(index);

    editRow();
}

void InspectorHttpPostWidget::urlChanged(QString url)
{
    this->command->setUrl(url);

    checkEmptyUrl();
}

void InspectorHttpPostWidget::triggerOnNextChanged(int state)
{
    this->command->setTriggerOnNext((state == Qt::Checked) ? true : false);
}

void InspectorHttpPostWidget::currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous)
{
    Q_UNUSED(previous);

    if (current == NULL)
        return;

    updateHttpDataModels();
}

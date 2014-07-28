/*==================================================================================
    Copyright (c) 2008, binaryzebra
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    * Neither the name of the binaryzebra nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE binaryzebra AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL binaryzebra BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=====================================================================================*/


#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "configparser.h"
#include "updatedialog.h"
#include "buttonswidget.h"
#include "selfupdater.h"
#include "defines.h"
#include "filemanager.h"
#include <QSet>
#include <QQueue>
#include <QInputDialog>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    appManager(0)
{
    ui->setupUi(this);
    ui->webView->setContextMenuPolicy(Qt::NoContextMenu);
    ui->webView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
    ui->tableWidget->setStyleSheet(TABLE_STYLESHEET);

    listFontFav.setPointSize(listFontFav.pointSize() + 1);
    listFontFav.setBold(true);

    setWindowTitle(QString("DAVA Launcher %1").arg(LAUNCHER_VER));

    connect(ui->webView, SIGNAL(linkClicked(QUrl)), this, SLOT(OnlinkClicked(QUrl)));
    connect(ui->refreshButton, SIGNAL(clicked()), this, SLOT(OnRefreshClicked()));
    connect(ui->listWidget, SIGNAL(clicked(QModelIndex)), this, SLOT(OnListItemClicked(QModelIndex)));
    connect(ui->setUrlButton, SIGNAL(clicked()), this, SLOT(OnURLClicked()));
    connect(ui->tableWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(OnCellDoubleClicked(QModelIndex)));

    appManager = new ApplicationManager();

    connect(appManager, SIGNAL(Refresh()), this, SLOT(RefreshApps()));

    UpdateURLValue();

    OnRefreshClicked();
}

MainWindow::~MainWindow()
{
    SafeDelete(ui);
    SafeDelete(appManager);
}

void MainWindow::OnURLClicked()
{
    QInputDialog dialog(this);
    dialog.setWindowModality(Qt::WindowModal);
    dialog.setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
    dialog.setTextValue(appManager->GetLocalConfig()->GetRemoteConfigURL());
    dialog.setWindowTitle("Config URL");
    dialog.setLabelText("Input new config URL:");
    dialog.setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    dialog.resize(width() / 3, -1);

    if(dialog.exec() == QDialog::Accepted)
    {
        appManager->GetLocalConfig()->SetRemoteConfigURL(dialog.textValue());
        UpdateURLValue();
    }
}
void MainWindow::OnlinkClicked(QUrl url)
{
    QDesktopServices::openUrl( url );
}

void MainWindow::OnCellDoubleClicked(QModelIndex index)
{
    if(index.column() != COLUMN_BUTTONS)
        OnRun(index.row());
}

void MainWindow::OnRun(int rowNumber)
{
    QString appID, insVersionID, avVersionID;
    GetTableApplicationIDs(rowNumber, appID, insVersionID, avVersionID);
    if(!appID.isEmpty() && !insVersionID.isEmpty())
        appManager->RunApplication(selectedBranchID, appID, insVersionID);
}

void MainWindow::OnRemove(int rowNumber)
{
    QString appID, insVersionID, avVersionID;
    GetTableApplicationIDs(rowNumber, appID, insVersionID, avVersionID);    
    if(!appID.isEmpty() && !insVersionID.isEmpty())
    {
        QMessageBox msbox(QMessageBox::Information, "Remove application",
                          QString("Are you sure you want to remove %1?").arg(appManager->GetString(appID)),
                          QMessageBox::Yes | QMessageBox::No );
        int result = msbox.exec();

        if(result == QMessageBox::Yes && appManager->RemoveApplication(selectedBranchID, appID, insVersionID))
            RefreshApps();
    }
}

void MainWindow::OnInstall(int rowNumber)
{
    QString appID, insVersionID, avVersionID;
    GetTableApplicationIDs(rowNumber, appID, insVersionID, avVersionID);

    AppVersion * version = appManager->GetRemoteConfig()->GetAppVersion(selectedBranchID, appID, avVersionID);
    QQueue<UpdateTask> tasks;
    tasks.push_back(UpdateTask(selectedBranchID, appID, *version));

    ShowUpdateDialog(tasks);

    ShowTable(selectedBranchID);
}

void MainWindow::OnRefreshClicked()
{
    ui->refreshButton->setEnabled(false);
    ui->setUrlButton->setEnabled(false);

    FileManager::Instance()->ClearTempDirectory();

    appManager->RefreshRemoteConfig();
}

void MainWindow::RefreshApps()
{
    QQueue<UpdateTask> tasks;
    appManager->CheckUpdates(tasks);
    ShowUpdateDialog(tasks);

    RefreshBranchesList();

    if(appManager->ShouldShowNews())
        selectedBranchID = CONFIG_LAUNCHER_WEBPAGE_KEY;
    else
        ui->listWidget->setCurrentIndex(selectedListItem);

    ShowTable(selectedBranchID);
}

void MainWindow::OnListItemClicked(QModelIndex qindex)
{
    QString dataRole = ui->listWidget->item(qindex.row())->data(DAVA_WIDGET_ROLE).toString();
    if(!dataRole.isEmpty())
    {
        selectedBranchID = dataRole;
        selectedListItem = ui->listWidget->currentIndex();
        ShowTable(selectedBranchID);
    }
}

void MainWindow::OnCellClicked(const QPoint & pos)
{
    QWidget * cell = (QWidget*)(QObject::sender());

    QMenu menu(this);
    QAction *u = menu.addAction("Copy");
    QAction *a = menu.exec(ui->tableWidget->viewport()->mapToGlobal(pos) + cell->pos());
    if (a == u)
        QApplication::clipboard()->setText(cell->property(DAVA_CUSTOM_PROPERTY_NAME).toString());
}

void MainWindow::ShowWebpage()
{
    ui->stackedWidget->setCurrentIndex(0);

    const QString & webpageUrl = appManager->GetLocalConfig()->GetWebpageURL();
    ui->webView->setUrl(QUrl(webpageUrl));

    appManager->NewsShowed();
}

void MainWindow::ShowTable(const QString & branchID)
{
    ui->refreshButton->setEnabled(true);
    ui->setUrlButton->setEnabled(true);

    if(branchID == CONFIG_LAUNCHER_WEBPAGE_KEY || branchID.isEmpty())
    {
        ShowWebpage();
        return;
    }

    ui->stackedWidget->setCurrentIndex(1);
    ui->tableWidget->setRowCount(0);

    ConfigParser * localConfig = appManager->GetLocalConfig();
    ConfigParser * remoteConfig = appManager->GetRemoteConfig();

    QVector<int> states;
    if(remoteConfig)
    {
        Branch * remoteBranch = remoteConfig->GetBranch(branchID);
        if(remoteBranch)
        {
            int appCount = remoteBranch->GetAppCount();
            ui->tableWidget->setRowCount(appCount);
            for(int i = 0; i < appCount; ++i)
            {
                int state = 0;
                QString avalibleVersion;
                Application * remoteApp = remoteBranch->GetApplication(i);

                QWidget * item = CreateAppNameTableItem(remoteApp->id);
                item->setMinimumWidth(120);
                ui->tableWidget->setCellWidget(i, COLUMN_APP_NAME, item);

                ui->tableWidget->setCellWidget(i, COLUMN_APP_AVAL, CreateAppAvalibleTableItem(remoteApp));

                int versCount = remoteApp->GetVerionsCount();
                if(versCount == 1)
                    avalibleVersion = remoteApp->GetVersion(0)->id;
                else
                    state |= ButtonsWidget::BUTTONS_STATE_AVALIBLE;

                if(localConfig)
                {
                    Application * localApp = localConfig->GetApplication(branchID, remoteApp->id);
                    if(localApp)
                    {
                        ui->tableWidget->setCellWidget(i, COLUMN_APP_INS, CreateAppInstalledTableItem(localApp->GetVersion(0)->id));

                        state |= ButtonsWidget::BUTTONS_STATE_INSTALLED;
                        if(avalibleVersion != localApp->GetVersion(0)->id)
                            state |= ButtonsWidget::BUTTONS_STATE_AVALIBLE;
                    }
                    else
                    {
                        state |= ButtonsWidget::BUTTONS_STATE_AVALIBLE;
                    }
                }

                states.push_back(state);
            }
        }
        else
        {
            Branch * localBranch = localConfig->GetBranch(branchID);
            if(localBranch)
            {
                int appCount = localBranch->GetAppCount();
                ui->tableWidget->setRowCount(appCount);
                for(int i = 0; i < appCount; ++i)
                {
                    Application * localApp = localBranch->GetApplication(i);
                    ui->tableWidget->setCellWidget(i, COLUMN_APP_NAME, CreateAppNameTableItem(localApp->id));
                    ui->tableWidget->setCellWidget(i, COLUMN_APP_INS, CreateAppInstalledTableItem(localApp->GetVersion(0)->id));

                    states.push_back(ButtonsWidget::BUTTONS_STATE_INSTALLED);
                }
            }
        }
    }
    else if(localConfig)
    {
        Branch * localBranch = localConfig->GetBranch(branchID);
        if(localBranch)
        {
            int appCount = localBranch->GetAppCount();
            ui->tableWidget->setRowCount(appCount);
            for(int i = 0; i < appCount; ++i)
            {
                Application * localApp = localBranch->GetApplication(i);
                ui->tableWidget->setCellWidget(i, COLUMN_APP_NAME, CreateAppNameTableItem(localApp->id));
                ui->tableWidget->setCellWidget(i, COLUMN_APP_INS, CreateAppInstalledTableItem(localApp->GetVersion(0)->id));

                states.push_back(ButtonsWidget::BUTTONS_STATE_INSTALLED);
            }
        }
    }
    else
        return;

    for(int i = 0; i < ui->tableWidget->rowCount(); ++i)
    {
        ButtonsWidget * butts = new ButtonsWidget(i, this);
        butts->SetButtonsState(states[i]);
        ui->tableWidget->setCellWidget(i, COLUMN_BUTTONS, butts);
    }

    ui->tableWidget->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
    QHeaderView * hHeader = ui->tableWidget->horizontalHeader();
    hHeader->setResizeMode(COLUMN_APP_NAME, QHeaderView::ResizeToContents);
    hHeader->setResizeMode(COLUMN_APP_INS, QHeaderView::Stretch);
    hHeader->setResizeMode(COLUMN_APP_AVAL, QHeaderView::Stretch);
    hHeader->setResizeMode(COLUMN_BUTTONS, QHeaderView::ResizeToContents);
}

void MainWindow::ShowUpdateDialog(QQueue<UpdateTask> & tasks)
{
    if(!tasks.isEmpty())
    {
        //self-update
        if(tasks.front().isSelfUpdate)
        {
            SelfUpdater updater(tasks.front().version.url);
            updater.setWindowModality(Qt::ApplicationModal);
            updater.exec();
            if(updater.result() != QDialog::Rejected)
                return;

            tasks.dequeue();
        }
        if(!tasks.isEmpty())
        {
            //application update
            UpdateDialog dialog(tasks, this);
            connect(&dialog, SIGNAL(AppInstalled(QString, QString, AppVersion)),
                        appManager, SLOT(OnAppInstalled(QString,QString,AppVersion)));
            dialog.setWindowModality(Qt::ApplicationModal);
            dialog.exec();
        }
    }
}

void MainWindow::RefreshBranchesList()
{
    ConfigParser * localConfig = appManager->GetLocalConfig();
    ConfigParser * remoteConfig = appManager->GetRemoteConfig();

    ui->listWidget->clear();

    if(!localConfig->GetWebpageURL().isEmpty())
    {
        ui->listWidget->addItem(CreateListItem(CONFIG_LAUNCHER_WEBPAGE_KEY, LIST_ITEM_NEWS));
        ui->listWidget->addItem(CreateSeparatorItem());
    }

    QVector<QString> favs;
    QSet<QString> branchIDs;
    if(localConfig)
    {
        localConfig->MergeBranchesIDs(branchIDs);
        favs = localConfig->GetFavorites();
    }
    if(remoteConfig)
    {
        remoteConfig->MergeBranchesIDs(branchIDs);
        favs = remoteConfig->GetFavorites();
    }

    QList<QString> branchesList = branchIDs.toList();
    qSort(branchesList);

    int branchesCount = branchesList.size();

    //Add favorites branches
    if(favs.size())
    {
        bool hasFavorite = false;
        for(int i = 0; i < branchesCount; i++)
        {
            const QString & branchID = branchesList[i];
            if(favs.contains(branchID))
            {
                ui->listWidget->addItem(CreateListItem(branchID, LIST_ITEM_FAVORITES));
                hasFavorite = true;
            }
        }

        if(hasFavorite)
            ui->listWidget->addItem(CreateSeparatorItem());
    }

    //Add Others
    for(int i = 0; i < branchesCount; i++)
    {
        const QString & branchID = branchesList[i];
        if(!favs.contains(branchID))
            ui->listWidget->addItem(CreateListItem(branchID, LIST_ITEM_BRANCH));
    }
}

QListWidgetItem * MainWindow::CreateListItem(const QString &stringID, ListItemType type)
{
    QListWidgetItem * item = new QListWidgetItem(appManager->GetString(stringID));
    item->setSizeHint(QSize(-1, 34));
    if(type == LIST_ITEM_FAVORITES)
        item->setFont(listFontFav);
    else
        item->setFont(listFont);
    if(type == LIST_ITEM_BRANCH)
        item->setTextColor(QColor(100, 100, 100));
    item->setData(DAVA_WIDGET_ROLE, stringID);
    return item;
}

QListWidgetItem * MainWindow::CreateSeparatorItem()
{
    QListWidgetItem * item = new QListWidgetItem();
    item->setFlags(Qt::NoItemFlags);
    item->setBackground(QBrush(QColor(180, 180, 180), Qt::HorPattern));
    item->setSizeHint(QSize(0, 7));
    return item;
}

QWidget * MainWindow::CreateAppNameTableItem(const QString & stringID)
{
    QLabel * item = new QLabel(appManager->GetString(stringID));
    item->setProperty(DAVA_CUSTOM_PROPERTY_NAME, stringID);
    item->setFont(tableFont);

    return item;
}

QWidget * MainWindow::CreateAppInstalledTableItem(const QString & stringID)
{
    QLabel * item = new QLabel(appManager->GetString(stringID));
    item->setFont(tableFont);
    item->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    item->setContextMenuPolicy(Qt::CustomContextMenu);
    item->setProperty(DAVA_CUSTOM_PROPERTY_NAME, stringID);
    connect(item, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(OnCellClicked(QPoint)));

    return item;
}

QWidget * MainWindow::CreateAppAvalibleTableItem(Application * app)
{
    int versCount = app->GetVerionsCount();
    if(versCount == 1)
    {
        return CreateAppInstalledTableItem(app->GetVersion(0)->id);
    }
    else
    {
        QComboBox * comboBox = new QComboBox();
        for(int j = 0; j < versCount; ++j)
        {
            comboBox->addItem(app->GetVersion(j)->id);
        }
        comboBox->view()->setTextElideMode(Qt::ElideLeft);
        comboBox->setFont(tableFont);
        comboBox->setFocusPolicy(Qt::NoFocus);
        comboBox->model()->sort(0, Qt::DescendingOrder);
        comboBox->setCurrentIndex(0);

        return comboBox;
    }
}

void MainWindow::GetTableApplicationIDs(int rowNumber, QString & appID,
                                   QString & installedVersionID, QString & avalibleVersionID)
{
    QWidget * cell = 0;

    cell = ui->tableWidget->cellWidget(rowNumber, COLUMN_APP_NAME);
    if(cell)
        appID = cell->property(DAVA_CUSTOM_PROPERTY_NAME).toString();

    cell = ui->tableWidget->cellWidget(rowNumber, COLUMN_APP_INS);
    if(cell)
        installedVersionID = cell->property(DAVA_CUSTOM_PROPERTY_NAME).toString();

    cell = ui->tableWidget->cellWidget(rowNumber, COLUMN_APP_AVAL);
    QComboBox * cBox = dynamic_cast<QComboBox*>(cell);
    if(cBox)
        avalibleVersionID = cBox->currentText();
    else if(cell)
        avalibleVersionID = cell->property(DAVA_CUSTOM_PROPERTY_NAME).toString();
}

void MainWindow::UpdateButtonsState(int rowNumber, ButtonsWidget::ButtonsState state)
{
    ButtonsWidget * buttons = dynamic_cast<ButtonsWidget *>(ui->tableWidget->cellWidget(rowNumber, COLUMN_BUTTONS));
    if(buttons)
        buttons->SetButtonsState(state);
}

void MainWindow::UpdateURLValue()
{
    ui->labelRemoteURL->setText(appManager->GetLocalConfig()->GetRemoteConfigURL());
}

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QColorDialog>
#include <QTextBlock>
#include <QTextList>
#include <QTextCursor>
#include <QTextEdit>
#include <QLineEdit>
#include <QTextListFormat>
#include <QTextBlockFormat>
#include <QTextDocumentFragment>
#include <QTextBlockFormat>
#include <QVector>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->noteTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    this->is_operated = false;
    this->modelSelected = 1;
    this->model = new QStandardItemModel();
    this->model->setHorizontalHeaderLabels(QStringList() << "笔记名称" << "标签");
    ui->noteTree->setColumnWidth(0, COL_WIDTH);
    ui->noteTree->setModel(this->model);
    this->setWindowTitle("笔记阅读器");
    this->set_font_menu();
    this->set_size_menu();
    this->set_lineHeight_menu();
    QFontMetrics metrics(ui->noteEdit->font());
    int charWidth = metrics.averageCharWidth();
    int tabWidth = charWidth * 2;  // 两个字符宽
    ui->noteEdit->setTabStopDistance(tabWidth);  // Qt 5.10及以上版本
    ui->noteTree->setContextMenuPolicy(Qt::CustomContextMenu);
    threadPool = new QThreadPool(this);
    threadPool->setMaxThreadCount(QThread::idealThreadCount()); // 创建线程池
    this->system = new SocketSystem("E:/works/", "huital");
    system->init_socket("huital", "http://oss-cn-hangzhou.aliyuncs.com", "LTAI5t6p5J4LEwBY3xLabKTK", "jWW614zJCeUMA2NQ5ItyH3Z5mIo2vm", "");
    this->worker_thread = new WorkThread("E:/works/", "huital", this);
    this->worker_thread->system->init_socket("huital", "http://oss-cn-hangzhou.aliyuncs.com", "LTAI5t6p5J4LEwBY3xLabKTK", "jWW614zJCeUMA2NQ5ItyH3Z5mIo2vm", "");
    connect(ui->noteTree, &QWidget::customContextMenuRequested, this, &MainWindow::showTreeContextMenu);
    connect(ui->noteTree, &QTreeView::doubleClicked, this, &MainWindow::onNoteTreeDoubleClicked);
    connect(ui->searchLine, &QLineEdit::returnPressed, this, &MainWindow::performSearch);
    connect(ui->searchBox, QOverload<int>::of(&QComboBox::activated), this, &MainWindow::handleSearchResult);

}

void MainWindow::showTreeContextMenu(const QPoint &pos)
{
    // 获取视图中点击位置的模型索引
    QModelIndex index = ui->noteTree->indexAt(pos);
    // 从模型索引获取 QStandardItem
    QStandardItem *clickedItem = static_cast<QStandardItemModel*>(ui->noteTree->model())->itemFromIndex(index);
    QMenu menu;
    if (clickedItem != nullptr) {
        if (clickedItem->text().endsWith('/')) {
            menu.addAction(ui->action);
            menu.addAction(ui->actionDeleteGroup);
            menu.addAction(ui->actionRenameGroup);
            menu.addAction(ui->actionCreateNote);
        } else {
            if (clickedItem->column() == 0) {
                menu.addAction(ui->actionOpenNote);
                menu.addAction(ui->actionRenameNote);
                menu.addAction(ui->actionDeleteNote);
            } else {
                menu.addAction(ui->actionCreateLabel);
                menu.addAction(ui->actionAddLabel);
                menu.addAction(ui->actionDeleteLabel);
                menu.addAction(ui->actionRenameLabel);
            }
        }
    }
    // 创建菜单并添加动作

    // 获取触发事件的坐标转换为全局坐标
    QPoint globalPos = ui->noteTree->viewport()->mapToGlobal(pos);

    // 显示菜单
    menu.exec(globalPos);
}

MainWindow::~MainWindow()
{
    threadPool->waitForDone();
    delete system;
    delete ui;
}

int MainWindow::init_system()
{

    this->model->clear();
    this->model->setHorizontalHeaderLabels(QStringList() << "笔记名称" << "标签");
//    this->current_banks = this->system->get_content_from_group("", false, true);
//    this->current_labels = this->system->get_current_labels();
    this->current_item = nullptr;
    ui->noteEdit->clear();
    ui->noteTree->setColumnWidth(0, COL_WIDTH);
    this->dataProcessed = false;
    this->labelsTaskFinished = false;
    this->banksTaskFinished = false;
//    this->worker_thread->system->i
    return 0;
}

void MainWindow::create_banks(int selected_model)
{
    if (selected_model == 1) {
        qDebug() << "选择本地模式";
    } else if (selected_model == 2) {
        qDebug() << "选择数据库模式";
        database_init(this->data_base);
    } else if (selected_model == 3) {
        qDebug() << "选择数据库模式";
    }
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    // 调用父类的事件处理函数，以确保其他事件仍然可以被处理
    QMainWindow::mousePressEvent(event);

    // 判断是否是鼠标左键按下事件
    if (event->button() == Qt::LeftButton) {
        // 获取鼠标点击的全局坐标
        QPoint globalPos = event->globalPos();

        // 判断鼠标点击的位置是否在树视图区域内
        if (!ui->noteTree->geometry().contains(ui->noteTree->mapFromGlobal(globalPos))) {
            // 如果鼠标点击的位置不在树视图区域内，清除树视图的当前索引
            ui->noteTree->setCurrentIndex(QModelIndex());
        }
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!this->is_operated)
        return;
   if (QMessageBox::question(this, "确认", "您真的要退出吗？",
                             QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
       system->clear_system();
       event->accept();  // 接受关闭事件

   } else {
       event->ignore();  // 忽略关闭事件，不关闭窗口
   }
}

void MainWindow::keyPressEvent(QKeyEvent *e)
{
    QTextCursor cursor = ui->noteEdit->textCursor();
    if (e->key() == Qt::Key_Tab && !e->modifiers()) {
        // 处理 Tab 键，增加缩进
        cursor.insertText("  ");  // 插入两个宽度的空格作为缩进
        adjustIndentation(cursor, 1);

    } else if (e->key() == Qt::Key_Backtab || (e->key() == Qt::Key_Tab && e->modifiers() & Qt::ShiftModifier)) {
        // 处理 Shift+Tab 组合键，减少缩进
        removeLeadingSpaces(cursor);
        adjustIndentation(cursor, -1);
    }
}

void MainWindow::handleTabKey()
{

}

QStandardItem *MainWindow::get_selected_item(bool flag)
{
    // 获取当前选中项的索引
    QModelIndex currentIndex = ui->noteTree->currentIndex();

    // 检查是否有项被选中
    if (flag&&!currentIndex.isValid()) {
        QMessageBox::information(this, tr("提示"), tr("请先选中一个项！"));
        return nullptr;
    }

    // 获取模型和当前项
    if (!this->model) {
        QMessageBox::critical(this, tr("错误"), tr("模型错误，无法处理请求。"));
        return nullptr;
    }

    QStandardItem *item = this->model->itemFromIndex(currentIndex);
    return item;
}

void MainWindow::error_show(int ret)
{
    QMessageBox msgBox;

    // 创建QFont对象并设置字体大小
    QFont font;
    font.setPointSize(15);  // 设置字体大小

    // 设置QMessageBox的字体
    msgBox.setFont(font);

    // 根据ret的值设置不同的提示信息
    switch (ret) {
        case SUCCESS:
            msgBox.setWindowTitle("Success");
            msgBox.setText("操作成功");
            msgBox.setIcon(QMessageBox::Information);
            break;
        case NAME_ERROR:
            msgBox.setWindowTitle("Error");
            msgBox.setText("重名错误");
            msgBox.setIcon(QMessageBox::Critical);
            break;
        case NAME_INVALID:
            msgBox.setWindowTitle("Error");
            msgBox.setText("名称无效");
            msgBox.setIcon(QMessageBox::Critical);
            break;
        case DIR_ERROR:
            msgBox.setWindowTitle("Error");
            msgBox.setText("创建目录失败");
            msgBox.setIcon(QMessageBox::Critical);
            break;
        case SQL_OPEN_ERROR:
            msgBox.setWindowTitle("Error");
            msgBox.setText("数据库打开失败");
            msgBox.setIcon(QMessageBox::Critical);
            break;
        case SQL_EXEC_ERROR:
            msgBox.setWindowTitle("Error");
            msgBox.setText("数据库指令执行失败");
            msgBox.setIcon(QMessageBox::Critical);
            break;
        case UPLOAD_ERROR:
            msgBox.setWindowTitle("Error");
            msgBox.setText("服务器上传文件失败");
            msgBox.setIcon(QMessageBox::Critical);
            break;
        case DOWNLOAD_ERROR:
            msgBox.setWindowTitle("Error");
            msgBox.setText("服务器下载文件失败");
            msgBox.setIcon(QMessageBox::Critical);
            break;
        case FILE_ERROR:
            msgBox.setWindowTitle("Error");
            msgBox.setText("创建文件失败");
            msgBox.setIcon(QMessageBox::Critical);
            break;
        default:
            msgBox.setWindowTitle("Unknown Error");
            msgBox.setText("未知错误");
            msgBox.setIcon(QMessageBox::Critical);
            break;
    }

    // 显示消息框
    msgBox.exec();
}

void MainWindow::switch_item(QStandardItem *item)
{
    this->current_item = item;
}

void MainWindow::set_font_menu()
{
    // 创建一个 QMenu 对象
    QMenu *fontMenu = new QMenu(this);

    // 定义一些常用的中英文字体
    QStringList fonts = {
        "宋体",             // 宋体，中国文档中常用
        "黑体",             // 黑体，常用于中文强调文字
        "楷体",             // 楷体，常用于中国书法
        "仿宋",             // 仿宋，通常用于中国的印刷材料
        "微软雅黑",         // 微软雅黑，流行于中国的网页设计和用户界面
        "Arial",            // Arial，广泛使用的无衬线字体
        "Times New Roman",  // Times New Roman，正式文件中常用
        "Verdana",          // Verdana，优秀的网页可读性字体
        "Helvetica",        // Helvetica，印刷和数字媒体中很流行
        "Courier New",      // Courier New，标准等宽字体
        "Georgia",          // Georgia，常用于网页的衬线字体，易于阅读
        "Garamond",         // Garamond，印刷中使用的老式衬线字体
        "Comic Sans MS",    // Comic Sans MS，休闲的书写体字体
        "Trebuchet MS",     // Trebuchet MS，人文主义无衬线字体
        "Impact"            // Impact，以其醒目的外观著称的无衬线字体
    };

    // 遍历字体列表，为每个字体创建一个 QAction 并添加到菜单中
    foreach (const QString &fontName, fonts) {
        QAction *fontAction = new QAction(fontName, this);
        fontAction->setData(fontName);  // 将字体名称存储在动作的数据属性中
        fontMenu->addAction(fontAction);

        // 连接信号和槽，当选择一个字体时调用 setFontFamily
        connect(fontAction, &QAction::triggered, this, [this, fontName]() {
            QTextCursor cursor = ui->noteEdit->textCursor();
            if (!cursor.hasSelection()) {
                // 如果没有选中文本，则设置后续输入的字体
                QTextCharFormat format;
                format.setFontFamily(fontName);
                ui->noteEdit->setCurrentCharFormat(format);
                ui->tbnFont->setText(fontName);
            } else {
                // 修改选中文本的字体
                QTextCharFormat format;
                format.setFontFamily(fontName);
                cursor.mergeCharFormat(format);
                ui->noteEdit->mergeCurrentCharFormat(format);
                ui->tbnFont->setText(fontName);
            }
        });
    }

    // 将菜单设置到工具按钮上
    ui->tbnFont->setMenu(fontMenu);
    ui->tbnFont->setPopupMode(QToolButton::InstantPopup);
}

void MainWindow::set_size_menu()
{
    // 创建一个 QMenu 对象
    QMenu *fontSizeMenu = new QMenu(this);

    // 定义一些常用的字体大小
    QList<int> fontSizes = {12, 13, 14, 15, 16, 19, 22, 24, 29, 32, 40, 48};

    // 遍历字体大小列表，为每个大小创建一个 QAction 并添加到菜单中
    for (int fontSize : fontSizes) {
        QAction *fontSizeAction = new QAction(QString::number(fontSize), this);
        fontSizeAction->setData(fontSize);  // 将字体大小存储在动作的数据属性中
        fontSizeMenu->addAction(fontSizeAction);

        // 连接信号和槽，当选择一个字体大小时调用 setFontSize
        connect(fontSizeAction, &QAction::triggered, this, [this, fontSize]() {
            QTextCursor cursor = ui->noteEdit->textCursor();
            if (!cursor.hasSelection()) {
                // 如果没有选中文本，则设置后续输入的字体大小
                QTextCharFormat format;
                format.setFontPointSize(fontSize);
                ui->noteEdit->setCurrentCharFormat(format);
                ui->tbnFont->setText(QString::number(fontSize));
            } else {
                // 修改选中文本的字体大小
                QTextCharFormat format;
                format.setFontPointSize(fontSize);
                cursor.mergeCharFormat(format);
                ui->noteEdit->mergeCurrentCharFormat(format);
                ui->tbnSize->setText(QString::number(fontSize));
            }
        });
    }

    // 将菜单设置到工具按钮上
    ui->tbnSize->setMenu(fontSizeMenu);
    ui->tbnSize->setPopupMode(QToolButton::InstantPopup);
}

void MainWindow::set_lineHeight_menu()
{
    // 创建一个 QMenu 对象
    QMenu *lineHeightMenu = new QMenu(this);

    // 定义一些常用的行高倍数
    QList<double> lineHeights = {1.0, 1.15, 1.5, 2.0, 2.5, 3.0};

    // 遍历行高倍数列表，为每个行高创建一个 QAction 并添加到菜单中
    for (double lineHeight : lineHeights) {
        QAction *lineHeightAction = new QAction(QString::number(lineHeight, 'f', 2), this);
        lineHeightAction->setData(lineHeight);  // 将行高倍数存储在动作的数据属性中
        lineHeightMenu->addAction(lineHeightAction);

        // 连接信号和槽，当选择一个行高时调用 setLineHeight
        connect(lineHeightAction, &QAction::triggered, this, [this, lineHeight]() {
            QTextCursor cursor = ui->noteEdit->textCursor();
            if (cursor.hasSelection()) {
                QTextBlockFormat blockFormat = cursor.blockFormat();
                blockFormat.setLineHeight(lineHeight * 100, QTextBlockFormat::ProportionalHeight);
                cursor.setBlockFormat(blockFormat);
            }
        });
    }

    // 将菜单设置到工具按钮上
    ui->tbnLineHeight->setMenu(lineHeightMenu);
    ui->tbnLineHeight->setPopupMode(QToolButton::InstantPopup);
}

void MainWindow::adjustIndentation(QTextCursor &cursor, int adjust)
{
    QTextBlockFormat blockFormat = cursor.blockFormat();
    int indentLevel = blockFormat.indent();
    indentLevel += adjust;
    if (indentLevel < 0) indentLevel = 0;  // 防止缩进级别为负数
    blockFormat.setIndent(indentLevel);
    cursor.setBlockFormat(blockFormat);
}

void MainWindow::removeLeadingSpaces(QTextCursor &cursor)
{
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 2);
    if (cursor.selectedText() == "  ") {
        cursor.removeSelectedText();
    }
}

void MainWindow::initializeProcessing(const QString &functionName, QString name, QStandardItem* item)
{
    emit worker_thread->process(functionName, name, item);
}

void MainWindow::runForDuration(qint64 duration)
{
    QElapsedTimer timer;
    timer.start();

    while (timer.elapsed() < duration) {
        // 这里可以放一些测试代码，例如进行计算或者处理数据
        for (int i = 0; i < 1000000; ++i) {
            // 模拟一些工作负载
            volatile int dummy = i * i;
        }
    }
}

void MainWindow::open_bank_aggregator()
{

}


void MainWindow::on_actionModelSelect_triggered()
{
    QStringList options;
    options << tr("本地模式") << tr("数据库模式") << tr("服务器模式");
    QString selectedOption = QInputDialog::getItem(this, tr("选择模式"), tr("请选择模式:"), options, 0, false);

    // 判别用户是否选择了其中的某一项
    int modelSelected2 = 0; // 默认为无效输入，转换成0
    if (selectedOption == tr("本地模式"))
        modelSelected2 = 1;
    else if (selectedOption == tr("数据库模式"))
        modelSelected2 = 2;
    else if (selectedOption == tr("服务器模式")) {
        modelSelected2 = 3;
//        init_system();
        // 执行预定函数
    }

    // 将结果修改到this->modelSelected中
    this->modelSelected = modelSelected2;
    qDebug() << "modelSelected:" << this->modelSelected;
}

void MainWindow::on_actionCreateBank_triggered()
{
    QString bankName;
    if (!modelSelected) {
       QMessageBox::warning(this, tr("错误"), tr("您尚未选择模式"));
       return;
    }
    // 弹出一个输入框让用户输入笔记库名称
    bankName = QInputDialog::getText(this, tr("创建笔记库"), tr("请输入笔记库名称:"));

    // 检查用户是否点击了取消
    if (bankName.isEmpty()) {
        return; // 用户取消输入，直接返回
    }

    // 在用户输入的名称后面补上一个/作为后缀

    current_name = bankName;
    current_item = nullptr;
    labelsTaskFinished = true;


//    auto createBanksTaskFunction = [this, bankName]() {
//        return system->create_bank(bankName, nullptr);
//    };
//    NetworkTaskForInt *createBanksTask = new NetworkTaskForInt(createBanksTaskFunction);
//    connect(createBanksTask, &NetworkTaskForInt::resultReady, this, &MainWindow::onItemCreate);
//    threadPool->start(createBanksTask);

    QString path = ""; // 根据实际情况设置路径
    bool with_prefix = false;
    bool with_suffix = true;
    qDebug() << "第一步调用线程，准备查询目前的笔记库";
    auto banksTaskFunction = [this, path, with_prefix, with_suffix]() {
        return system->get_content_from_group(path, with_prefix, with_suffix);
    };
    NetworkTaskForStringVector *banksTask = new NetworkTaskForStringVector(banksTaskFunction);
    connect(banksTask, &NetworkTaskForStringVector::resultReady, this, &MainWindow::onBanksTaskFinished);
    threadPool->start(banksTask);
    qDebug() << "开始执行第一步";


//    int ret = this->system->create_bank(bankName, nullptr);
//    if (ret == SUCCESS) {
//        this->is_operated = true;
//    }
//    error_show(ret);
//    initializeProcessing("create_bank", bankName, nullptr);
//    if (!result_code) {
//        this->is_operated = true;
//    }
}

void MainWindow::on_actionTest_triggered()
{
//    this->system->download_in_local("E:/notes/");
    this->system->test("test_thread/test1/test2/hello.js", "E:/works/test_thread/tmp/hello.js");
    this->system->test("test_thread/test1/test2/hello1_1.txt", "E:/works/test_thread/tmp/hello1_1.txt");
}

void MainWindow::on_actionCheck_triggered()
{
    // 获取 QTextEdit 控件
     QTextEdit *noteEdit = ui->noteEdit;

     // 获取用户点击位置的光标
     QTextCursor cursor = noteEdit->textCursor();

     // 获取光标所在块的文本
     QTextBlock block = cursor.block();
     QString blockText = block.text();

     // 计算索引等级
     int indexLevel = 0;
     while (blockText.startsWith('\t')) {
         ++indexLevel;
         blockText.remove(0, 1);
     }

     // 设置列表格式
     QTextListFormat listFormat;
     listFormat.setIndent(indexLevel + 1);

     switch (indexLevel % 3) {
         case 0:
             listFormat.setStyle(QTextListFormat::ListDisc); // 实心圆
             break;
         case 1:
             listFormat.setStyle(QTextListFormat::ListCircle); // 空心圆
             break;
         case 2:
             listFormat.setStyle(QTextListFormat::ListSquare); // 实心正方形
             break;
     }

     // 插入无序列表符号
     cursor.beginEditBlock();
     // 如果当前块已经是无序列表的一部分，则不需要再次插入
     QTextList *list = cursor.insertList(listFormat);
     list->add(cursor.block());
     cursor.endEditBlock();
}

void MainWindow::on_actionOpenBank_triggered()
{
    init_system();
    this->is_operated = true;
    if (this->modelSelected == 0) {
        QMessageBox::warning(this, "警告", "尚未选中加载环境！");
        return;
    }


    QString path = ""; // 根据实际情况设置路径
    bool with_prefix = false;
    bool with_suffix = true;
    qDebug() << "第一步调用线程，准备查询目前的笔记库";
    auto banksTaskFunction = [this, path, with_prefix, with_suffix]() {
        return system->get_content_from_group(path, with_prefix, with_suffix);
    };

    NetworkTaskForStringVector *banksTask = new NetworkTaskForStringVector(banksTaskFunction);
    connect(banksTask, &NetworkTaskForStringVector::resultReady, this, &MainWindow::onBanksOpenFinished, Qt::BlockingQueuedConnection);
    connect(banksTask, &NetworkTaskBase::taskFinished, banksTask, &QObject::deleteLater);
    threadPool->start(banksTask);
    qDebug() << "开始执行第一步";

//    QString directoryPath;


//    this->current_banks = this->system->get_content_from_group("", false, true);
//    qDebug() << current_labels;
//    if (this->current_banks.isEmpty()) {
//        QMessageBox::warning(this, "警告", "当前没有已创建的笔记库！");
//    } else {
//        bool ok;
//        directoryPath = QInputDialog::getItem(this, "选择笔记库", "请选择一个笔记库：", this->current_banks.toList(), 0, false, &ok);
//    }

//    // 检查用户是否取消了对话框
//    if(directoryPath.isEmpty()) {
//        QMessageBox::information(this, "提示", "未选择任何文件夹。");
//        return;
//    }

//    this->system->set_database(directoryPath);


//    int ret = this->system->open_bank(directoryPath, this->model);
//    this->current_labels = this->system->get_current_labels();
//    qDebug() << current_labels;
//    error_show(ret);
//    initializeProcessing("open_bank", directoryPath, this->model->invisibleRootItem());
//    return_code = 1;
    ui->noteTree->setColumnWidth(0, COL_WIDTH);
}

void MainWindow::on_action_triggered()
{
    if (!this->is_operated) {
        QMessageBox::warning(this, "警告", "您尚未选择一个笔记库");
        return;
    }
    // 获取当前选中的项
    QStandardItem *item = get_selected_item(false);
    // 引导用户输入合法的分组名称
    // 弹出一个输入框让用户输入笔记库名称
    QString groupName = QInputDialog::getText(this, tr("创建分组"), tr("请输入分组名称:"));

    // 检查用户是否点击了取消
    if (groupName.isEmpty()) {
        return; // 用户取消输入，直接返回
    }
    if (item == nullptr) {
        item = model->invisibleRootItem();
    }

    this->current_item = item;
    this->current_name = groupName;
    this->current_type = 1; // 表示文件夹
    auto createGroupTaskFunction = [this, groupName, item]() {
        return system->create_group(groupName, item);
    };
    NetworkTaskForInt *createGroupTask = new NetworkTaskForInt(createGroupTaskFunction);
    connect(createGroupTask, &NetworkTaskForInt::resultReady, this, &MainWindow::onItemCreate);
    connect(createGroupTask, &NetworkTaskBase::taskFinished, createGroupTask, &QObject::deleteLater);
    threadPool->start(createGroupTask);


//    initializeProcessing("create_group", groupName, item);
//    //this->system->create_group(groupName, item);
//    QMutexLocker locker(&mutex);
//    return_code = 0;
//    locker.unlock();
    ui->noteTree->setColumnWidth(0, COL_WIDTH);
}


void MainWindow::on_actionCreateNote_triggered()
{
    if (!this->is_operated) {
        QMessageBox::warning(this, "警告", "您尚未选择一个笔记库");
        return;
    }
    // 获取当前选中的项
    QStandardItem *item = get_selected_item(false);
    // 引导用户输入合法的分组名称
    // 弹出一个输入框让用户输入笔记库名称
    if (item != nullptr && !is_dir(item->text())) {
        QMessageBox::warning(this, "警告", "您需要选中一个分组!");
        return;
    }
    QString groupName = QInputDialog::getText(this, tr("创建笔记"), tr("请输入笔记名称:"));
    QString absolute_path = "";
    // 检查用户是否点击了取消
    if (groupName.isEmpty()) {
        return; // 用户取消输入，直接返回
    }
    if (item == nullptr) {
        item = model->invisibleRootItem();
    }
    current_type = 0;
    current_item = item;
    current_name = groupName;
    auto createNoteTaskFunction = [this, groupName, item]() {
        return system->create_note(groupName, item);
    };
    NetworkTaskForInt *createNoteTask = new NetworkTaskForInt(createNoteTaskFunction);
    connect(createNoteTask, &NetworkTaskForInt::resultReady, this, &MainWindow::onItemCreate);
    connect(createNoteTask, &NetworkTaskBase::taskFinished, createNoteTask, &QObject::deleteLater);
    threadPool->start(createNoteTask);

//    initializeProcessing("create_note", groupName, item);
//    QMutexLocker locker(&mutex);
//    locker.unlock();        // 如果不释放则会卡死
    ui->noteTree->setColumnWidth(0, COL_WIDTH);
//    error_show(result_code);
}

void MainWindow::on_actionCreateLabel_triggered()
{
    if (!this->is_operated) {
        QMessageBox::warning(this, "警告", "您尚未选择一个笔记库");
        return;
    }
    QString labelName = QInputDialog::getText(this, tr("创建标签"), tr("请输入标签名称:"));

    auto createLabelTaskFunction = [this, labelName]() {
        return system->create_label(labelName, nullptr);
    };

    this->current_name = labelName;
    this->current_type = 2;

    NetworkTaskForInt *createLabelTask = new NetworkTaskForInt(createLabelTaskFunction);
    connect(createLabelTask, &NetworkTaskForInt::resultReady, this, &MainWindow::onItemCreate);
    connect(createLabelTask, &NetworkTaskBase::taskFinished, createLabelTask, &QObject::deleteLater);
    threadPool->start(createLabelTask);


//    int ret = this->system->create_label(labelName, nullptr);
//    if (ret == SUCCESS) {
//        current_labels.append(labelName);
//    }
//    error_show(ret);
}

void MainWindow::on_actionSave_triggered()
{
    auto saveTaskFunction = [this]() {
        return system->save_system();
    };
    NetworkTaskForInt *saveTask = new NetworkTaskForInt(saveTaskFunction);
    connect(saveTask, &NetworkTaskForInt::resultReady, this, &MainWindow::onErrorShow);
    connect(saveTask, &NetworkTaskBase::taskFinished, saveTask, &QObject::deleteLater);
    threadPool->start(saveTask);
}

void MainWindow::on_actionAddLabel_triggered()
{
    bool ok;
    if (!this->is_operated) {
        QMessageBox::warning(this, "警告", "您尚未选择一个笔记库");
        return;
    }
    // 获取当前选中的项
    QStandardItem *item = get_selected_item(false);
    if (item == nullptr || item->text().endsWith('/')) {
        QMessageBox::warning(this, "警告", "请选中一项笔记！");
        return;
    }

    if (item->column() == 1) {
        item = item->parent()->child(item->row(), 0);
    }
    QString labelName = QInputDialog::getItem(this, "选择标签", "请选择一个标签：", this->current_labels.toList(), 0, false, &ok);
    if(labelName.isEmpty()) {
        QMessageBox::information(this, "提示", "尚未选中一个标签");
        return;
    }

    QString id = this->system->get_id(item);
    item->setData(id, Qt::UserRole + 1);
    current_item = item;
    current_name = labelName;
    current_type = 3;
    auto addLabelTaskFunction = [this, labelName, item]() {
        return system->add_label(labelName, item);
    };
    NetworkTaskForInt *addLabelTask = new NetworkTaskForInt(addLabelTaskFunction);
    connect(addLabelTask, &NetworkTaskForInt::resultReady, this, &MainWindow::onItemCreate);
    connect(addLabelTask, &NetworkTaskBase::taskFinished, addLabelTask, &QObject::deleteLater);
    threadPool->start(addLabelTask);

//    int ret = this->system->add_label(labelName, item);
//    error_show(ret);
}

void MainWindow::on_actionDeleteLabel_triggered()
{
    if (!this->is_operated) {
        QMessageBox::warning(this, "警告", "您尚未选择一个笔记库");
        return;
    }
    // 获取当前选中的项
    QStandardItem *item = get_selected_item(false);
    if (item == nullptr || item->text().endsWith('/')) {
        QMessageBox::warning(this, "警告", "请选中一项笔记！");
        return;
    }
    if (item->column() == 1) {
        item = item->parent()->child(item->row(), 0);
    }
    QString labelName = item->parent()->child(item->row(), 1)->text();
    current_item = item;
    current_name = labelName;
    auto deleteLabelTaskFunction = [this, labelName, item]() {

        return system->delete_label(labelName, item);
    };
    NetworkTaskForInt *deleteLabelTask = new NetworkTaskForInt(deleteLabelTaskFunction);
    connect(deleteLabelTask, &NetworkTaskForInt::resultReady, this, &MainWindow::onDeleteLabelItem, Qt::BlockingQueuedConnection);
    connect(deleteLabelTask, &NetworkTaskBase::taskFinished, deleteLabelTask, &QObject::deleteLater);
    threadPool->start(deleteLabelTask);
//    int ret = this->system->delete_label(labelName, item);
//    error_show(ret);
    return;
}

void MainWindow::on_actionDeleteNote_triggered()
{
    if (!this->is_operated) {
        QMessageBox::warning(this, "警告", "您尚未选择一个笔记库");
        return;
    }

    // 获取当前选中的项
    QStandardItem *item = get_selected_item(false);
    if (item == nullptr || item->text().endsWith('/')) {
        QMessageBox::warning(this, "警告", "请选中一项笔记！");
        return;
    }

    QString name = item->text();
    current_item = item;
    current_name = name;
    current_type = 5;
    auto deleteNoteTaskFunction = [this, name, item]() {

        return system->delete_note(name, item);
    };
    NetworkTaskForInt *deleteNoteTask = new NetworkTaskForInt(deleteNoteTaskFunction);
    connect(deleteNoteTask, &NetworkTaskForInt::resultReady, this, &MainWindow::onDeleteNoteItem, Qt::BlockingQueuedConnection);
    connect(deleteNoteTask, &NetworkTaskBase::taskFinished, deleteNoteTask, &QObject::deleteLater);
    threadPool->start(deleteNoteTask);


//    int ret = this->system->delete_note(item->text(), item);
//    error_show(ret);
}

void MainWindow::on_actionDeleteGroup_triggered()
{
    if (!this->is_operated) {
        QMessageBox::warning(this, "警告", "您尚未选择一个笔记库");
        return;
    }
    // 获取当前选中的项
    QStandardItem *item = get_selected_item(false);
    if (item == nullptr || !item->text().endsWith('/')) {
        QMessageBox::warning(this, "警告", "请选中一个分组！");
        return;
    }
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(nullptr, "确认删除",
                                  "确定要删除 " + item->text() + " 吗？",
                                  QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes)  return;

    QString name = item->text();
    current_item = item;
    current_name = name;
    current_type = 6;
    auto deleteGroupTaskFunction = [this, name, item]() {

        return system->delete_group(name, item);
    };
    NetworkTaskForInt *deleteGroupTask = new NetworkTaskForInt(deleteGroupTaskFunction);
    connect(deleteGroupTask, &NetworkTaskForInt::resultReady, this, &MainWindow::onDeleteNoteItem, Qt::BlockingQueuedConnection);
    connect(deleteGroupTask, &NetworkTaskBase::taskFinished, deleteGroupTask, &QObject::deleteLater);
    threadPool->start(deleteGroupTask);

//    int ret = this->system->delete_group(item->text(), item);
//    error_show(ret);

}

void MainWindow::on_actionRenameLabel_triggered()
{
    bool ok;
    if (!this->is_operated) {
        QMessageBox::warning(this, "警告", "您尚未选择一个笔记库");
        return;
    }
    // 获取当前选中的项
    QStandardItem *item = get_selected_item(false);
    if (item == nullptr || item->text().endsWith('/')) {
        QMessageBox::warning(this, "警告", "请选中一项笔记！");
        return;
    }
    if (item->column() == 1) {
        item = item->parent()->child(item->row(), 0);
    }
    QStandardItem *labelItem = item->parent()->child(item->row(), 1);
    QString labelName = QInputDialog::getItem(this, "选择标签", "请选择一个新的标签：", this->current_labels.toList(), 0, false, &ok);
    if(labelName.isEmpty()) {
        QMessageBox::information(this, "提示", "尚未选中一个标签");
        return;
    }
    if (labelName == labelItem->text()) {
        QMessageBox::warning(this, "警告", "请选中一个新的标签！");
        return;
    }
    int ret = this->system->rename_label(labelItem->text(), labelName, item);
    if (ret == SUCCESS) {
        labelItem->setText(labelName);
    }
    error_show(ret);
    return;
}

void MainWindow::on_actionRenameNote_triggered()
{
    bool ok;
    if (!this->is_operated) {
        QMessageBox::warning(this, "警告", "您尚未选择一个笔记库");
        return;
    }
    // 获取当前选中的项
    QStandardItem *item = get_selected_item(false);
    if (item == nullptr || item->text().endsWith('/')) {
        QMessageBox::warning(this, "警告", "请选中一项笔记！");
        return;
    }
    QString noteName = QInputDialog::getText(this, tr("重命名笔记"), tr("请输入新的笔记名称:"));
    int ret = this->system->rename_note(noteName, item);
    if (ret == SUCCESS) {
        item->setText(noteName);
    }
    error_show(ret);
    return;
}

void MainWindow::on_actionRenameGroup_triggered()
{
    bool ok;
    if (!this->is_operated) {
        QMessageBox::warning(this, "警告", "您尚未选择一个笔记库");
        return;
    }
    // 获取当前选中的项
    QStandardItem *item = get_selected_item(false);
    if (item == nullptr || !item->text().endsWith('/')) {
        QMessageBox::warning(this, "警告", "请选中一个分组！");
        return;
    }
    QString groupName = QInputDialog::getText(this, tr("重命名分组"), tr("请输入新的分组名称:"));
    if (this->system->exists(item, groupName, true)) {
        QMessageBox::warning(this, "警告", "您输入的分组已存在！");
        return;
    }
    groupName.append('/');
    int ret = this->system->rename_group(groupName, item);
    if (ret == SUCCESS) {
        item->setText(groupName);
    }
    error_show(ret);
}

void MainWindow::on_actionOpenNote_triggered()
{
    bool ok;
    if (!this->is_operated) {
        QMessageBox::warning(this, "警告", "您尚未选择一个笔记库");
        return;
    }
    // 获取当前选中的项
    QStandardItem *item = get_selected_item(false);
    if (item == nullptr || item->text().endsWith('/')) {
        QMessageBox::warning(this, "警告", "请选中一项笔记！");
        return;
    }
    QString pathBase = item->text();
    QString content;
    int ret = this->system->open_file(item, pathBase, content);
    if (ret == SUCCESS) {
        ui->noteEdit->setHtml(content);
        switch_item(item);
    }
    error_show(ret);
}

void MainWindow::on_actionSaveNote_triggered()
{
    bool ok;
    if (!this->is_operated) {
        QMessageBox::warning(this, "警告", "您尚未选择一个笔记库");
        return;
    }
    if (current_item == nullptr) {
        QMessageBox::warning(this, "警告", "您尚未打开过一个笔记");
        return;
    }
    QString content = ui->noteEdit->toHtml();
    int ret = this->system->save_file(current_item, content);
    error_show(ret);
}

void MainWindow::onNoteTreeDoubleClicked(const QModelIndex &index)
{
    QStandardItem *item = get_selected_item(false);
    if (item == nullptr || item->text().endsWith('/')) {
        QMessageBox::warning(this, "警告", "请选中一项笔记！");
        return;
    }
    QString pathBase = item->text();
    QString content;
    int ret = this->system->open_file(item, pathBase, content);
    if (ret == SUCCESS) {
        ui->noteEdit->setHtml(content);
        switch_item(item);
    }
}

void MainWindow::performSearch()
{
    qDebug() << "搜索开始";
    // 获取输入的搜索内容
    QString search_path = ui->searchLine->text().trimmed();

    // 调用搜索函数获取结果
    QStringList* search_result = this->system->search_model(search_path, model);

    // 检查搜索结果是否为空
    if (search_result->isEmpty()) {
        QMessageBox::information(this, "No Match", "No matching results found.");
    } else {
        // 清空搜索框中的内容
        ui->searchLine->clear();

        // 清空搜索结果框中的内容
        ui->searchBox->clear();

        // 将搜索结果添加到搜索框中
        ui->searchBox->addItems(*search_result);
    }

    // 释放搜索结果内存
    delete search_result;
}

void MainWindow::handleSearchResult(int index) {
    // 获取用户点击的搜索结果
    QString result = ui->searchBox->itemText(index);
    // 将路径按空格分割成两部分
    QStringList parts = result.split("    ");
    if (parts.size() != 2) {
        qDebug() << "Invalid path format!";
        return;
    }

    QString itemText = parts.at(1); // 获取第二部分文本
    // 调用预设方法获取目标项
    QStandardItem* targetItem = this->system->path_to_model(model, itemText);
    if (targetItem) {
        // 获取目标项的索引
        QModelIndex targetIndex = model->indexFromItem(targetItem);

        // 选中目标项
        ui->noteTree->setCurrentIndex(targetIndex);

        // 展开目标项的父项
        ui->noteTree->expand(targetIndex.parent());
    }
}

void MainWindow::on_tbnHeavy_clicked()
{
    QTextCursor cursor = ui->noteEdit->textCursor();
    if (!cursor.hasSelection()) {
        // 如果没有选中文本，修改光标处的字体加粗状态
        QTextCharFormat format = ui->noteEdit->currentCharFormat();
        format.setFontWeight(format.fontWeight() == QFont::Bold ? QFont::Normal : QFont::Bold);
        ui->noteEdit->setCurrentCharFormat(format);
    } else {
        // 修改选中文本的加粗状态
        QTextCharFormat format;
        bool isBold = cursor.charFormat().fontWeight() > QFont::Normal;
        format.setFontWeight(isBold ? QFont::Normal : QFont::Bold);
        cursor.mergeCharFormat(format);
        ui->noteEdit->mergeCurrentCharFormat(format);
    }
}

void MainWindow::on_tbnUnderline_clicked()
{
    QTextCursor cursor = ui->noteEdit->textCursor();
    if (!cursor.hasSelection()) {
        // 如果没有选中文本，修改光标处的字体下划线状态
        QTextCharFormat format = ui->noteEdit->currentCharFormat();
        format.setFontUnderline(!format.fontUnderline());  // 切换下划线状态
        ui->noteEdit->setCurrentCharFormat(format);
    } else {
        // 修改选中文本的下划线状态
        QTextCharFormat format;
        format.setFontUnderline(!cursor.charFormat().fontUnderline());  // 切换下划线状态
        cursor.mergeCharFormat(format);
        ui->noteEdit->mergeCurrentCharFormat(format);
    }
}

void MainWindow::on_tbnColor_clicked()
{
    // 创建并打开颜色选择对话框，初始颜色设置为黑色
    QColor color = QColorDialog::getColor(Qt::black, this, "选择颜色");

    // 检查用户是否选择了颜色（点击取消则返回无效颜色）
    if (color.isValid()) {
        QTextCursor cursor = ui->noteEdit->textCursor();
        QTextCharFormat format;
        format.setForeground(color);

        if (!cursor.hasSelection()) {
            // 如果没有选中任何文本，则设置后续输入的文本颜色
            ui->noteEdit->setCurrentCharFormat(format);
        } else {
            // 修改选中文本的颜色
            cursor.mergeCharFormat(format);
            ui->noteEdit->mergeCurrentCharFormat(format);
        }
        QPalette pal = ui->tbnColor->palette();
        pal.setColor(QPalette::ButtonText, color);
        ui->tbnColor->setPalette(pal);
    }
}

void MainWindow::on_tbnList_clicked()
{
    QTextEdit* textEdit = ui->noteEdit;
    QTextCursor cursor = textEdit->textCursor();

    if (!cursor.hasSelection()) {
        return;
    }

    int start = cursor.selectionStart();
    int end = cursor.selectionEnd();
    const QStringList bulletSymbols = {"*", "-", "+", ">", "#", "%", "&"}; // 7个级别的符号

    cursor.setPosition(start);
    int previousPosition = -1;

    do {
        QTextBlock block = cursor.block();
        if (block.position() > end || !block.isValid()) {
            break;
        }

        QString text = block.text();
        int tabCount = 0;
        while (tabCount < text.length() && text[tabCount] == '\t') {
            tabCount++;
        }
        int level = qMin(tabCount, 7); // 制表符数量限制在7以内
        QString symbol = bulletSymbols[level % bulletSymbols.size()];
        QString symbolWithSpace = symbol + " "; // 符号加一个空格，对应添加时的格式

        // 定位到制表符后的第一个字符位置
        cursor.setPosition(block.position() + tabCount);
        cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, symbolWithSpace.length());

        if (cursor.selectedText() == symbolWithSpace) {
            // 如果找到符号和空格，移除它们
            cursor.removeSelectedText();
        } else {
            // 如果符号不匹配或不存在，添加符号
            cursor.setPosition(block.position() + tabCount); // 定位到最后一个制表符后
            cursor.insertText(symbolWithSpace); // 插入符号和空格
        }

        previousPosition = cursor.position();
        cursor.movePosition(QTextCursor::NextBlock);

        if (cursor.position() == previousPosition) {
            break; // 如果位置未改变，说明已到达文档末尾或无更多块
        }
    } while (cursor.position() <= end);
}

void MainWindow::on_ptnList_clicked()
{
    QTextCursor cursor = ui->noteEdit->textCursor();

    // 获取当前光标所在的列表，如果没有则为nullptr
    QTextList *currentList = cursor.currentList();

    // 定义列表格式
    QTextListFormat listFormat;
    if (currentList) {
        listFormat = currentList->format();
    } else {
        listFormat.setStyle(QTextListFormat::ListDecimal); // 默认为数字列表
    }

    // 获取当前缩进级别
    int indentLevel = listFormat.indent();

    // 如果已经是三级列表，则重置为无列表
    if (indentLevel == 3) {
        listFormat.setIndent(0);
        cursor.setBlockFormat(QTextBlockFormat());
        cursor.createList(QTextListFormat::ListStyleUndefined);
    } else {
        // 增加一级缩进
        listFormat.setIndent(indentLevel + 1);

        // 根据缩进级别设置样式
        switch (indentLevel + 1) {
            case 1:
                listFormat.setStyle(QTextListFormat::ListDecimal);
                break;
            case 2:
                listFormat.setStyle(QTextListFormat::ListLowerAlpha);
                break;
            case 3:
                listFormat.setStyle(QTextListFormat::ListUpperRoman);
                break;
            default:
                listFormat.setStyle(QTextListFormat::ListDecimal); // 默认设置
                break;
        }

        // 应用列表格式
        cursor.createList(listFormat);
    }
}

void MainWindow::MainWindow::onBanksTaskFinished(QVector<QString> result)
{
    current_banks = result;
    banksTaskFinished = true;
    QString name = current_name;
    QStandardItem* c = nullptr;
    qDebug() << "第二步调用线程，准备创建笔记库";
    if (banksTaskFinished && labelsTaskFinished) {
        auto createBanksTaskFunction = [this, name, c]() {
            return system->create_bank(name, c);
        };
        NetworkTaskForInt *createBanksTask = new NetworkTaskForInt(createBanksTaskFunction);
        connect(createBanksTask, &NetworkTaskForInt::resultReady, this, &MainWindow::onItemCreate);
        connect(createBanksTask, &NetworkTaskBase::taskFinished, createBanksTask, &QObject::deleteLater);
        threadPool->start(createBanksTask);
        qDebug() << "开始执行第二步";
        // 重置，等待下一次传输
        banksTaskFinished = false;
        labelsTaskFinished = false;
    }
}

void MainWindow::onBanksOpenFinished(QVector<QString> result)
{
    current_banks = result;
    banksTaskFinished = true;
    qDebug() << "第二步调用线程，准备创建笔记库";
    if (banksTaskFinished) {
        banksTaskFinished = false;
        labelsTaskFinished = true;

        QString directoryPath;
        if (this->current_banks.isEmpty()) {
            QMessageBox::warning(this, "警告", "当前没有已创建的笔记库！");
        } else {
            bool ok;
            directoryPath = QInputDialog::getItem(this, "选择笔记库", "请选择一个笔记库：", this->current_banks.toList(), 0, false, &ok);
        }
        this->system->set_database(directoryPath);
        // 检查用户是否取消了对话框
        if(directoryPath.isEmpty()) {
            QMessageBox::information(this, "提示", "未选择任何文件夹。");
            return;
        }

        QStandardItemModel* c = this->model;
        auto openBanksTaskFunction = [this, directoryPath, c]() {

            return system->open_bank(directoryPath, c);
        };
        NetworkTaskForInt *openBanksTask = new NetworkTaskForInt(openBanksTaskFunction);
        connect(openBanksTask, &NetworkTaskForInt::resultReady, this, &MainWindow::showLabel, Qt::BlockingQueuedConnection);
        connect(openBanksTask, &NetworkTaskBase::taskFinished, openBanksTask, &QObject::deleteLater);
        threadPool->start(openBanksTask);
    }
}

void MainWindow::onLabelsOpenTaskFinished(QVector<QString> result)
{
    current_labels = result;
    qDebug() << "current_labels:" << current_labels;
    labelsTaskFinished = true;
    qDebug() << "第二步调用线程，准备创建笔记库";
    if (labelsTaskFinished) {
        banksTaskFinished = false;
        labelsTaskFinished = false;
        QString directoryPath;
        if (this->current_banks.isEmpty()) {
            QMessageBox::warning(this, "警告", "当前没有已创建的笔记库！");
        } else {
            bool ok;
            directoryPath = QInputDialog::getItem(this, "选择笔记库", "请选择一个笔记库：", this->current_banks.toList(), 0, false, &ok);
        }

        // 检查用户是否取消了对话框
        if(directoryPath.isEmpty()) {
            QMessageBox::information(this, "提示", "未选择任何文件夹。");
            return;
        }

        QStandardItemModel* c = this->model;
        auto openBanksTaskFunction = [this, directoryPath, c]() {
            QMutexLocker locker(&mutex);
            return system->open_bank(directoryPath, c);
        };
        NetworkTaskForInt *openBanksTask = new NetworkTaskForInt(openBanksTaskFunction);
        connect(openBanksTask, &NetworkTaskForInt::resultReady, this, &MainWindow::onErrorShow);
        connect(openBanksTask, &NetworkTaskBase::taskFinished, openBanksTask, &QObject::deleteLater);
        threadPool->start(openBanksTask);
    }
}

void MainWindow::onErrorShow(int result)
{
    error_show(result);
}

void MainWindow::showLabel(int result)
{
    QMutexLocker locker(&mutex);
    if (result == SUCCESS) {
        this->current_labels = this->system->get_current_labels();
        this->system->buildLabel(this->model);
        error_show(result);
    }
}

void MainWindow::onItemCreate(int result)
{
    if (result == SUCCESS && current_item != nullptr) {
        if (current_type == 1)
            current_name.append("/");
        if (current_type == 1 || current_type == 0) {
            QStandardItem *nameItem = new QStandardItem(current_name);
            QStandardItem *labelItem = new QStandardItem("");
            this->current_item->appendRow(QList<QStandardItem*>() << nameItem << labelItem);
            if (current_type == 0)
                nameItem->setForeground(QBrush(Qt::blue));
        }
    }

    if (current_type == 2) {
        current_labels.append(current_name);
        qDebug() << "after create:" << current_labels;
    }

    if (current_type == 3 && result == SUCCESS) {
        QStandardItem* labelitem = current_item->parent()->child(current_item->row(), 1);
        labelitem->setText(current_name);
    }
    current_item = nullptr;
    current_name = "";
    current_type = -1;
    error_show(result);
}

void MainWindow::onDeleteLabelItem(int result)
{
    QStandardItem *labelItem = current_item->parent()->child(current_item->row(), 1);
    labelItem->setText("");
    error_show(result);
}

void MainWindow::onDeleteNoteItem(int result)
{
    if (result == SUCCESS) {
        model->removeRow(current_item->row(), current_item->index().parent());
    }

    error_show(result);
}


void MainWindow::on_actionDownload_triggered()
{
    QVector<QString> options = this->system->get_port();
    QStringList items;
    for (const QString& option : options) {
        items << option;
    }

    bool ok;
    QString selectedItem = QInputDialog::getItem(this, "Select an option", "Options:", items, 0, false, &ok);

    if (ok && !selectedItem.isEmpty()) {
        this->port = this->system->initializeSerialPort(selectedItem);

        connect(port, &QSerialPort::readyRead, this, &MainWindow::handleReadyRead);
        connect(this, &MainWindow::download_files, this, &MainWindow::on_download_files);
        this->system->sendCommand(port, "开始进行下载");
        QThread::sleep(2);
        emit this->download_files("/测试2/测试2_1.txt", 1, "你可真好啊");
//        if (this->system->sendCommand(port, "开始进行下载")) {
//            QMessageBox::information(this, "成功", "接收成功!");
//        } else {
//            QMessageBox::critical(this, "失败", "串口发送命令失败!");
//        }
        // 在这里处理用户选择的选项
    } else {
        qDebug() << "User canceled the selection or did not select any option.";
    }
}

void MainWindow::on_download_files(QString path, int is_dir, QString content)
{
    QString result = QString("%1#%2#%3").arg(path).arg(is_dir).arg(content);
    this->system->sendCommand(port, result);
    qDebug() << "Concatenated string:" << result;
}

#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTableWidgetItem>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QFrame>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QStringList>
#include <QtGui/QFont>
#include <QtGui/QIcon>

#include <windows.h>
#include <string>
#include <vector>
#include <sstream>

class PathManager {
public:
    void loadPaths() {
        std::string userPath = getEnvironmentVariable("PATH", HKEY_CURRENT_USER);
        std::string systemPath = getEnvironmentVariable("PATH", HKEY_LOCAL_MACHINE);
        
        if (userPath == systemPath) {
            userPaths = splitPath(userPath);
            systemPaths.clear(); // don't double-count
        } else {
            userPaths = splitPath(userPath);
            systemPaths = splitPath(systemPath);
        }
    }

    const std::vector<std::string>& getUserPaths() const { return userPaths; }
    const std::vector<std::string>& getSystemPaths() const { return systemPaths; }

private:
    std::vector<std::string> userPaths;
    std::vector<std::string> systemPaths;

    std::string getEnvironmentVariable(const std::string& name, HKEY hKey) {
        HKEY key;
        LONG result = RegOpenKeyExA(hKey, 
            hKey == HKEY_CURRENT_USER ? "Environment" : "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment",
            0, KEY_READ, &key);
        
        if (result != ERROR_SUCCESS) {
            return "";
        }

        DWORD valueType;
        DWORD dataSize = 0;
        result = RegQueryValueExA(key, name.c_str(), nullptr, &valueType, nullptr, &dataSize);
        
        if (result != ERROR_SUCCESS || dataSize == 0) {
            RegCloseKey(key);
            return "";
        }

        std::string value(dataSize, '\0');
        result = RegQueryValueExA(key, name.c_str(), nullptr, &valueType, 
                                  reinterpret_cast<LPBYTE>(&value[0]), &dataSize);
        
        RegCloseKey(key);
        
        if (result == ERROR_SUCCESS) {
            // Remove null terminator if present
            if (!value.empty() && value.back() == '\0') {
                value.pop_back();
            }
            return value;
        }
        
        return "";
    }

    std::vector<std::string> splitPath(const std::string &path) {
        std::vector<std::string> parts;
        std::stringstream iss(path);
        std::string token;

        while (std::getline(iss, token, ';')) {
            if (!token.empty()) {
                // Expand environment variables
                std::string expanded = expandEnvironmentStrings(token);
                parts.push_back(expanded);
            }
        }
        return parts;
    }

    std::string expandEnvironmentStrings(const std::string &str) {
        char buffer[32768];
        DWORD result = ExpandEnvironmentStringsA(str.c_str(), buffer, sizeof(buffer));
        if (result > 0 && result <= sizeof(buffer)) {
            return std::string(buffer);
        }
        return str;
    }
};

class EnvironmentViewer : public QMainWindow
{
    Q_OBJECT

public:
    EnvironmentViewer(QWidget *parent = nullptr)
        : QMainWindow(parent), pathManager(new PathManager())
    {
        setupUI();
        loadEnvironmentVariables();
        loadPathVariables();
        applyDarkTheme();
    }

    ~EnvironmentViewer() {
        delete pathManager;
    }

private slots:
    void filterVariables()
    {
        QString filterText = searchBox->text().toLower();
        
        for (int row = 0; row < envTable->rowCount(); ++row) {
            QTableWidgetItem* nameItem = envTable->item(row, 0);
            QTableWidgetItem* valueItem = envTable->item(row, 1);
            
            bool visible = filterText.isEmpty() ||
                          (nameItem && nameItem->text().toLower().contains(filterText)) ||
                          (valueItem && valueItem->text().toLower().contains(filterText));
            
            envTable->setRowHidden(row, !visible);
        }
        
        updateStatusLabel();
    }

    void filterPaths()
    {
        QString filterText = pathSearchBox->text().toLower();
        
        for (int row = 0; row < pathTable->rowCount(); ++row) {
            QTableWidgetItem* pathItem = pathTable->item(row, 1);
            QTableWidgetItem* typeItem = pathTable->item(row, 0);
            
            bool visible = filterText.isEmpty() ||
                          (pathItem && pathItem->text().toLower().contains(filterText)) ||
                          (typeItem && typeItem->text().toLower().contains(filterText));
            
            pathTable->setRowHidden(row, !visible);
        }
        
        updatePathStatusLabel();
    }
    
    void refreshVariables()
    {
        loadEnvironmentVariables();
        loadPathVariables();
        filterVariables();
        filterPaths();
    }
    
    void onEnvItemSelectionChanged()
    {
        auto selectedItems = envTable->selectedItems();
        if (!selectedItems.isEmpty()) {
            int row = selectedItems.first()->row();
            QTableWidgetItem* nameItem = envTable->item(row, 0);
            QTableWidgetItem* valueItem = envTable->item(row, 1);
            
            if (nameItem && valueItem) {
                QString detailText = QString(
                    "<h3 style='color: #64B5F6; margin-bottom: 10px;'>%1</h3>"
                    "<div style='background: #2E2E2E; padding: 12px; border-radius: 6px; "
                    "border-left: 3px solid #64B5F6; font-family: Consolas, monospace;'>"
                    "<span style='color: #E0E0E0; line-height: 1.4;'>%2</span>"
                    "</div>"
                ).arg(nameItem->text()).arg(valueItem->text().replace(";", ";<br/>"));
                
                detailView->setHtml(detailText);
            }
        }
    }

    void onPathItemSelectionChanged()
    {
        auto selectedItems = pathTable->selectedItems();
        if (!selectedItems.isEmpty()) {
            int row = selectedItems.first()->row();
            QTableWidgetItem* typeItem = pathTable->item(row, 0);
            QTableWidgetItem* pathItem = pathTable->item(row, 1);
            
            if (typeItem && pathItem) {
                QString detailText = QString(
                    "<h3 style='color: #64B5F6; margin-bottom: 10px;'>PATH Entry (%1)</h3>"
                    "<div style='background: #2E2E2E; padding: 12px; border-radius: 6px; "
                    "border-left: 3px solid #64B5F6; font-family: Consolas, monospace;'>"
                    "<span style='color: #E0E0E0; line-height: 1.4;'>%2</span>"
                    "</div>"
                ).arg(typeItem->text()).arg(pathItem->text());
                
                pathDetailView->setHtml(detailText);
            }
        }
    }

private:
    void setupUI()
    {
        setWindowTitle("Environment Variables Viewer");
        setMinimumSize(1200, 700);
        resize(1400, 800);
        
        // Main widget and layout
        QWidget* centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);
        
        QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
        mainLayout->setSpacing(0);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        
        // Header section
        setupHeader(mainLayout);
        
        // Tab widget for different views
        QTabWidget* tabWidget = new QTabWidget();
        tabWidget->setObjectName("mainTabs");
        
        // Environment Variables Tab
        setupEnvironmentTab(tabWidget);
        
        // PATH Variables Tab
        setupPathTab(tabWidget);
        
        mainLayout->addWidget(tabWidget);
        
        // Status bar
        statusLabel = new QLabel("Loading environment variables...", this);
        statusBar()->addWidget(statusLabel);
    }
    
    void setupHeader(QVBoxLayout* mainLayout)
    {
        QFrame* headerFrame = new QFrame();
        headerFrame->setObjectName("headerFrame");
        headerFrame->setFixedHeight(80);
        
        QHBoxLayout* headerLayout = new QHBoxLayout(headerFrame);
        headerLayout->setContentsMargins(20, 10, 20, 10);
        
        // Title and subtitle
        QVBoxLayout* titleLayout = new QVBoxLayout();
        QLabel* titleLabel = new QLabel("Environment Variables Viewer");
        titleLabel->setObjectName("titleLabel");
        
        QLabel* subtitleLabel = new QLabel("Windows System Environment Configuration with PATH Analysis");
        subtitleLabel->setObjectName("subtitleLabel");
        
        titleLayout->addWidget(titleLabel);
        titleLayout->addWidget(subtitleLabel);
        titleLayout->setSpacing(2);
        
        headerLayout->addLayout(titleLayout);
        headerLayout->addStretch();
        
        // Global refresh button
        refreshBtn = new QPushButton("Refresh All");
        refreshBtn->setObjectName("modernButton");
        refreshBtn->setFixedSize(100, 32);
        
        headerLayout->addWidget(refreshBtn);
        
        mainLayout->addWidget(headerFrame);
        
        // Connect signals
        connect(refreshBtn, &QPushButton::clicked, this, &EnvironmentViewer::refreshVariables);
    }

    void setupEnvironmentTab(QTabWidget* tabWidget)
    {
        QWidget* envTab = new QWidget();
        QVBoxLayout* envLayout = new QVBoxLayout(envTab);
        envLayout->setContentsMargins(10, 10, 10, 10);
        
        // Search box for environment variables
        QHBoxLayout* searchLayout = new QHBoxLayout();
        QLabel* searchLabel = new QLabel("Search:");
        searchLabel->setObjectName("searchLabel");
        
        searchBox = new QLineEdit();
        searchBox->setPlaceholderText("Search environment variables...");
        searchBox->setObjectName("searchBox");
        
        searchLayout->addWidget(searchLabel);
        searchLayout->addWidget(searchBox);
        searchLayout->addStretch();
        
        envLayout->addLayout(searchLayout);
        
        // Content splitter
        QSplitter* envSplitter = new QSplitter(Qt::Horizontal);
        
        // Table panel
        setupEnvTablePanel(envSplitter);
        
        // Detail panel
        setupEnvDetailPanel(envSplitter);
        
        envSplitter->setSizes({800, 400});
        envLayout->addWidget(envSplitter);
        
        tabWidget->addTab(envTab, "Environment Variables");
        
        connect(searchBox, &QLineEdit::textChanged, this, &EnvironmentViewer::filterVariables);
    }

    void setupPathTab(QTabWidget* tabWidget)
    {
        QWidget* pathTab = new QWidget();
        QVBoxLayout* pathLayout = new QVBoxLayout(pathTab);
        pathLayout->setContentsMargins(10, 10, 10, 10);
        
        // Search box for PATH variables
        QHBoxLayout* pathSearchLayout = new QHBoxLayout();
        QLabel* pathSearchLabel = new QLabel("Search:");
        pathSearchLabel->setObjectName("searchLabel");
        
        pathSearchBox = new QLineEdit();
        pathSearchBox->setPlaceholderText("Search PATH entries...");
        pathSearchBox->setObjectName("searchBox");
        
        pathSearchLayout->addWidget(pathSearchLabel);
        pathSearchLayout->addWidget(pathSearchBox);
        pathSearchLayout->addStretch();
        
        pathLayout->addLayout(pathSearchLayout);
        
        // Content splitter
        QSplitter* pathSplitter = new QSplitter(Qt::Horizontal);
        
        // PATH table panel
        setupPathTablePanel(pathSplitter);
        
        // PATH detail panel
        setupPathDetailPanel(pathSplitter);
        
        pathSplitter->setSizes({800, 400});
        pathLayout->addWidget(pathSplitter);
        
        tabWidget->addTab(pathTab, "PATH Analysis");
        
        connect(pathSearchBox, &QLineEdit::textChanged, this, &EnvironmentViewer::filterPaths);
    }
    
    void setupEnvTablePanel(QSplitter* splitter)
    {
        QWidget* tablePanel = new QWidget();
        QVBoxLayout* layout = new QVBoxLayout(tablePanel);
        layout->setContentsMargins(10, 10, 5, 10);
        
        envTable = new QTableWidget();
        envTable->setColumnCount(2);
        envTable->setHorizontalHeaderLabels({"Variable Name", "Value"});
        envTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        envTable->setAlternatingRowColors(true);
        envTable->setSortingEnabled(true);
        envTable->setObjectName("modernTable");
        
        // Configure headers
        QHeaderView* header = envTable->horizontalHeader();
        header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        header->setSectionResizeMode(1, QHeaderView::Stretch);
        header->setObjectName("tableHeader");
        
        envTable->verticalHeader()->setVisible(false);
        
        layout->addWidget(envTable);
        splitter->addWidget(tablePanel);
        
        connect(envTable, &QTableWidget::itemSelectionChanged, 
                this, &EnvironmentViewer::onEnvItemSelectionChanged);
    }

    void setupPathTablePanel(QSplitter* splitter)
    {
        QWidget* tablePanel = new QWidget();
        QVBoxLayout* layout = new QVBoxLayout(tablePanel);
        layout->setContentsMargins(10, 10, 5, 10);
        
        pathTable = new QTableWidget();
        pathTable->setColumnCount(2);
        pathTable->setHorizontalHeaderLabels({"Type", "Path"});
        pathTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        pathTable->setAlternatingRowColors(true);
        pathTable->setSortingEnabled(true);
        pathTable->setObjectName("modernTable");
        
        // Configure headers
        QHeaderView* header = pathTable->horizontalHeader();
        header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        header->setSectionResizeMode(1, QHeaderView::Stretch);
        header->setObjectName("tableHeader");
        
        pathTable->verticalHeader()->setVisible(false);
        
        layout->addWidget(pathTable);
        splitter->addWidget(tablePanel);
        
        connect(pathTable, &QTableWidget::itemSelectionChanged, 
                this, &EnvironmentViewer::onPathItemSelectionChanged);
    }
    
    void setupEnvDetailPanel(QSplitter* splitter)
    {
        QWidget* detailPanel = new QWidget();
        QVBoxLayout* layout = new QVBoxLayout(detailPanel);
        layout->setContentsMargins(5, 10, 10, 10);
        
        QLabel* detailTitle = new QLabel("Variable Details");
        detailTitle->setObjectName("sectionTitle");
        
        detailView = new QTextEdit();
        detailView->setObjectName("detailView");
        detailView->setReadOnly(true);
        detailView->setHtml(
            "<div style='color: #999; text-align: center; margin-top: 50px;'>"
            "<h3>Select a variable to view details</h3>"
            "<p>Click on any environment variable from the list to see its detailed information here.</p>"
            "</div>"
        );
        
        layout->addWidget(detailTitle);
        layout->addWidget(detailView);
        splitter->addWidget(detailPanel);
    }

    void setupPathDetailPanel(QSplitter* splitter)
    {
        QWidget* detailPanel = new QWidget();
        QVBoxLayout* layout = new QVBoxLayout(detailPanel);
        layout->setContentsMargins(5, 10, 10, 10);
        
        QLabel* detailTitle = new QLabel("PATH Entry Details");
        detailTitle->setObjectName("sectionTitle");
        
        pathDetailView = new QTextEdit();
        pathDetailView->setObjectName("detailView");
        pathDetailView->setReadOnly(true);
        pathDetailView->setHtml(
            "<div style='color: #999; text-align: center; margin-top: 50px;'>"
            "<h3>Select a PATH entry to view details</h3>"
            "<p>Click on any PATH entry from the list to see its detailed information here.</p>"
            "</div>"
        );
        
        layout->addWidget(detailTitle);
        layout->addWidget(pathDetailView);
        splitter->addWidget(detailPanel);
    }
    
    void loadEnvironmentVariables()
    {
        envTable->setRowCount(0);
        
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        QStringList keys = env.keys();
        keys.sort();
        
        envTable->setRowCount(keys.size());
        
        for (int i = 0; i < keys.size(); ++i) {
            QString key = keys.at(i);
            QString value = env.value(key);
            
            QTableWidgetItem* nameItem = new QTableWidgetItem(key);
            QTableWidgetItem* valueItem = new QTableWidgetItem(value);
            
            // Set tooltips for long values
            if (value.length() > 50) {
                valueItem->setToolTip(value);
            }
            
            envTable->setItem(i, 0, nameItem);
            envTable->setItem(i, 1, valueItem);
        }
        
        updateStatusLabel();
    }

    void loadPathVariables()
    {
        pathTable->setRowCount(0);
        pathManager->loadPaths();
        
        const auto& userPaths = pathManager->getUserPaths();
        const auto& systemPaths = pathManager->getSystemPaths();
        
        int totalRows = userPaths.size() + systemPaths.size();
        pathTable->setRowCount(totalRows);
        
        int row = 0;
        
        // Add user paths
        for (const auto& path : userPaths) {
            QTableWidgetItem* typeItem = new QTableWidgetItem("User");
            QTableWidgetItem* pathItem = new QTableWidgetItem(QString::fromStdString(path));
            
            typeItem->setIcon(QIcon()); // You can add icons here
            pathItem->setToolTip(QString::fromStdString(path));
            
            pathTable->setItem(row, 0, typeItem);
            pathTable->setItem(row, 1, pathItem);
            row++;
        }
        
        // Add system paths
        for (const auto& path : systemPaths) {
            QTableWidgetItem* typeItem = new QTableWidgetItem("System");
            QTableWidgetItem* pathItem = new QTableWidgetItem(QString::fromStdString(path));
            
            typeItem->setIcon(QIcon()); // You can add icons here
            pathItem->setToolTip(QString::fromStdString(path));
            
            pathTable->setItem(row, 0, typeItem);
            pathTable->setItem(row, 1, pathItem);
            row++;
        }
        
        updatePathStatusLabel();
    }
    
    void updateStatusLabel()
    {
        int visibleRows = 0;
        for (int i = 0; i < envTable->rowCount(); ++i) {
            if (!envTable->isRowHidden(i)) {
                visibleRows++;
            }
        }
        
        QString status = QString("Environment: %1 of %2 variables")
                        .arg(visibleRows).arg(envTable->rowCount());
        statusLabel->setText(status);
    }

    void updatePathStatusLabel()
    {
        int visibleRows = 0;
        for (int i = 0; i < pathTable->rowCount(); ++i) {
            if (!pathTable->isRowHidden(i)) {
                visibleRows++;
            }
        }
        
        const auto& userPaths = pathManager->getUserPaths();
        const auto& systemPaths = pathManager->getSystemPaths();
        
        QString pathStatus = QString(" | PATH: %1 of %2 entries (%3 user, %4 system)")
                           .arg(visibleRows)
                           .arg(pathTable->rowCount())
                           .arg(userPaths.size())
                           .arg(systemPaths.size());
        
        statusLabel->setText(statusLabel->text() + pathStatus);
    }
    
    void applyDarkTheme()
    {
        setStyleSheet(R"(
            QMainWindow {
                background-color: #1E1E1E;
                color: #FFFFFF;
            }
            
            #headerFrame {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                    stop:0 #2D2D30, stop:1 #1E1E1E);
                border-bottom: 1px solid #3E3E42;
            }
            
            #titleLabel {
                font-size: 24px;
                font-weight: bold;
                color: #FFFFFF;
            }
            
            #subtitleLabel {
                font-size: 12px;
                color: #CCCCCC;
            }
            
            #sectionTitle {
                font-size: 14px;
                font-weight: bold;
                color: #64B5F6;
                margin-bottom: 8px;
            }

            #searchLabel {
                font-size: 12px;
                color: #CCCCCC;
                font-weight: bold;
            }
            
            #searchBox {
                background-color: #2D2D30;
                border: 1px solid #3E3E42;
                border-radius: 4px;
                padding: 6px 12px;
                color: #FFFFFF;
                font-size: 13px;
            }
            
            #searchBox:focus {
                border: 1px solid #64B5F6;
                background-color: #252526;
            }
            
            #modernButton {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                    stop:0 #505050, stop:1 #404040);
                border: 1px solid #606060;
                border-radius: 4px;
                color: #FFFFFF;
                font-weight: bold;
                font-size: 12px;
            }
            
            #modernButton:hover {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                    stop:0 #606060, stop:1 #505050);
                border: 1px solid #64B5F6;
            }
            
            #modernButton:pressed {
                background: #404040;
            }

            #mainTabs {
                background-color: #1E1E1E;
            }

            #mainTabs::pane {
                border: 1px solid #3E3E42;
                background-color: #1E1E1E;
            }

            #mainTabs::tab-bar {
                alignment: left;
            }

            QTabBar::tab {
                background: #2D2D30;
                color: #CCCCCC;
                padding: 8px 16px;
                margin-right: 2px;
                border-top-left-radius: 4px;
                border-top-right-radius: 4px;
            }

            QTabBar::tab:selected {
                background: #64B5F6;
                color: #FFFFFF;
            }

            QTabBar::tab:hover:!selected {
                background: #3E3E42;
            }
            
            #modernTable {
                background-color: #252526;
                alternate-background-color: #2D2D30;
                gridline-color: #3E3E42;
                selection-background-color: #094771;
                color: #FFFFFF;
                border: 1px solid #3E3E42;
            }
            
            #modernTable::item {
                padding: 8px;
                border: none;
            }
            
            #modernTable::item:selected {
                background-color: #094771;
                color: #FFFFFF;
            }
            
            #tableHeader {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                    stop:0 #3E3E42, stop:1 #2D2D30);
                color: #FFFFFF;
                border: none;
                font-weight: bold;
                padding: 8px;
            }
            
            #tableHeader::section {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                    stop:0 #3E3E42, stop:1 #2D2D30);
                color: #FFFFFF;
                border: none;
                border-right: 1px solid #555555;
                padding: 8px;
                font-weight: bold;
            }
            
            #detailView {
                background-color: #1E1E1E;
                border: 1px solid #3E3E42;
                border-radius: 4px;
                padding: 10px;
                color: #FFFFFF;
                font-family: 'Segoe UI', Arial, sans-serif;
            }
            
            QStatusBar {
                background-color: #2D2D30;
                color: #CCCCCC;
                border-top: 1px solid #3E3E42;
            }
            
            QSplitter::handle {
                background-color: #3E3E42;
                width: 1px;
            }
        )");
    }

private:
    // Environment Variables Tab
    QTableWidget* envTable;
    QLineEdit* searchBox;
    QTextEdit* detailView;
    
    // PATH Variables Tab
    QTableWidget* pathTable;
    QLineEdit* pathSearchBox;
    QTextEdit* pathDetailView;
    
    // Common
    QPushButton* refreshBtn;
    QLabel* statusLabel;
    
    // PATH Manager
    PathManager* pathManager;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application properties
    app.setApplicationName("Environment Variables Viewer");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("EnvViewer");
    
    // Create and show main window
    EnvironmentViewer window;
    window.show();
    
    return app.exec();
}

#include "main.moc"
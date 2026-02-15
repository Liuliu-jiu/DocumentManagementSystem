#ifndef VIEW_FUNCTIONGUIDEWINDOW_H
#define VIEW_FUNCTIONGUIDEWINDOW_H

#include <QWidget>
#include<QResizeEvent>
#include<QTimer>
#include<QLabel>
#define CREATE_PIXMAP_COUNT 6
#define OPEN_SYSTEN_FILE_COUNT 6
#define FIND_ONE_PATH_FILE_COUNT 4
#define FIND_ALL_PATH_FILE_COUNT 2
namespace Ui {
class View_FunctionGuideWindow;
}

class View_FunctionGuideWindow : public QWidget
{
    Q_OBJECT

public:
    explicit View_FunctionGuideWindow(QWidget *parent = nullptr);
    ~View_FunctionGuideWindow();

private:
    QString currentPixampPath;
private slots:
    void on_nextPagePushButton_clicked();
    void on_showFunctionGuideTabWidget_currentChanged(int index);

private:
    Ui::View_FunctionGuideWindow *ui;

    void resizeEvent(QResizeEvent* event) override;
};

#endif // VIEW_FUNCTIONGUIDEWINDOW_H

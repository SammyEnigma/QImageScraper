#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <QNetworkReply>

#include <map>
#include <memory>

#include <qt_enhance/network/download_supervisor.hpp>

class image_search;

namespace Ui {
class MainWindow;
}

class general_settings;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_actionScroll_triggered();
    void on_actionDownload_triggered();

    void on_actionSettings_triggered();

    void on_actionRefresh_triggered();

    void on_actionHome_triggered();

    void on_actionInfo_triggered();

    void on_actionStop_triggered();

private:
    enum class link_choice
    {
        big,
        small
    };

    void download_img_error(std::shared_ptr<qte::net::download_supervisor::download_task> task,
                            QString const &error_msg);
    void download_finished(std::shared_ptr<qte::net::download_supervisor::download_task> task);
    void download_img(std::tuple<QString, QString, link_choice> info);
    void download_next_image();
    void download_progress(std::shared_ptr<qte::net::download_supervisor::download_task> task,
                           qint64 bytesReceived, qint64 bytesTotal);
    void download_small_img(QString const &save_as,
                            std::tuple<QString, QString, link_choice> const &img_info);
    void found_img_link(QString const &big_img_link, QString const &small_img_link);
    void general_settings_ok_clicked();
    void process_go_to_first_page();
    void process_go_to_second_page();
    void process_scroll_second_page_done();
    void set_enabled_main_window_except_stop(bool value);
    void refresh_window();

    Ui::MainWindow *ui;

    struct download_statistic
    {
        void clear();

        size_t fail() const;
        size_t success() const;

        size_t big_img_download_ = 0;
        size_t small_img_download_ = 0;
        size_t total_download_ = 0;
    };

    QStringList big_img_links_;
    QSize default_max_size_;
    QSize default_min_size_;
    bool download_finished_;
    qte::net::download_supervisor *downloader_;    
    general_settings *general_settings_;
    std::map<size_t, std::tuple<QString, QString, link_choice>> img_links_map_;
    image_search *img_search_;
    QStringList small_img_links_;
    download_statistic statistic_;
    void change_search_engine();
};

#endif // MAINWINDOW_HPP

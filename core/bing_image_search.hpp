#ifndef BING_IMAGE_SEARCH_HPP
#define BING_IMAGE_SEARCH_HPP

#include "image_search.hpp"

#include <QColor>
#include <QSize>

class QTimer;

class bing_image_search : public image_search
{
    Q_OBJECT
public:
    explicit bing_image_search(QWebEnginePage &page, QObject *parent = nullptr);

    void get_page_link(std::function<void(QStringList const&)> callback) override;
    void go_to_first_page() override;
    void go_to_second_page() override;
    void parse_imgs_link(QStringList const &page_links,
                         std::function<void(QString const&, QString const&)> callback) override;
    void scroll_second_page(size_t max_search_size) override;

private:
    enum class state{
        parse_img_link,
        to_first_page,
        to_second_page,
        scroll_page
    };

    void load_web_page_finished(bool ok) override;
    void parse_imgs_link_content();
    void parse_page_link(QString const &contents);
    void scroll_web_page();
    void scroll_web_page_impl();

    QStringList img_page_links_;
    size_t max_search_size_;
    std::function<void(QString const&, QString const&)> parse_img_link_callback_;
    QString second_page_contents_;
    size_t scroll_count_;
    size_t scroll_limit_;
    state state_;
    QTimer *timer_;
};

#endif // BING_IMAGE_SEARCH_HPP

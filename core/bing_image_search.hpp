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

    void find_image_links(QString const &target, size_t max_search_size = 0) override;

private:
    enum class state{
       load_first_page,
       scroll_page,
       parse_page_link,
       parse_img_link
    };

    void load_web_page_finished(bool ok) override;
    void parse_imgs_link();
    void parse_imgs_link_content();    
    void parse_page_link();
    void parse_page_link_by_regex(QString const &contents);
    void scroll_web_page();

    QColor color_;    
    QStringList img_page_links_;        
    size_t max_search_size_;
    QSize max_size_;
    QSize min_size_;
    size_t scroll_count_;
    state state_;
    QStringList suffix_;
    QTimer *timer_;
    qreal ypos_;
};

#endif // BING_IMAGE_SEARCH_HPP

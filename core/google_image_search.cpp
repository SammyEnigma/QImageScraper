#include "google_image_search.hpp"
#include "js_function.hpp"

#include <QsLog.h>

#include <QSize>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QTimer>
#include <QWebEnginePage>

#include <QFile>
#include <QTextStream>

namespace{

constexpr int scroll_page_duration = 1500;

}

google_image_search::google_image_search(QWebEnginePage &page, QObject *parent) :
    image_search{page, parent},
    max_search_size_{0},
    scroll_count_{0},
    scroll_limit_{2},
    state_{state::to_search_page},
    stop_scroll_page_{false}
{

}

void google_image_search::get_page_link(std::function<void (const QStringList&)> callback)
{
    get_web_page().toHtml([this, callback](QString const &contents)
    {
        parse_page_link(contents);
        callback(img_page_links_);
    });
}

void google_image_search::go_to_search_page()
{
    state_ = state::to_search_page;
    img_page_links_.clear();
    get_web_page().load(QUrl("https://images.google.com/"));
}

void google_image_search::get_imgs_link(const QString &page_link,
                                        std::function<void (const QString &, const QString &)> callback)
{
    parse_img_link_callback_ = callback;
    state_ = state::parse_img_link;
    get_web_page().load(page_link);
}

void google_image_search::get_imgs_link_from_gallery_page(std::function<void (const QStringList &, const QStringList &)> callback)
{
    state_ = state::get_img_link_from_gallery_page;
    get_web_page().toHtml([this, callback](QString const &contents)
    {
        QRegularExpression const reg("<div class=\"rg_meta\">{[^}]*}");
        auto iter = reg.globalMatch(contents);
        QStringList big_im, small_im;
        while(iter.hasNext()){
            auto const match = iter.next();
            QRegularExpression const link_big("\"ou\":\"([^\"]*)");
            QRegularExpression const link_small("\"tu\":\"([^\"]*)");
            auto const bm = link_big.match(match.captured(0));
            auto const sm = link_small.match(match.captured(0));
            big_im.push_back(bm.captured(1));
            small_im.push_back(sm.captured(1).replace("\\u003d","="));
        }
        callback(big_im, small_im);
    });
}

void google_image_search::get_search_target(std::function<void (const QString &)> callback)
{
    get_web_page().runJavaScript("function jscmd(){return document.getElementById(\"lst-ib\").value} jscmd()",
                                 [this, callback](QVariant const &contents)
    {
        QLOG_INFO()<<"gallery page target:"<<contents.toString();
        callback(contents.toString());
    });
}

void google_image_search::go_to_gallery_page(const QString &target)
{
    state_ = state::to_gallery_page;
    if(!target.isEmpty()){
        get_web_page().load("https://www.google.co.in/search?q=" +
                            target + "&source=lnms&tbm=isch");
    }
}

void google_image_search::show_more_images(size_t max_search_size)
{
    max_search_size_ = max_search_size;
    if((int)max_search_size_ > img_page_links_.size()){
        scroll_limit_ = (max_search_size_ - img_page_links_.size()) / 100 + 1;
    }else{
        scroll_limit_ = 4;
    }
    state_ = state::show_more_images;
    scroll_web_page();
}

void google_image_search::stop_show_more_images()
{
    stop_scroll_page_ = true;
}

QString google_image_search::decode_link_char(QString link)
{
    link.replace("&amp;", "&");
    link.replace("\\\\u003d", "=");
    link.chop(1);

    return link;
}

void google_image_search::load_web_page_finished(bool ok)
{
    QLOG_INFO()<<"load web page finished:"<<ok<<", url:"<<get_web_page().url();
    if(ok){
        QRegularExpression const re("https://www.google.[^/]*/search?");
        if(get_web_page().url().toString().contains(re)){
            state_ = state::to_gallery_page;
            emit go_to_gallery_page_done();
            return;
        }

        switch(state_){
        case state::to_search_page:{
            QLOG_INFO()<<"state to first page";
            emit go_to_search_page_done();
            break;
        }
        case state::to_gallery_page:{
            QLOG_INFO()<<"state to second page";
            emit go_to_gallery_page_done();
            break;
        }
        case state::show_more_images:{
            QLOG_INFO()<<"state scroll page";
            break;
        }
        case state::parse_img_link:{
            QLOG_INFO()<<"state parse img link";
            parse_imgs_link_content();
            break;
        }
        case state::get_img_link_from_gallery_page:{
            QLOG_INFO()<<"state get_img_link_from_sec_page";
            return;
        }
        default:
            QLOG_INFO()<<"default state";
            break;
        }
    }else{
        emit search_error(image_search_error::error::load_page_error);
    }
}

void google_image_search::parse_imgs_link_content()
{
    get_web_page().toHtml([this](QString const &contents){
        QRegularExpression const reg("\\[\"image_group\",(null,){4}\\[null,\"\\[{3}"
                                     "[^\"]*\"[^\"]*\",\\[\\\\\"([^\"]*)\""
                                     "[^\\[]*\\[\\\\\"([^\"]*)\"");
        auto const match = reg.match(contents);
        if(match.hasMatch()){
            QLOG_INFO()<<"img link:"<<decode_link_char(match.captured(2))<<"\n"
                      <<decode_link_char(match.captured(3));
        }else{
            QLOG_INFO()<<"cannot capture img link";
        }

        parse_img_link_callback_(decode_link_char(match.captured(3)),
                                 decode_link_char(match.captured(2)));
    });
}

void google_image_search::parse_page_link(const QString &contents)
{
    QRegularExpression const reg("<a href=\"(/imgres\\?imgurl[^\"]*)\"");
    auto iter = reg.globalMatch(contents);
    QStringList links;
    while(iter.hasNext()){
        QRegularExpressionMatch const match = iter.next();
        QString url = QUrl("https://www.google.com" + match.captured(1)).toString();
        url.replace("&amp;", "&");
        links.push_back(std::move(url));
    }
    links.removeDuplicates();
    QLOG_INFO()<<"google parse page link total match link:"<<links.size();
    if(links.size() > img_page_links_.size()){
        links.swap(img_page_links_);
    }
}

void google_image_search::scroll_web_page()
{
    //we need to setup timer because web view may not able to update in time,
    //this may cause the page stop scrolling too early
    if(state_ == state::show_more_images){
        scroll_count_ = 0;
        stop_scroll_page_ = false;
        QTimer::singleShot(scroll_page_duration, [this](){scroll_web_page_impl();});
    }
}

void google_image_search::scroll_web_page_impl()
{
    QLOG_INFO()<<__func__<<":scroll_count:"<<scroll_count_;
    if(state_ != state::show_more_images){
        return;
    }

    if(stop_scroll_page_){
        stop_scroll_page_ = false;
        emit show_more_images_done();
        return;
    }

    if(scroll_count_ == scroll_limit_){
        emit show_more_images_done();
        return;
    }

    get_web_page().toHtml([this](QString const &contents)
    {
        QLOG_INFO()<<"scroll page contents:"<<contents.size();
        if(contents.contains("Show more results")){
            QLOG_INFO()<<"found Show more results";
            get_web_page().runJavaScript("document.getElementById(\"smb\").click();"
                                         "window.scrollTo(0, document.body.scrollHeight);");
        }else{
            QLOG_INFO()<<"cannot found Show more results";
            get_web_page().runJavaScript("window.scrollTo(0, document.body.scrollHeight)");
        }
        ++scroll_count_;
        emit second_page_scrolled();
        QTimer::singleShot(scroll_page_duration, [this](){scroll_web_page_impl();});
    });
}

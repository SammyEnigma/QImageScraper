#include "bing_image_search.hpp"
#include "js_function.hpp"

#include <QDebug>
#include <QSize>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QTimer>
#include <QWebEnginePage>

bing_image_search::bing_image_search(QWebEnginePage &page, QObject *parent) :
    image_search(page, parent),
    max_search_size_(0),
    scroll_count_(0),
    scroll_limit_(2),
    state_(state::to_first_page),
    timer_(new QTimer(this))
{
    auto *web_page = &get_web_page();
    connect(web_page, &QWebEnginePage::loadProgress, [](int progress){ qDebug()<<"load progress:"<<progress;});
    connect(web_page, &QWebEnginePage::loadStarted, [](){ qDebug()<<"load started";});
}

QStringList bing_image_search::get_page_link()
{
    parse_page_link(second_page_contents_);
    return img_page_links_;
}

void bing_image_search::go_to_first_page()
{
    state_ = state::to_first_page;
    img_page_links_.clear();
    second_page_contents_.clear();
    get_web_page().load(QUrl("https://www.bing.com/?scope=images&nr=1&FORM=NOFORM"));
}

void bing_image_search::go_to_second_page()
{
    state_ = state::to_second_page;
    get_web_page().runJavaScript("function jscmd(){return document.getElementById(\"sb_form_q\").value} jscmd()",
                                 [this](QVariant const &contents)
    {
        if(contents.isValid()){
            get_web_page().load("https://www.bing.com/images/search?q=" + contents.toString());
        }else{
            emit error_msg(tr("Please enter the target you want to search"));
        }
    });
}

void bing_image_search::scroll_second_page(size_t max_search_size)
{
    max_search_size_ = max_search_size;
    if((int)max_search_size_ > img_page_links_.size()){
        scroll_limit_ = (max_search_size_ - img_page_links_.size()) / 35 + 1;
    }else{
        scroll_limit_ = 4;
    }
    state_ = state::scroll_page;
    scroll_web_page();
}

void bing_image_search::load_web_page_finished(bool ok)
{
    qDebug()<<"load web page finished:"<<ok<<", url:"<<get_web_page().url().toString();
    if(ok){
        if(get_web_page().url().toString().contains("https://www.bing.com/images/search?q=")){
            state_ = state::to_second_page;
            emit go_to_second_page_done();
            return;
        }

        switch(state_){
        case state::to_first_page:{
            qDebug()<<"state to first page";
            emit go_to_first_page_done();
            break;
        }
        case state::to_second_page:{
            qDebug()<<"state to second page";
            emit go_to_second_page_done();
            break;
        }
        case state::scroll_page:{
            qDebug()<<"state scroll page";
            break;
        }
        case state::parse_img_link:{
            qDebug()<<"state parse img link";
            parse_imgs_link_content();
            break;
        }
        default:
            qDebug()<<"default state";
            break;
        }
    }else{
        emit error_msg(tr("Failed to load the web page"));
    }
}

void bing_image_search::parse_imgs_link()
{
    state_ = state::parse_img_link;
    get_web_page().toHtml([this](QString const &contents)
    {
        second_page_contents_ = contents;
        parse_page_link(contents);
        parse_imgs_link_content();
    });
}

void bing_image_search::parse_imgs_link_content()
{
    get_web_page().toHtml([this](QString const &contents)
    {
        QRegularExpression reg("<img class=\"mainImage\" src=\"([^\"]*)\" src2=\"([^\"]*)");
        auto match = reg.match(contents);
        if(match.hasMatch()){
            qDebug()<<"img link:"<<match.captured(1)<<"\n"<<match.captured(2);

        }else{
            qDebug()<<"cannot capture img link";
        }
        if(!img_page_links_.isEmpty()){
            img_page_links_.pop_front();
            emit found_image_link(match.captured(2).replace("&amp;", "&"),
                                  match.captured(1).replace("&amp;", "&"));
            if(!img_page_links_.empty()){
                get_web_page().load(img_page_links_[0]);
            }
        }else{
            qDebug()<<"bing image search parse all imgs link";
            emit parse_all_image_link();
        }
    });
}

void bing_image_search::scroll_web_page_impl()
{
    ++scroll_count_;
    qDebug()<<__func__<<":scroll_count:"<<scroll_count_;
    if(state_ != state::scroll_page){
        return;
    }

    if(scroll_count_ == scroll_limit_){
        timer_->stop();
        emit scroll_second_page_done();
    }

    get_web_page().toHtml([this](QString const &contents)
    {
        qDebug()<<"get image link contents";
        second_page_contents_ = contents;
        get_web_page().findText("See more images", QWebEnginePage::FindFlag(), [this](bool found)
        {
            if(found){
                qDebug()<<"found See more images";
                get_web_page().runJavaScript("document.getElementsByClassName(\"btn_seemore\")[0].click();"
                                             "window.scrollTo(0, document.body.scrollHeight);");
            }else{
                qDebug()<<"cannot found See more images";
                get_web_page().runJavaScript("window.scrollTo(0, document.body.scrollHeight)");
            }
            emit second_page_scrolled();
        });
    });
}

void bing_image_search::parse_page_link(const QString &contents)
{
    QRegularExpression reg("(search\\?view=detailV2[^\"]*)");
    auto iter = reg.globalMatch(contents);
    QStringList links;
    while(iter.hasNext()){
        QRegularExpressionMatch match = iter.next();
        if(match.captured(1).right(20) != "ipm=vs#enterinsights"){
            QString url = QUrl("https://www.bing.com/images/" + match.captured(1)).toString();
            url.replace("&amp;", "&");
            links.push_back(url);
            emit found_page_link(url);
        }
    }
    links.removeDuplicates();
    qDebug()<<"total match link:"<<links.size();
    if(links.size() > img_page_links_.size()){
        links.swap(img_page_links_);
    }
}

void bing_image_search::scroll_web_page()
{
    //we need to setup timer if the web view are shown on the screen.
    //Because web view may not able to update in time, this may cause the signal scrollPositionChanged
    //never emit, because the web page do not have enough of space to scroll down
    if(state_ == state::scroll_page){
        scroll_count_ = 0;
        connect(timer_, &QTimer::timeout, this, &bing_image_search::scroll_web_page_impl);
        timer_->start(2000);
    }
}

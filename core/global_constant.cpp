#include "global_constant.hpp"

#include <QObject>

QString global_constant::bing_search_name()
{
    return QObject::tr("Bing");
}

QString global_constant::google_search_name()
{
    return QObject::tr("Google");
}

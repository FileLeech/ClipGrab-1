/*
    ClipGrab³
    Copyright (C) Philipp Schmieder
    http://clipgrab.de
    feedback [at] clipgrab [dot] de

    This file is part of ClipGrab.
    ClipGrab is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    ClipGrab is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with ClipGrab.  If not, see <http://www.gnu.org/licenses/>.
*/



#include "video_dailymotion.h"

video_dailymotion::video_dailymotion()
{
    this->_name = "Dailymotion";
    this->_supportsTitle = true;
    this->_supportsDescription = true;
    this->_supportsThumbnail = true;
    this->_supportsSearch = true;
    this->_icon = 0;
    this->_urlRegExp << QRegExp("http://\\w*\\.dailymotion\\.com/video/.*", Qt::CaseInsensitive);
}

video* video_dailymotion::createNewInstance()
{
    return new video_dailymotion();
}

bool video_dailymotion::setUrl(QString url)
{
    _originalUrl = url;

    if (_url.isEmpty())
    {
        this->_url = QUrl(url);

        QNetworkCookieJar* cookieJar = new QNetworkCookieJar;
        QList<QNetworkCookie> cookieList;
        cookieList << QNetworkCookie("ff", "off");
        cookieJar->setCookiesFromUrl(cookieList, _url);
        this->handler->networkAccessManager->setCookieJar(cookieJar);

        return _url.isValid();
    }
    else
    {
        return false;
    }
}

void video_dailymotion::parseVideo(QString html)
{
    QRegExp expression;
    expression = QRegExp("<meta property=\"og:title\" content=\"(.*)\"");
    expression.setMinimal(true);
    if (expression.indexIn(html) !=-1)
    {

        _title = QString(expression.cap(1));

        html = QUrl::fromEncoded(html.toAscii()).toString(QUrl::None);
        expression = QRegExp("dmp\\.create\\(document\\.getElementById\\('player'\\),\\s?(.+\\})\\);");
        expression.setMinimal(true);
        if (expression.indexIn(html)!=-1)
        {
            QString json = expression.cap(1);

            QList<dailymotion_quality> qualities;
            qualities << dailymotion_quality("1080", tr("HD (1080p)"));
            qualities << dailymotion_quality("720", tr("HD (720p)"));
            qualities << dailymotion_quality("480", tr("480p"));
            qualities << dailymotion_quality("380", tr("380p"));
            qualities << dailymotion_quality("240", tr("240p"));

            QListIterator<dailymotion_quality> i(qualities);
            while (i.hasNext())
            {
                dailymotion_quality quality = i.next();
                QString url = getQualityUrl(json, quality.key);
                if (!url.isEmpty())
                {
                    videoQuality newQuality;
                    newQuality.quality = quality.name;
                    newQuality.videoUrl = url;
                    newQuality.containerName = url.contains(".mp4") ? ".mp4" : ".flv";
                    _supportedQualities.append(newQuality);
                }
            }
        }

    }

    if (_supportedQualities.isEmpty() | _title.isEmpty())
    {
        emit error("Could not retrieve video title.", this);

    }
    emit analysingFinished();
 }

QString video_dailymotion::getQualityUrl(QString json, QString quality)
{
    QWebView* view = new QWebView();
    view->setHtml("<script>function data() {return " + json + ";}</script>");
    QString result = view->page()->mainFrame()->evaluateJavaScript("data().metadata.qualities['" + quality + "'][0].url").toString();

    view->deleteLater();
    return result;
}

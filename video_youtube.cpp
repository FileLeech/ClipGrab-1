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



#include "video_youtube.h"
#include "QMutableListIterator"

video_youtube::video_youtube()
{
    this->_name = "Youtube";
    this->_supportsTitle = true;
    this->_supportsDescription = true;
    this->_supportsThumbnail = true;
    this->_supportsSearch = true;
    this->_icon = 0;
    this->_urlRegExp << QRegExp("http[s]?://\\w*\\.youtube\\.com/watch.*v\\=.*", Qt::CaseInsensitive);
    this->_urlRegExp << QRegExp("http[s]?://\\w*\\.youtube\\.com/view_play_list\\?p\\=.*&v\\=.*", Qt::CaseInsensitive);
    this->_urlRegExp << QRegExp("http[s]?://youtu.be/.*", Qt::CaseInsensitive);
    this->_urlRegExp << QRegExp("http[s]?://w*\\.youtube\\.com/embed/.*", Qt::CaseInsensitive);
}

video* video_youtube::createNewInstance()
{
    return new video_youtube();
}

bool video_youtube::setUrl(QString url)
{
    _originalUrl = url;

    if (_url.isEmpty())
    {
        url.replace("#!", "?");
        url.replace("youtube.com/embed/", "youtube.com/watch?v=");
        this->_url = QUrl(url);
        if (_url.isValid())
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

QString video_youtube::getFmtLink(QStringList qualityLinks, QString fmt)
{
    QRegExp urlExpression;
    urlExpression = QRegExp("(http[s]?[^,]+)");

    QRegExp itagExpression;
    itagExpression = QRegExp("[,]?itag=([^,]+)");

    for (int i=0; i < qualityLinks.size(); i++)
    {
       bool urlExpressionMatch = (urlExpression.indexIn(qualityLinks.at(i)) > -1);
       bool itagExpressionMatch = (itagExpression.indexIn(qualityLinks.at(i)) > -1 );

       if (urlExpressionMatch && itagExpressionMatch && itagExpression.cap(1) == fmt)
       {
           return qualityLinks.at(i);
       }
    }
    return "";
}

QString video_youtube::getUrlFromFmtLink(QString link)
{
    QRegExp urlExpression;
    urlExpression = QRegExp("(http[s]?[^,]+)");

    if (urlExpression.indexIn(link) > -1)
    {
        QString url = QUrl::fromEncoded(QUrl::fromEncoded(urlExpression.cap(1).toAscii()).toString().toAscii()).toString();

        QRegExp sigExpression;
        sigExpression = QRegExp("(?:^|[^a-zA-Z])[,]?s(ig)?=([^,]+)");

        if (sigExpression.indexIn(link) > -1)
        {
            QString signature;
            if (sigExpression.cap(1) == "ig")
            {
                signature = sigExpression.cap(2);
            }
            else
            {
                signature = parseSignature(sigExpression.cap(2));
            }

            url = url.append("&signature=").append(signature);

        }

        return url;

    }

    return "";
}

void video_youtube::parseJS(QString js)
{
    QRegExp expression("\\(function\\(\\)\\{(.*)\\}");
    if (expression.indexIn(js) != -1)
    {
        js = expression.cap(1);
        expression = QRegExp("signature=(.+)\\(");
        expression.setMinimal(true);
        if (expression.indexIn(js) != -1)
        {
            js.replace("function " + expression.cap(1), "function parseSignature");
            this->js = js;
            return;
        }
        expression = QRegExp("set\\(\"signature\",(.+)\\(");
        expression.setMinimal(true);
        if (expression.indexIn(js) != -1)
        {
            js.replace("function " + expression.cap(1), "function parseSignature");
            this->js = js;
        }
    }
}

void video_youtube::parseVideo(QString html)
{
    QSettings settings;
    QRegExp expression;

    //What is being downloaded?
    if (this->downloading.isEmpty() || this->downloading == "html")
    {
        this->html = html;

        //Is the access to the video restricted to authenticated users?
        if (html.contains("age-gate-content"))
        {
            expression = QRegExp("<div id=\"watch7-player-age-gate-content\">.*<button.*href=\"([^\"]+)\"");
            expression.setMinimal(true);
            if (expression.indexIn(html) != -1)
            {
                dui = new Ui::YoutubePassword();
                passwordDialog = new QDialog;
                dui->setupUi(passwordDialog);
                connect(dui->youtubePasswordWebView, SIGNAL(urlChanged(QUrl)), this, SLOT(handleLogin(QUrl)));
                dui->youtubePasswordWebView->setUrl(QUrl::fromUserInput(expression.cap(1).replace("&amp;", "&")));

                if (passwordDialog->exec() == QDialog::Accepted)
                {
                    handler->addDownload(this->_url.toString());
                }
                return;
            }
        }

        //Is signature parsing required?
        expression = QRegExp("\"url_encoded_fmt_stream_map\": ?\"[^\"]*([^a-z])(s=)");
        expression.setMinimal(true);
        if (this->js.isEmpty() && expression.indexIn(html) != -1)
        {
            this->html = html;
            QString html5PlayerUrl;

            expression = QRegExp("html5player.+\\.js");
            expression.setMinimal(true);
            if (expression.indexIn(html) !=-1)
            {
                html5PlayerUrl = "http://s.ytimg.com/yts/jsbin/" + expression.cap(0).replace("\\/", "/");
            }

            if (html5PlayerUrl.isEmpty())
            {
                expression = QRegExp("//[^\"]*html5player[^\"]*\\.js");
                expression.setMinimal(true);
                if (expression.indexIn(html) !=-1)
                {
                    html5PlayerUrl = "https:" + expression.cap(0);
                }
            }

            if (html5PlayerUrl.isEmpty())
            {
                expression = QRegExp("//[^\"]*player[^\"]*\\/base\\.js");
                expression.setMinimal(true);
                if (expression.indexIn(html) !=-1)
                {
                    //html5PlayerUrl = "https:" + expression.cap(0);
                    //todo: retry analyzing automatically when only this incompatible version was found
                    emit error("Incompatible video page detected. This might be a temporary problem, please try again.");
                    return;
                }
            }

            if (!html5PlayerUrl.isEmpty())
            {
                qDebug() << "Using player URL: " << html5PlayerUrl;
                this->downloading = "js";
                handler->addDownload(html5PlayerUrl);
                return;
            }
            else
            {
                qDebug() << "Could not retrieve player URL";
            }
        }
    }
    else if (this->downloading == "js")
    {
        parseJS(html);
    }

    //Continue parsing the html
    if (!this->html.isEmpty())
    {

        html = this->html;
    }



    expression = QRegExp("<meta name=\"title\" content=\"(.*)\"");
    expression.setMinimal(true);
    if (expression.indexIn(html) !=-1)
    {
        _title = QString(expression.cap(1)).replace("&amp;quot;", "\"").replace("&amp;amp;", "&").replace("&#39;", "'").replace("&quot;", "\"").replace("&amp;", "&");
        QStringList qualityLinks;


        expression = QRegExp("\"adaptive_fmts\": ?\"(.*)\"");
        expression.setMinimal(true);
        if (expression.indexIn(html)!=-1 && expression.cap(1) != "")
        {
            qualityLinks << expression.cap(1).split(",");
        }

        expression = QRegExp("\"url_encoded_fmt_stream_map\": ?\"(.*)\"");
        expression.setMinimal(true);

        if (expression.indexIn(html)!=-1 && expression.cap(1) != "")
        {
            qualityLinks << expression.cap(1).split(",");
        }

        if (!qualityLinks.isEmpty())
        {

            qualityLinks.replaceInStrings("\\u0026", ",");


            QList<fmtQuality> fmtQualities;


            if (settings.value("UseWebM", false).toBool() == false)
            {
                fmtQualities << fmtQuality("144p", "160", "139");

                fmtQualities << fmtQuality("240p", "5");
                fmtQualities << fmtQuality("240p", "133", "139");

                fmtQualities << fmtQuality("360p", "34");
                fmtQualities << fmtQuality("360p", "18");
                fmtQualities << fmtQuality("360p", "134", "140");

                fmtQualities << fmtQuality("480p", "35");
                fmtQualities << fmtQuality("480p", "135", "140");

                fmtQualities << fmtQuality("HD (720p)", "22");
                fmtQualities << fmtQuality("HD (720p)", "136", "141");

                fmtQualities << fmtQuality("HD (1080p)", "37");
                fmtQualities << fmtQuality("HD (1080p)", "137", "141");

                fmtQualities << fmtQuality("HD (1440p)", "264", "141");

                fmtQualities << fmtQuality("4K (2160p)", "38");
                fmtQualities << fmtQuality("4K (2160p)", "138", "141");
                fmtQualities << fmtQuality("4K (2160p)", "266", "141");

                fmtQualities << fmtQuality("HD (1080p60)", "299", "141");


            }
            else
            {
                fmtQualities << fmtQuality("360p", "43");
                fmtQualities << fmtQuality("360p", "167", "172");

                fmtQualities << fmtQuality("480p", "44");
                fmtQualities << fmtQuality("480p", "168", "172");

                fmtQualities << fmtQuality("HD (720p)", "45");
                fmtQualities << fmtQuality("HD (720p)", "169", "172");
                fmtQualities << fmtQuality("HD (720p)", "247", "172");

                fmtQualities << fmtQuality("HD (1080p)", "46");
                fmtQualities << fmtQuality("HD (1080p)", "170", "172");

                fmtQualities << fmtQuality("HD (1440p)", "271", "172");

                fmtQualities << fmtQuality("4K (2160p)", "272", "172");
                fmtQualities << fmtQuality("4K (2160p)", "313", "172");

                fmtQualities << fmtQuality("HD (1080p60)", "303", "172");
                fmtQualities << fmtQuality("HD (1440p60)", "308", "172");
                fmtQualities << fmtQuality("4K (2160p60)", "315", "172");

            }

			QMutableListIterator<fmtQuality> i(fmtQualities);
			i.toBack();
			while (i.hasPrevious())
            {
				fmtQuality q = i.previous();

                QString videoLink = getFmtLink(qualityLinks, q.video);
                QString audioLink;

                if (!videoLink.isEmpty() && !q.audio.isEmpty())
                {
                    audioLink = getFmtLink(qualityLinks, q.audio);
                    if (audioLink.isEmpty() && q.audio == "141")
                    {
                        audioLink = getFmtLink(qualityLinks, "140");
                    }
                    else if (audioLink.isEmpty() && q.audio == "172")
                    {
                        audioLink = getFmtLink(qualityLinks, "171");
                    }
                }

                if ((!videoLink.isEmpty()) && (audioLink.isEmpty() == q.audio.isEmpty()))
                {
                    if (!(!audioLink.isEmpty() && settings.value("DashSupported", false).toBool() == false))
                    {
                        qDebug() << q.video << q.audio << q.quality;
                        videoQuality newQuality;

                        QRegExp formatExpression;
                        formatExpression = QRegExp("mp4|webm|flv");
                        if (formatExpression.indexIn(videoLink) > -1)
                        {
                            newQuality.containerName = "." + formatExpression.cap(0);

                        }
                        else
                        {
                            newQuality.containerName = ".mp4";

                        }


                        newQuality.quality = q.quality;
                        newQuality.videoUrl = getUrlFromFmtLink(videoLink);

                        if (!audioLink.isEmpty())
                        {
                            newQuality.audioUrl = getUrlFromFmtLink(audioLink);
                        }


                        if (settings.value("UseChunkedDownload", true).toBool())
                        {
                            newQuality.chunkedDownload = true;
                        }
                        else
                        {
                            newQuality.chunkedDownload = false;
                        }
                        _supportedQualities.append(newQuality);

                        QMutableListIterator<fmtQuality> ii(fmtQualities);
                        while (ii.hasNext())
                        {
                            if (ii.next().quality == q.quality)
                            {
                                ii.value().video.clear();
                                ii.value().audio.clear();
                            }
                        }
                    }
                }
            }
        }
        else if (expression.indexIn(html)!=-1 && expression.cap(1) == "")
        {
            expression = QRegExp("\"t\": \"(.*)\"");
            expression.setMinimal(true);
            QRegExp expression2 = QRegExp("\"video_id\": \"(.*)\"");
            expression2.setMinimal(true);
            if (expression.indexIn(html) !=-1 && expression2.indexIn(html) !=-1)
            {
                videoQuality newQuality;
                newQuality.quality = tr("normal");
                newQuality.videoUrl = QUrl::fromEncoded(QString("http://www.youtube.com/get_video?video_id=" + expression2.cap(1) + "&t=" + expression.cap(1)).toAscii()).toString(QUrl::None);
                _supportedQualities.append(newQuality);
            }
            else
            {
                emit error("Could not retrieve video link.", this);

            }
        }
        else
        {
            emit error("Could not retrieve video link.", this);

        }
    }
    else
    {
        emit error("Could not retrieve video title.", this);
    }

    emit analysingFinished();
}

QString video_youtube::parseSignature(QString s)
{
    qDebug() << "received signature: " << s;

    QWebView* view = new QWebView();
    view->setHtml("<script>" + this->js + "</script>");
    s = view->page()->mainFrame()->evaluateJavaScript("parseSignature(\"" + s + "\")").toString();
    qDebug() << "parsed signature: " << s;
    view->deleteLater();

    return s;
}

void video_youtube::handleLogin(QUrl url)
{
    if (url.toString() == this->_url.toString().replace("http://", "https://"))
    {
        this->handler->networkAccessManager->cookieJar()->setCookiesFromUrl(dui->youtubePasswordWebView->page()->networkAccessManager()->cookieJar()->cookiesForUrl(url), url);
        passwordDialog->accept();
    }
}

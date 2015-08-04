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



#include "video_vimeo.h"

video_vimeo::video_vimeo()
{
    this->_name = "Vimeo";
    this->_supportsTitle = true;
    this->_supportsDescription = true;
    this->_supportsThumbnail = true;
    this->_supportsSearch = true;
    this->_icon = 0;
    this->_urlRegExp << QRegExp("http[s]?://(www\\.)?vimeo\\.com/(channels/[a-z0-9]+/)?([0-9]+)", Qt::CaseInsensitive);
    _treeItem = NULL;
}

video* video_vimeo::createNewInstance()
{
    return new video_vimeo();
}


bool video_vimeo::setUrl(QString url)
{
    _originalUrl = url;

    _urlRegExp.first().indexIn(url);
    if (!_urlRegExp.first().cap(3).isEmpty())
    {
        this->_url = QUrl("http://player.vimeo.com/video/" + _urlRegExp.first().cap(3) + "/config");
        if (_url.isValid())
        {
            return true;
        }
    }
    return false;
}


void video_vimeo::parseVideo(QString json)
{
    QRegExp expression;
    expression = QRegExp("\"title\":\"(.*)\",");
    expression.setMinimal(true);
    if (expression.indexIn(json) !=-1)
    {
        _title = QString(expression.cap(1)).replace("\\\"", "\"");

        expression = QRegExp("\"h264\":\\{.*\"hd\":\\{.*\"url\":\"([^\"]+)\"");
        expression.setMinimal(true);
        if (expression.indexIn(json) && !expression.cap(1).isEmpty())
        {
            videoQuality newQuality;
            newQuality.quality = tr("HD");
            newQuality.videoUrl = expression.cap(1);
            newQuality.containerName = ".mp4";
            _supportedQualities.append(newQuality);
        }

        expression = QRegExp("\"h264\":\\{.*\"sd\":\\{.*\"url\":\"([^\"]+)\"");
        expression.setMinimal(true);

        if (expression.indexIn(json) && !expression.cap(1).isEmpty())
        {
            videoQuality newQuality;
            newQuality.quality = tr("high");
            newQuality.videoUrl = expression.cap(1);
            newQuality.containerName = ".mp4";
            _supportedQualities.append(newQuality);
        }

        expression = QRegExp("\"h264\":\\{.*\"mobile\":\\{.*\"url\":\"([^\"]+)\"");
        expression.setMinimal(true);
        if (expression.indexIn(json) && !expression.cap(1).isEmpty())
        {
            videoQuality newQuality;
            newQuality.quality = tr("low");
            newQuality.videoUrl = expression.cap(1);
            newQuality.containerName = ".mp4";
            _supportedQualities.append(newQuality);
        }


        if (_supportedQualities.isEmpty())
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

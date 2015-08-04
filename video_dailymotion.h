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



#ifndef VIDEO_DAILYMOTION_H
#define VIDEO_DAILYMOTION_H
#include "video.h"
#include "QtWebKit"

struct dailymotion_quality
{
        dailymotion_quality(QString key, QString name)
        {
            this->key = key;
            this->name = name;
        }

        QString key;
        QString name;
};

class video_dailymotion : public video
{
    Q_OBJECT

    public:
        video_dailymotion();
        video* createNewInstance();
        QString getQualityUrl(QString json, QString quality);
        bool setUrl(QString);

    public slots:
        virtual void parseVideo(QString html);
};

#endif // VIDEO_DAILYMOTION_H

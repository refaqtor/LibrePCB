/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QDomDocument>
#include <QDomElement>
#include "xmlfile.h"
#include "../common/exceptions.h"
#include "../common/filepath.h"

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

XmlFile::XmlFile(const FilePath& filepath, bool restore,
                 const QString& rootName) throw (Exception) :
    QObject(0), mFilepath(filepath), mDomDocument()
{
    mDomDocument.implementation().setInvalidDataPolicy(QDomImplementation::ReturnNullNode);

    // decide if we open the original file (*.xml) or the backup (*.xml~)
    FilePath xmlFilepath(mFilepath.toStr() % '~');
    if ((!restore) || (!xmlFilepath.isExistingFile()))
        xmlFilepath = mFilepath;

    // check if the file exists
    if (!xmlFilepath.isExistingFile())
    {
        throw RuntimeError(__FILE__, __LINE__, xmlFilepath.toStr(),
            QString(tr("The file \"%1\" does not exist!")).arg(xmlFilepath.toNative()));
    }

    // try opening the file
    QFile file(xmlFilepath.toStr());
    if (!file.open(QIODevice::ReadOnly))
    {
        throw RuntimeError(__FILE__, __LINE__, xmlFilepath.toStr(), QString(tr("Cannot "
            "open file \"%1\": %2")).arg(xmlFilepath.toNative(), file.errorString()));
    }

    // load XML into mDomDocument
    QString errMsg;
    int errLine;
    int errColumn;
    if (!mDomDocument.setContent(&file, &errMsg, &errLine, &errColumn))
    {
        file.reset();
        QString line = file.readAll().split('\n').at(errLine-1);
        throw RuntimeError(__FILE__, __LINE__, QString("%1: %2 [%3:%4] LINE:%5")
            .arg(xmlFilepath.toStr(), errMsg).arg(errLine).arg(errColumn).arg(line),
            QString(tr("Error while parsing XML in file \"%1\": %2 [%3:%4]"))
            .arg(xmlFilepath.toNative(), errMsg).arg(errLine).arg(errColumn));
    }

    // check if the root node exists
    mDomRoot = mDomDocument.documentElement();
    if (mDomRoot.isNull())
    {
        throw RuntimeError(__FILE__, __LINE__, xmlFilepath.toStr(),
            QString(tr("No XML root node found in \"%1\"!")).arg(xmlFilepath.toNative()));
    }

    // check the name of the root node, if desired
    if ((!rootName.isEmpty()) && (mDomRoot.tagName() != rootName))
    {
        throw RuntimeError(__FILE__, __LINE__, QString("%1: \"%2\"!=\"%3\"")
            .arg(xmlFilepath.toStr(), mDomRoot.nodeName(), rootName),
            QString(tr("Invalid root node in \"%1\"!")).arg(xmlFilepath.toNative()));
    }
}

XmlFile::~XmlFile()
{
    // remove temporary file
    QFile::remove(mFilepath.toStr() % "~");
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void XmlFile::save(bool toOriginal) throw (Exception)
{
    QString filepath = toOriginal ? mFilepath.toStr() : mFilepath.toStr() % '~';
    saveDomDocument(mDomDocument, FilePath(filepath));
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

void XmlFile::create(const FilePath& filepath, const QString& rootName) throw (Exception)
{
    QString xmlTmpl("<?xml version='1.0' encoding='UTF-8' standalone='yes'?>\n<%1/>");

    QDomDocument dom;
    QString errMsg;
    if (!dom.setContent(xmlTmpl.arg(rootName), &errMsg))
        throw LogicError(__FILE__, __LINE__, errMsg, tr("Could not set XML DOM content!"));

    saveDomDocument(dom, filepath);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void XmlFile::saveDomDocument(const QDomDocument& domDocument,
                              const FilePath& filepath) throw (Exception)
{
    if (!filepath.getParentDir().mkPath())
        qWarning() << "could not make path for file" << filepath.toNative();

    QFile file(filepath.toStr());
    if (!file.open(QIODevice::WriteOnly))
    {
        throw RuntimeError(__FILE__, __LINE__, QString("%1: %2 [%3]")
            .arg(filepath.toStr(), file.errorString()).arg(file.error()),
            QString(tr("Could not open or create file \"%1\": %2"))
            .arg(filepath.toNative(), file.errorString()));
    }

    QByteArray content = domDocument.toByteArray(4);
    if (content.isEmpty())
    {
        throw LogicError(__FILE__, __LINE__, filepath.toStr(),
                         tr("XML DOM Document is empty!"));
    }

    qint64 written = file.write(content);
    if (written != content.size())
    {
        throw RuntimeError(__FILE__, __LINE__,
            QString("%1: %2 (only %3 of %4 bytes written)")
            .arg(filepath.toStr(), file.errorString()).arg(written).arg(content.size()),
            QString(tr("Could not write to file \"%1\": %2"))
            .arg(filepath.toNative(), file.errorString()));
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
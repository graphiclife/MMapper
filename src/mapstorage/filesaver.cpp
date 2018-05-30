/************************************************************************
**
** Authors:   Thomas Equeter <waba@waba.be>
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

#ifndef WIN32
#define UNIX_SAFETY 1
#endif

#ifdef UNIX_SAFETY
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#endif

#include "mapstorage/filesaver.h"
#include <stdexcept>

namespace {
const char *c_suffix = ".tmp";

void throw_sys_error()
{
#ifdef UNIX_SAFETY
    char buf[1024] = "";
#if _GNU_SOURCE
    char const *str = strerror_r(errno, buf, sizeof(buf));
    throw std::runtime_error(str);
#else
    strerror_r(errno, buf, sizeof(buf));
    throw std::runtime_error(buf);
#endif
#else
    return;
#endif
}

} // namespace

FileSaver::FileSaver() = default;

FileSaver::~FileSaver()
{
    try {
        close();
    } catch (...) {
    }
}

void FileSaver::open(const QString &filename)
{
    close();

    m_filename = filename;

#ifdef UNIX_SAFETY
    m_file.setFileName(filename + c_suffix);
#else
    m_file.setFileName(filename);
#endif

    if (!m_file.open(QFile::WriteOnly)) {
        throw std::runtime_error(m_file.errorString().toStdString());
    }
}

void FileSaver::close()
{
    if (!m_file.isOpen()) {
        return;
    }

    m_file.flush();

#ifdef UNIX_SAFETY
    if (fsync(m_file.handle()) == -1) {
        throw_sys_error();
    }

    if (rename(QFile::encodeName(m_filename + c_suffix).data(), QFile::encodeName(m_filename).data())
        == -1) {
        throw_sys_error();
    }
#endif

    m_file.close();
}

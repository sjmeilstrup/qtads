/* Copyright (C) 2013 Nikos Chantziaras.
 *
 * This file is part of the QTads program.  This program is free software; you
 * can redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation; either version
 * 2, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; see the file COPYING.  If not, write to the Free Software
 * Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef QTADSTIMER_H
#define QTADSTIMER_H

#include <QTimer>

#include "htmlsys.h"


/* Tads HTML layer class whose interface needs to be implemented by the
 * interpreter.
 *
 * See htmltads/htmlsys.h and htmltads/notes/porting.htm for information
 * about this class.
 */
class QTadsTimer: public QTimer, public CHtmlSysTimer {
    Q_OBJECT

  public slots:
    // We connect the timeout() signal to this slot.
    void
    trigger()
    {
        // If we have a callback, call it.
        if (this->func_ != 0) {
            this->invoke_callback();
        }
    }

  public:
    QTadsTimer( void (*func)(void*), void* ctx, QObject* parent = 0 )
    : QTimer(parent), CHtmlSysTimer(func, ctx)
    {
        connect(this, SIGNAL(timeout()), this, SLOT(trigger()));
    }

    // We bring this into public scope since we need to evaluate the callback
    // pointer in order to unregister the timer.
    using CHtmlSysTimer::func_;
};


#endif

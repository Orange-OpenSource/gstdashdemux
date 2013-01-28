/*
 * glibcompat.h
 *
 * Copyright (C) 2013 Douglas Gore <doug@ssonic.co.uk>
 *
 * Authors:
 *   Douglas Gore <doug@ssonic.co.uk>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef GLIBCOMPAT_H_
#define GLIBCOMPAT_H_

/* The GLib threading API changed in 2.32, these macros are used to
 * provide backwards compatibility with older versions without
 * complicating the code. */

#include <glib.h>

#if (GLIB_CHECK_VERSION (2, 32, 0))
    #define G_MUTEX GMutex
    #define G_MUTEX_INIT(mutex) g_mutex_init(&mutex)
    #define G_MUTEX_CLEAR(mutex) g_mutex_clear (&mutex);
    #define G_MUTEX_LOCK(mutex) g_mutex_lock (&mutex)
    #define G_MUTEX_UNLOCK(mutex) g_mutex_unlock (&mutex)
#else
    #define G_MUTEX GMutex*
    #define G_MUTEX_INIT(mutex) mutex = g_mutex_new()
    #define G_MUTEX_CLEAR(mutex) g_mutex_free (mutex)
    #define G_MUTEX_LOCK(mutex) g_mutex_lock (mutex)
    #define G_MUTEX_UNLOCK(mutex) g_mutex_unlock (mutex)
#endif

#endif /* GLIBCOMPAT_H_ */

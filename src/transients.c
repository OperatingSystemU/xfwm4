/*
        This program is free software; you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation; either version 2, or (at your option)
        any later version.
 
        This program is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.
 
        You should have received a copy of the GNU General Public License
        along with this program; if not, write to the Free Software
        Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 
        xfwm4    - (c) 2002-2004 Olivier Fourdan
 
 */
 
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <libxfce4util/libxfce4util.h> 

#include "screen.h"
#include "client.h"
#include "stacking.h"

Client *
clientGetTransient (Client * c)
{
    Client *c2 = NULL;

    g_return_val_if_fail (c != NULL, NULL);

    TRACE ("entering clientGetTransient");

    if ((c->transient_for) && (c->transient_for != c->screen_info->xroot))
    {
        c2 = clientGetFromWindow (c->screen_info, c->transient_for, WINDOW);
        return c2;
    }
    return NULL;
}

gboolean
clientIsTransient (Client * c)
{
    g_return_val_if_fail (c != NULL, FALSE);

    TRACE ("entering clientIsTransient");

    return (((c->transient_for != c->screen_info->xroot) && (c->transient_for != None) && (c->transient_for != c->window)) || 
            ((c->transient_for == c->screen_info->xroot) && (c->group_leader != None) && (c->group_leader != c->window)));
}

gboolean
clientIsModal (Client * c)
{
    g_return_val_if_fail (c != NULL, FALSE);

    TRACE ("entering clientIsModal");

    return (FLAG_TEST (c->flags, CLIENT_FLAG_STATE_MODAL) && 
            (((c->transient_for != c->screen_info->xroot) && (c->transient_for != None) && (c->transient_for != c->window)) ||
             ((c->group_leader != None) && (c->group_leader != c->window))));
}

gboolean
clientIsTransientOrModal (Client * c)
{
    g_return_val_if_fail (c != NULL, FALSE);

    TRACE ("entering clientIsTransientOrModal");

    return (clientIsTransient(c) || clientIsModal(c));
}

gboolean
clientSameGroup (Client * c1, Client * c2)
{
    g_return_val_if_fail (c1 != NULL, FALSE);
    g_return_val_if_fail (c2 != NULL, FALSE);

    TRACE ("entering clientSameGroup");

    return ((c1 != c2) && 
            (((c1->group_leader != None) && 
              (c1->group_leader == c2->group_leader)) ||
             (c1->group_leader == c2->window) ||
             (c2->group_leader == c1->window)));
}

gboolean
clientIsTransientFor (Client * c1, Client * c2)
{
    g_return_val_if_fail (c1 != NULL, FALSE);
    g_return_val_if_fail (c2 != NULL, FALSE);

    TRACE ("entering clientIsTransientFor");

    if (c1->transient_for)
    {
        if (c1->transient_for != c1->screen_info->xroot)
        {
            if (c1->transient_for == c2->window)
            return (c1->transient_for == c2->window);
        }
        else if (c1->serial >= c2->serial)
        {
            return (clientSameGroup (c1, c2));
        }
    }
    return FALSE;
}

gboolean
clientIsModalFor (Client * c1, Client * c2)
{
    g_return_val_if_fail (c1 != NULL, FALSE);
    g_return_val_if_fail (c2 != NULL, FALSE);

    TRACE ("entering clientIsModalFor");

    if (FLAG_TEST (c1->flags, CLIENT_FLAG_STATE_MODAL) && (c1->serial >= c2->serial))
    {
        return (clientIsTransientFor (c1, c2) || clientSameGroup (c1, c2));
    }
    return FALSE;
}

gboolean
clientIsTransientOrModalFor (Client * c1, Client * c2)
{
    g_return_val_if_fail (c1 != NULL, FALSE);
    g_return_val_if_fail (c2 != NULL, FALSE);

    TRACE ("entering clientIsTransientOrModalFor");

    return (clientIsTransientFor(c1, c2) || clientIsModalFor(c1, c2));
}

gboolean
clientIsTransientForGroup (Client * c)
{
    g_return_val_if_fail (c != NULL, FALSE);

    TRACE ("entering clientIsTransientForGroup");

    return ((c->transient_for == c->screen_info->xroot) && (c->group_leader != None));
}

gboolean
clientIsModalForGroup (Client * c)
{
    g_return_val_if_fail (c != NULL, FALSE);

    TRACE ("entering clientIsModalForGroup");

    return (FLAG_TEST (c->flags, CLIENT_FLAG_STATE_MODAL) && 
            !clientIsTransient(c) && (c->group_leader != None));
}

gboolean
clientIsTransientOrModalForGroup (Client * c)
{
    g_return_val_if_fail (c != NULL, FALSE);

    TRACE ("entering clientIsTransientOrModalForGroup");

    return (clientIsTransientForGroup(c) || clientIsModalForGroup(c));
}

Client *
clientGetModalFor (Client * c)
{
    Client *latest_modal = NULL;
    Client *c2, *c3;
    ScreenInfo *screen_info;
    GList *modals = NULL;
    GList *index1, *index2;

    g_return_val_if_fail (c != NULL, NULL);
    TRACE ("entering clientGetModalFor");
    
    screen_info = c->screen_info;
    for (index1 = screen_info->windows_stack; index1; index1 = g_list_next (index1))
    {
        c2 = (Client *) index1->data;
        if (c2)
        {
            if ((c2 != c) && clientIsModalFor (c2, c))
            {
                modals = g_list_append (modals, c2);
                latest_modal = c2;
            }
            else
            {
                for (index2 = modals; index2;
                    index2 = g_list_next (index2))
                {
                    c3 = (Client *) index2->data;
                    if ((c3 != c2) && clientIsModalFor (c2, c3))
                    {
                        modals = g_list_append (modals, c2);
                        latest_modal = c2;
                        break;
                    }
                }
            }
        }
    }
    if (modals)
    {
        g_list_free (modals);
    }

    return latest_modal;
}

/* Build a GList of clients that have a transient relationship */
GList *
clientListTransient (Client * c)
{
    ScreenInfo *screen_info;
    GList *transients = NULL;
    GList *index1, *index2;
    Client *c2, *c3;

    g_return_val_if_fail (c != NULL, NULL);

    screen_info = c->screen_info;
    transients = g_list_append (transients, c);
    for (index1 = screen_info->windows_stack; index1; index1 = g_list_next (index1))
    {
        c2 = (Client *) index1->data;
        if (c2 != c)
        {
            if (clientIsTransientFor (c2, c))
            {
                transients = g_list_append (transients, c2);
            }
            else
            {
                for (index2 = transients; index2;
                    index2 = g_list_next (index2))
                {
                    c3 = (Client *) index2->data;
                    if ((c3 != c2) && clientIsTransientFor (c2, c3))
                    {
                        transients = g_list_append (transients, c2);
                        break;
                    }
                }
            }
        }
    }
    return transients;
}

/* Build a GList of clients that have a transient or modal relationship */
GList *
clientListTransientOrModal (Client * c)
{
    ScreenInfo *screen_info;
    GList *transients = NULL;
    GList *index1, *index2;
    Client *c2, *c3;

    g_return_val_if_fail (c != NULL, NULL);

    screen_info = c->screen_info;
    transients = g_list_append (transients, c);
    for (index1 = screen_info->windows_stack; index1; index1 = g_list_next (index1))
    {
        c2 = (Client *) index1->data;
        if (c2 != c)
        {
            if (clientIsTransientOrModalFor (c2, c))
            {
                transients = g_list_append (transients, c2);
            }
            else
            {
                for (index2 = transients; index2;
                    index2 = g_list_next (index2))
                {
                    c3 = (Client *) index2->data;
                    if ((c3 != c2) && clientIsTransientOrModalFor (c2, c3))
                    {
                        transients = g_list_append (transients, c2);
                        break;
                    }
                }
            }
        }
    }
    return transients;
}

/* Check if a window is not already listed in transients of a client.
   That's to avoid potential self transient relationship...
 */
gboolean
clientCheckTransientWindow (Client *c, Window w)
{
    GList *transients = NULL;
    GList *index;
    Client *c2;

    g_return_val_if_fail (c != NULL, FALSE);

    transients = clientListTransient (c);
    for (index = transients; index; index = g_list_next (index))
    {
        c2 = (Client *) index->data;
        if (c2->window == w)
        {
            g_list_free (transients);
            return FALSE;
        }
    }
    g_list_free (transients);
    return TRUE;
}

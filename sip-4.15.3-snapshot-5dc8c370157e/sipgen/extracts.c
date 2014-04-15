/*
 * The extracts generator for SIP.
 *
 * Copyright (c) 2013 Riverbank Computing Limited <info@riverbankcomputing.com>
 *
 * This file is part of SIP.
 *
 * This copy of SIP is licensed for use under the terms of the SIP License
 * Agreement.  See the file LICENSE for more details.
 *
 * This copy of SIP may also used under the terms of the GNU General Public
 * License v2 or v3 as published by the Free Software Foundation which can be
 * found in the files LICENSE-GPL2 and LICENSE-GPL3 included in this package.
 *
 * SIP is supplied WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */


#include <stdio.h>
#include <string.h>

#include "sip.h"


/*
 * Generate the extracts.
 */
void generateExtracts(sipSpec *pt, const stringList *extracts)
{
    while (extracts != NULL)
    {
        const char *cp, *id, *fname;
        size_t id_len;
        extractDef *ed;
        extractPartDef *epd;
        FILE *fp;

        /* Extract the id and file name. */
        cp = strchr(extracts->s, ':');

        if (cp == NULL || cp == extracts->s || cp[1] == '\0')
            fatal("An extract must be in the form 'id:file', not '%s'\n",
                    extracts->s);

        id = extracts->s;
        id_len = cp - extracts->s;
        fname = &cp[1];

        for (ed = pt->extracts; ed != NULL; ed = ed->next)
            if (strlen(ed->id) == id_len && strncmp(ed->id, id, id_len) == 0)
                break;

        if (ed == NULL)
            fatal("There is no extract defined with the identifier \"%.*s\"\n",
                    id_len, id);

        if ((fp = fopen(fname, "w")) == NULL)
            fatal("Unable to create file '%s'\n", fname);

        for (epd = ed->parts; epd != NULL; epd = epd->next)
            fprintf(fp, "%s", epd->part->frag);

        fclose(fp);

        extracts = extracts->next;
    }
}


/*
 * Add a new part to a (possibly new) extract.
 */
void addExtractPart(sipSpec *pt, const char *id, int order, codeBlock *part)
{
    extractDef *ed;
    extractPartDef *epd, **at;

    /* Find the extract if it exists. */
    for (ed = pt->extracts; ed != NULL; ed = ed->next)
        if (strcmp(ed->id, id) == 0)
            break;

    /* Create the extract if it doesn't already exist. */
    if (ed == NULL)
    {
        ed = sipMalloc(sizeof (extractDef));

        ed->id = id;
        ed->parts = NULL;
        ed->next = pt->extracts;

        pt->extracts = ed;
    }

    /* Find the position where the new part should be inserted. */
    for (at = &ed->parts; *at != NULL; at = &(*at)->next)
        if (order >= 0 && ((*at)->order < 0 || order < (*at)->order))
            break;

    /* Create the new part. */
    epd = sipMalloc(sizeof (extractPartDef));

    epd->order = order;
    epd->part = part;
    epd->next = *at;

    *at = epd;
}

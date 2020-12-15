/*
 *  FreeLoader - arcname.c
 *
 *  Copyright (C) 2001  Brian Palmer  <brianp@sginet.com>
 *  Copyright (C) 2001  Eric Kohl
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <freeldr.h>

BOOLEAN
DissectArcPath(
    IN  PCSTR ArcPath,
    OUT PCSTR* Path OPTIONAL,
    OUT PUCHAR DriveNumber,
    OUT PULONG PartitionNumber)
{
    PCCH p;

    /* Detect ramdisk path */
    if (_strnicmp(ArcPath, "ramdisk(0)", 10) == 0)
    {
        /* Magic value for ramdisks */
        *DriveNumber = 0x49;
        *PartitionNumber = 1;

        /* Get the path (optional) */
        if (Path)
            *Path = ArcPath + 10;

        return TRUE;
    }

    /* NOTE: We are currently limited when handling multi()disk() paths!! */
    if (_strnicmp(ArcPath, "multi(0)disk(0)", 15) != 0)
        return FALSE;

    p = ArcPath + 15;
    if (_strnicmp(p, "fdisk(", 6) == 0)
    {
        /*
         * Floppy disk path:
         *  multi(0)disk(0)fdisk(x)\path
         */
        p = p + 6;
        *DriveNumber = atoi(p);
        p = strchr(p, ')');
        if (p == NULL)
            return FALSE;
        ++p;
        *PartitionNumber = 0;
    }
    else if (_strnicmp(p, "cdrom(", 6) == 0)
    {
        /*
         * Cdrom path:
         *  multi(0)disk(0)cdrom(x)\path
         */
        p = p + 6;
        *DriveNumber = atoi(p) + 0x80;
        p = strchr(p, ')');
        if (p == NULL)
            return FALSE;
        ++p;
        *PartitionNumber = 0xff;
    }
    else if (_strnicmp(p, "rdisk(", 6) == 0)
    {
        /*
         * Hard disk path:
         *  multi(0)disk(0)rdisk(x)[partition(y)][\path]
         */
        p = p + 6;
        *DriveNumber = atoi(p) + 0x80;
        p = strchr(p, ')');
        if (p == NULL)
            return FALSE;
        ++p;
        /* The partition is optional */
        if (_strnicmp(p, "partition(", 10) == 0)
        {
            p = p + 10;
            *PartitionNumber = atoi(p);
            p = strchr(p, ')');
            if (p == NULL)
                return FALSE;
            ++p;
        }
        else
        {
            *PartitionNumber = 0;
        }
    }
    else
    {
        return FALSE;
    }

    /* Get the path (optional) */
    if (Path)
        *Path = p;

    return TRUE;
}

/* PathSyntax: scsi() = 0, multi() = 1, ramdisk() = 2 */
BOOLEAN
DissectArcPath2(
    IN  PCSTR ArcPath,
    OUT PULONG x,
    OUT PULONG y,
    OUT PULONG z,
    OUT PULONG Partition,
    OUT PULONG PathSyntax)
{
    char* next_token;
    const char* token;

    /* Detect ramdisk() */
    if (_strnicmp(ArcPath, "ramdisk(0)", 10) == 0)
    {
        *x = *y = *z = 0;
        *Partition = 1;
        *PathSyntax = 2;
        return TRUE;
    }
    /* Detect scsi()... */
    else if (_strnicmp(ArcPath, "scsi(", 5) == 0)
    {
        *PathSyntax = 0;
        token = &ArcPath[5];
    }
    /* Detect multi()... */
    else if (_strnicmp(ArcPath, "multi(", 6) == 0)
    {
        *PathSyntax = 1;
        token = &ArcPath[6];
    }
    else
    {
        return FALSE;
    }

    *x = strtoul(token, &next_token, 10);
    if (next_token == token)
        return FALSE;

    if (*next_token != ')')
        return FALSE;

    token = next_token + 1;
    /* check disk() */
    if (_strnicmp(token, "disk(", 5) == 0)
    {
        token = &token[5];

        *y = strtoul(token, &next_token, 10);

        if (next_token == token)
            return FALSE;

        if (*next_token != ')')
            return FALSE;

        token = next_token + 1;

        /* We are at
         * scsi()disk() --> *PathSyntax == 0
         * multi()disk() --> *PathSyntax == 1
         */
        if (_strnicmp(token, "rdisk(", 6) == 0)
        {
            token = &token[6];

            *z = strtoul(token, &next_token, 10);

            if (next_token == token)
                return FALSE;

            if (*next_token != ')')
                return FALSE;

            token = next_token + 1;

            /* We are at
             * scsi()disk()rdisk() --> *PathSyntax == 0
             * multi()disk()rdisk() --> *PathSyntax == 1
             */
            if (_strnicmp(token, "partition(", 10) == 0)
            {
                token = &token[10];

                *Partition = strtoul(token, &next_token, 10);

                if (next_token == token)
                    return FALSE;

                if (*next_token != ')')
                    return FALSE;

                /*
                 * scsi()disk()rdisk()partition()
                 * multi()disk()rdisk()partition()
                 * All good
                 */
                return TRUE;
            }
            else
            {
                return FALSE;
            }
        }
        else
        {
            /* only rdisk() is allowed after scsi()disk() */
            if (*PathSyntax == 0)
                return FALSE;

            /* We are at multi()disk() */
            if ((_strnicmp(token, "cdrom(", 6) == 0) ||
                (_strnicmp(token, "fdisk(", 6) == 0))
            {
                token = &token[6];

                *z = strtoul(token, &next_token, 10);

                if (next_token == token)
                    return FALSE;

                if (*next_token != ')')
                    return FALSE;

                /*
                 * multi()disk()cdrom()
                 * multi()disk()fdisk()
                 * All good
                 */
                *Partition = 1;
                return TRUE;
            }
            else
            {
                return FALSE;
            }
        }
    }

    /* Only disk() is allowed after multi() */
    if (*PathSyntax == 1)
        return FALSE;

    /* We are at scsi() scsi()disk() was handled above.
     * Only cdrom() is allowed now */
    if (_strnicmp(token, "cdrom(", 6) != 0)
        return FALSE;

    token = &token[6];

    *y = strtoul(token, &next_token, 10);
    if (next_token == token)
        return FALSE;

    if (*next_token != ')')
        return FALSE;

    token = next_token + 1;

    /* We are at scsi()cdrom()
     * Only fdisk() is allowed now */
    if (_strnicmp(token, "fdisk(", 6) != 0)
        return FALSE;

    token = &token[6];

    *z = strtoul(token, &next_token, 10);
    if (next_token == token)
        return FALSE;

    if (*next_token != ')')
        return FALSE;

    *Partition = 0;
    return TRUE;
}

VOID ConstructArcPath(PCHAR ArcPath, PCHAR SystemFolder, UCHAR Disk, ULONG Partition)
{
    char    tmp[50];

    strcpy(ArcPath, "multi(0)disk(0)");

    if (Disk < 0x80)
    {
        /*
         * Floppy disk path:
         *  multi(0)disk(0)fdisk(x)\path
         */
        sprintf(tmp, "fdisk(%d)", (int) Disk);
        strcat(ArcPath, tmp);
    }
    else
    {
        /*
         * Hard disk path:
         *  multi(0)disk(0)rdisk(x)partition(y)\path
         */
        sprintf(tmp, "rdisk(%d)partition(%d)", (int) (Disk - 0x80), (int) Partition);
        strcat(ArcPath, tmp);
    }

    if (SystemFolder[0] == '\\' || SystemFolder[0] == '/')
    {
        strcat(ArcPath, SystemFolder);
    }
    else
    {
        strcat(ArcPath, "\\");
        strcat(ArcPath, SystemFolder);
    }
}

#if 0
UCHAR ConvertArcNameToBiosDriveNumber(PCHAR ArcPath)
{
    char *    p;
    UCHAR        DriveNumber = 0;

    if (_strnicmp(ArcPath, "multi(0)disk(0)", 15) != 0)
        return 0;

    p = ArcPath + 15;
    if (_strnicmp(p, "fdisk(", 6) == 0)
    {
        /*
         * Floppy disk path:
         *  multi(0)disk(0)fdisk(x)\path
         */
        p = p + 6;
        DriveNumber = atoi(p);
    }
    else if (_strnicmp(p, "rdisk(", 6) == 0)
    {
        /*
         * Hard disk path:
         *  multi(0)disk(0)rdisk(x)partition(y)\path
         */
        p = p + 6;
        DriveNumber = atoi(p) + 0x80;
    }

    return DriveNumber;
}
#endif

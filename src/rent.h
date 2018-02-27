/****************************************************************************
 * [S]imulated [M]edieval [A]dventure multi[U]ser [G]ame      |   \\._.//   *
 * -----------------------------------------------------------|   (0...0)   *
 * SMAUG 1.4 (C) 1994, 1995, 1996, 1998  by Derek Snider      |    ).:.(    *
 * -----------------------------------------------------------|    {o o}    *
 * SMAUG code team: Thoric, Altrag, Blodkai, Narn, Haus,      |   / ' ' \   *
 * Scryn, Rennard, Swordbearer, Gorog, Grishnakh, Nivek,      |~'~.VxvxV.~'~*
 * Tricops and Fireblade                                      |             *
 * ------------------------------------------------------------------------ *
 * Merc 2.1 Diku Mud improvments copyright (C) 1992, 1993 by Michael        *
 * Chastain, Michael Quan, and Mitchell Tse.                                *
 * Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,          *
 * Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.     *
 * ------------------------------------------------------------------------ *
 *                   Rental Code header information                         *
 ****************************************************************************/

/* Minimum rent value for rare items - Samson 1-14-98 */

#define MIN_RENT 25000

/* Rent functions added by Samson - 1-14-98 */

void scan_rent args((CHAR_DATA *ch));
void rent_display args((CHAR_DATA *ch, OBJ_DATA *obj, int *rent));
void rent_leaving args((CHAR_DATA *ch, OBJ_DATA *obj, int *rent));
void rent_calculate args((CHAR_DATA *ch, OBJ_DATA *obj, int *rent));
void rent_check args((CHAR_DATA *ch, OBJ_DATA *obj));


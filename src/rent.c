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
 *                         Rental Code Module                               *
 ****************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "mud.h"

/* Calculate player rent cost based on time stamp */

void scan_rent(CHAR_DATA *ch)
{
  OBJ_DATA *tobj;
  char buf[MAX_STRING_LENGTH];
  char log_buf[MAX_STRING_LENGTH];
  int rentcost = 0;
  int cost = 0;
  struct stat fst;

  snprintf(buf, MAX_STRING_LENGTH, "%s%c/%s", PLAYER_DIR, tolower(ch->name[0]),
          capitalize(ch->name));
  if (stat(buf, &fst) != -1) {
    for (tobj = ch->first_carrying; tobj; tobj = tobj->next_content)
      rent_calculate(ch, tobj, &rentcost);
    cost = (int)((rentcost * (double) (time(0) - fst.st_mtime)) / 86400);
    if (IS_IMMORTAL(ch))
      cost = 0;
    if (ch->gold < cost) {
      for (tobj = ch->first_carrying; tobj; tobj = tobj->next_content)
        rent_check(ch,tobj);
      ch->gold = 0;
      set_char_color(AT_BLUE, ch);
      ch_printf(ch, "You ran up charges of %d in rent, but could not afford it!\r\n", cost);
      send_to_char("Your rare items have been sold to cover the debt.\r\n", ch);
      snprintf(log_buf, MAX_STRING_LENGTH, "%s ran up %d in rent costs, but ran out of money. Rare items recirculated.", ch->name, cost);
      log_string_plus(log_buf, LOG_COMM, ch->level);
    }
    else {
      ch->gold -= cost;
      if (!(IS_IMMORTAL(ch))) {
        set_char_color(AT_BLUE, ch);
        ch_printf(ch, "You ran up charges of %d in rent.\r\n", cost);
        snprintf(log_buf, MAX_STRING_LENGTH, "%s ran up %d in rent costs.", ch->name, cost);
        log_string_plus(log_buf, LOG_COMM, LEVEL_IMMORTAL);
      }
      else {
        snprintf(log_buf, MAX_STRING_LENGTH, "%s returns from beyond the void.", ch->name);
        log_string_plus(log_buf, LOG_COMM, ch->level);
      }
    }
  }
  return;
}

void do_qui(CHAR_DATA* ch, const char* argument)
{
  set_char_color(AT_RED, ch);
  send_to_char("If you want to QUIT, you have to spell it out.\r\n", ch);
  return;
}

void do_quit(CHAR_DATA* ch, const char* argument)
{
  OBJ_DATA *obj, *obj_next;
  char arg1[MAX_INPUT_LENGTH];
  char log_buf[MAX_STRING_LENGTH];
  int x, y;
  int level;

  if (IS_NPC(ch))
    return;

  argument = one_argument(argument, arg1);

  if (arg1[0] == '\0') {
    do_help(ch, "quit");
    return;
  }

  if (str_cmp(arg1, "now")) {
    do_help(ch, "quit");
    return;
  }

  if (ch->position == POS_FIGHTING
      || ch->position == POS_EVASIVE
      || ch->position == POS_DEFENSIVE
      || ch->position == POS_AGGRESSIVE
      || ch->position == POS_BERSERK)
  {
    set_char_color(AT_RED, ch);
    send_to_char("No way! You are fighting.\r\n", ch);
    return;
  }

  if (ch->position < POS_STUNNED) {
    set_char_color(AT_BLOOD, ch);
    send_to_char("You're not DEAD yet.\r\n", ch);
    return;
  }

  if (get_timer(ch, TIMER_RECENTFIGHT) > 0 && !IS_IMMORTAL(ch)) {
    set_char_color(AT_RED, ch);
    send_to_char("Your adrenaline is pumping too hard to quit now!\r\n", ch);
    return;
  }

  if (auction->item != NULL && ((ch == auction->buyer) || (ch == auction->seller))) {
    send_to_char("Wait until you have bought/sold the item on auction.\r\n", ch);
    return;
  }

  if (!IS_IMMORTAL(ch) && xIS_SET(ch->in_room->room_flags, ROOM_NOQUIT)) {
    send_to_char("You cannot QUIT here.\r\n", ch);
    return;
  }

  if (!str_cmp(arg1, "now")) {
    if (ch->furniture) {
      ch->furniture->supporting--;
      ch->furniture = NULL;
    }

    if (ch->position == POS_MOUNTED)
      do_dismount(ch, "");
    
    /* This is going to have to be fixed for realspace -- Shamus */

    if (!IS_IMMORTAL(ch)) {
      snprintf(log_buf, MAX_STRING_LENGTH, "%s has failed autorent, inventory lost.", ch->name);
      act(AT_PLAIN, "Come back soon, &G$n&w...", ch, NULL, NULL, TO_CHAR);

      /* Character failed autorent, strip him naked! -- Samson 8-10-98 */
      
      for (obj = ch->first_carrying; obj != NULL ;obj = obj_next) {
        obj_next = obj->next_content;
        if (obj->wear_loc != WEAR_NONE)
          unequip_char(ch, obj);
      }

      for (obj = ch->first_carrying; obj; obj = obj_next) {
        obj_next = obj->next_content;
        separate_obj(obj);
        obj_from_char(obj);
        if (!obj_next)
          obj_next = ch->first_carrying;
        if (ch->in_room)
          obj = obj_to_room(obj, ch->in_room);
        else
          obj = obj_to_hex(obj, ch->xhex, ch->yhex);
      }
    } else {
      snprintf(log_buf, MAX_STRING_LENGTH, "%s has autorented safely.", ch->name);
      act(AT_PLAIN, "Come back soon, &G$n&w...", ch, NULL, NULL, TO_CHAR);
    }

    quitting_char = ch;
    if (ch->level >= LEVEL_HERO && !ch->pcdata->pet) /* Pet crash fix */
      xREMOVE_BIT(ch->act, PLR_BOUGHT_PET);
    save_char_obj(ch);

    if (sysdata.save_pets && ch->pcdata->pet) {
      act(AT_BYE, "$N follows $S master into the Void.", ch, NULL, ch->pcdata->pet, TO_ROOM);
      extract_char(ch->pcdata->pet, TRUE);
    }

    saving_char = NULL;
    level = get_trust(ch);
        
    /* After extract_char the ch is no longer valid! */
        
    extract_char(ch, TRUE);
    for (x = 0; x < MAX_WEAR; x++)
      for (y = 0; y < MAX_LAYERS; y++)
        save_equipment[x][y] = NULL;
    
    log_string_plus(log_buf, LOG_COMM, level);
    return;
  }

  set_char_color(AT_WHITE, ch);
  send_to_char("You drop everything, and leave the game.\r\n", ch);
  act(AT_SAY, "Come back soon, $n...'", ch, NULL, NULL, TO_CHAR);
  act(AT_BYE, "$n has left the game.", ch, NULL, NULL, TO_ROOM);
  set_char_color(AT_GREY, ch);

  snprintf(log_buf, MAX_STRING_LENGTH, "%s has quit.", ch->name);

  /* New code added here by Samson on 1-14-98: Character is unequipped here */
  
  for (obj = ch->first_carrying; obj != NULL ; obj = obj_next) {
    obj_next = obj->next_content;
    if (obj->wear_loc != WEAR_NONE)
      unequip_char(ch, obj);
  }

  /* Player drops everything on quit */
  /* Why does this happen twice? -- Shamus */
  
  for (obj = ch->first_carrying; obj; obj = obj_next) {
    obj_next = obj->next_content;
    separate_obj(obj);
    obj_from_char(obj);
    if (!obj_next)
      obj_next = ch->first_carrying;
    if (ch->in_room)
      obj = obj_to_room(obj, ch->in_room);
    else
      obj = obj_to_hex(obj, ch->xhex, ch->yhex);
  }
     
  quitting_char = ch;
  save_char_obj(ch);
  saving_char = NULL;

  level = get_trust(ch);
  
  /* After extract_char the ch is no longer valid! */

  extract_char(ch, TRUE);
  for (x = 0; x < MAX_WEAR; x++)
    for (y = 0; y < MAX_LAYERS; y++)
      save_equipment[x][y] = NULL;
  
  log_string_plus(log_buf, LOG_COMM, level);
  return;
}

/* Checks room to see if an Innkeeper mob is present */

CHAR_DATA *find_innkeeper(CHAR_DATA *ch)
{
  CHAR_DATA *innk;
  
  for (innk = ch->in_room->first_person; innk; innk = innk->next_in_room)
    if (IS_NPC(innk) && xIS_SET(innk->act, ACT_INNKEEPER))
      break;

  return innk;
}

/* Calculates rent for rare items, no display to player.
   Used by comm.c to calculate rent cost when player logs on */

void rent_calculate(CHAR_DATA *ch, OBJ_DATA *obj, int *rent)
{
  OBJ_DATA *tobj;
  
  if (obj->pIndexData->rent >= MIN_RENT)
    *rent += obj->pIndexData->rent * obj->count;
  for (tobj = obj->first_content; tobj; tobj = tobj->next_content)
    rent_calculate(ch, tobj, rent);
}

/* Calculates rent for rare items, displays cost to player.
   Used in do_offer to tell player how much they're going to be charged */

void rent_display(CHAR_DATA *ch, OBJ_DATA *obj, int *rent)
{
  OBJ_DATA *tobj;
  if (obj->pIndexData->rent >= MIN_RENT) {
    *rent += obj->pIndexData->rent * obj->count;
    ch_printf(ch, "%s:\t%d cerons per day.\r\n", obj->short_descr,
              obj->pIndexData->rent);
    }
  for (tobj = obj->first_content; tobj; tobj = tobj->next_content)
    rent_display(ch, tobj, rent);
}

/* Removes rare items the player cannot afford to maintain.
 * Used during login */

void rent_check(CHAR_DATA *ch, OBJ_DATA *obj)
{
  OBJ_DATA *tobj;

  if (obj->pIndexData->rent >= MIN_RENT) {
    obj_from_char(obj);
    extract_obj(obj);
  }
  for (tobj = obj->first_content; tobj; tobj = tobj->next_content)
    rent_check(ch, tobj);
}

/*
 * Calculates rent for rare items, displays cost to player
 * removes -1 rent items from player's inventory.
 * Used by do_rent when player is renting from the game.
 */

void rent_leaving(CHAR_DATA *ch, OBJ_DATA *obj, int *rent)
{
  OBJ_DATA *tobj;
  
  if (obj->pIndexData->rent >= MIN_RENT) {
    *rent += obj->pIndexData->rent * obj->count;
    ch_printf(ch, "%s:\t%d cerons per day.\r\n", obj->short_descr, obj->pIndexData->rent);
  }
  if (obj->pIndexData->rent == -1) {
    if (obj->wear_loc != WEAR_NONE)
      unequip_char(ch, obj);
    separate_obj(obj);
    obj_from_char(obj);
    ch_printf(ch, "%s disappears in a cloud of smoke!\r\n", obj->short_descr);
    extract_obj(obj);
  }
  for (tobj = obj->first_content; tobj; tobj = tobj->next_content)
    rent_leaving(ch, tobj, rent);
}

/* Tell player how much rent they will be charged */

void do_offer(CHAR_DATA* ch, const char* argument)
{
  OBJ_DATA *obj;
  CHAR_DATA *innkeeper;
  CHAR_DATA *victim;
  char buf [MAX_STRING_LENGTH];
  int rentcost;
    
  if (!(innkeeper = find_innkeeper(ch))) {
    send_to_char("You can only offer at an inn.\r\n", ch);
    return;
  }
    
  victim = innkeeper;
    
  if (IS_NPC(ch)) {
    send_to_char("Mobs can't offer!\r\n", ch);
    return;
  }

  rentcost = 0;
  act(AT_SOCIAL, "$n takes a look at your items.", victim, NULL, ch, TO_VICT);
  set_char_color(AT_GREEN, ch);
  for (obj = ch->first_carrying; obj; obj = obj->next_content)
    rent_display(ch, obj, &rentcost);
  if (rentcost) {
    snprintf(buf, MAX_STRING_LENGTH, "%s Your rent will cost you %d ceron%s per day.",
	     ch->name, rentcost, (rentcost == 1) ? "" : "s");
  }
  else {
    snprintf(buf, MAX_STRING_LENGTH, "%s Enjoy your stay with us. There will be no charges.", ch->name);
  }
  do_tell(innkeeper, buf);

  if (IS_IMMORTAL(ch)) {
    if (rentcost) {
      snprintf(buf, MAX_STRING_LENGTH, "%s But for you, %s, there is no charge.", ch->name, ch->name);
      rentcost = 0;
    }
    else {
      snprintf(buf, MAX_STRING_LENGTH, "%s Of course, you'd stay for free anyway.", ch->name);
    }
    do_tell(innkeeper, buf);
  }
  return;
}

/* Modified do_quit function, calculates rent cost & saves player data */

void do_rent(CHAR_DATA* ch, const char* argument)
{
  OBJ_DATA *obj;
  CHAR_DATA *innkeeper;
  CHAR_DATA *victim;
  char buf [MAX_STRING_LENGTH];
  char log_buf[MAX_STRING_LENGTH];
  int x, y;
  int level;
  int rentcost;
    
  if (!(innkeeper = find_innkeeper(ch))) {
    send_to_char("You can only rent at an inn.\r\n", ch);
    return;
  }

  victim = innkeeper;
    
  /* Rent cost calculation */
  
  rentcost = 0;
  act(AT_SOCIAL, "$n takes a look at your items.", victim, NULL, ch, TO_VICT);
  set_char_color(AT_GREEN, ch);
  for (obj = ch->first_carrying; obj; obj = obj->next_content)
    rent_leaving(ch, obj, &rentcost);

  if (rentcost)
    snprintf(buf, MAX_STRING_LENGTH, "%s Your rent will cost you %d ceron%s per day.",
            ch->name, rentcost, (rentcost == 1) ? "" : "s");
  else
    snprintf(buf, MAX_STRING_LENGTH, "%s There is no charge for your rent.", ch->name);
  do_tell(innkeeper, buf);
    
  if (IS_IMMORTAL(ch)) {
    if (rentcost)
      snprintf(buf, MAX_STRING_LENGTH, "%s But for you, %s, there is no charge.",
              ch->name, ch->name);
    else
      snprintf(buf, MAX_STRING_LENGTH, "%s Of course, you'd stay for free anyway.", ch->name);
    do_tell(innkeeper, buf);
    rentcost = 0;
  }

  if (ch->gold < rentcost) {
    act(AT_SAY, "$n says 'You cannot afford this much!'", victim, NULL, ch, TO_VICT);
    return;
  }
  act(AT_GREY, "$n takes your equipment into storage, and shows you to your room.", victim, NULL, ch, TO_VICT);
  act(AT_BYE, "$n shows $N to $S room, and stores $S equipment.", victim, NULL, ch, TO_NOTVICT);
  set_char_color(AT_GREY, ch);

  if (ch->level < 2) {
    ch->level = 2;
  }

  snprintf(log_buf, MAX_STRING_LENGTH, "%s has rented.", ch->name);
  quitting_char = ch;

  save_char_obj(ch);
  saving_char = NULL;

  level = get_trust(ch);

  /*
   * After extract_char the ch is no longer valid!
   */
  
  extract_char(ch, TRUE);
  for (x = 0; x < MAX_WEAR; x++)
    for (y = 0; y < MAX_LAYERS; y++)
      save_equipment[x][y] = NULL;

  /* don't show who's logging off to leaving player */

  log_string_plus(log_buf, LOG_COMM, level);
  return;
}

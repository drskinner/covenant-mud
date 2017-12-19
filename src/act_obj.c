/****************************************************************************
 * [S]imulated [M]edieval [A]dventure multi[U]ser [G]ame      |   \\._.//   *
 * -----------------------------------------------------------|   (0...0)   *
 * SMAUG 1.8 (C) 1994, 1995, 1996, 1998  by Derek Snider      |    ).:.(    *
 * -----------------------------------------------------------|    {o o}    *
 * SMAUG code team: Thoric, Altrag, Blodkai, Narn, Haus,      |   / ' ' \   *
 * Scryn, Rennard, Swordbearer, Gorog, Grishnakh, Nivek,      |~'~.VxvxV.~'~*
 * Tricops, Fireblade, Edmond, Conran                         |             *
 * ------------------------------------------------------------------------ *
 * Merc 2.1 Diku Mud improvments copyright (C) 1992, 1993 by Michael        *
 * Chastain, Michael Quan, and Mitchell Tse.                                *
 * Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,          *
 * Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.     *
 * ------------------------------------------------------------------------ *
 *                         Object manipulation module                       *
 ****************************************************************************/

#include <stdio.h>
#include <string.h>
#include "mud.h"
#include "bet.h"

/*
 * External functions
 */
void write_corpses(CHAR_DATA * ch, const char *name, OBJ_DATA * objrem);
void save_house_by_vnum(int vnum);

/*
 * how resistant an object is to damage                         -Thoric
 */
short get_obj_resistance(OBJ_DATA * obj)
{
  short resist;

  resist = number_fuzzy(MAX_ITEM_IMPACT);

  /*
   * magical items are more resistant 
   */
  if (IS_OBJ_STAT(obj, ITEM_MAGIC))
    resist += number_fuzzy(12);

  /*
   * metal objects are definately stronger 
   */
  if (IS_OBJ_STAT(obj, ITEM_METAL))
    resist += number_fuzzy(5);

  /*
   * organic objects are most likely weaker 
   */
  if (IS_OBJ_STAT(obj, ITEM_ORGANIC))
    resist -= number_fuzzy(5);

  /*
   * blessed objects should have a little bonus 
   */
  if (IS_OBJ_STAT(obj, ITEM_BLESS))
    resist += number_fuzzy(5);

  /*
   * lets make store inventory pretty tough 
   */
  if (IS_OBJ_STAT(obj, ITEM_INVENTORY))
    resist += 20;

  /*
   * okay... let's add some bonus/penalty for item level... 
   */
  resist += (obj->level / 10) - 2;

  /*
   * and lasty... take armor or weapon's condition into consideration 
   */
  if (obj->item_type == ITEM_ARMOR || obj->item_type == ITEM_WEAPON)
    resist += (obj->value[0] / 2) - 2;

  return URANGE(10, resist, 99);
}

void get_obj(CHAR_DATA * ch, OBJ_DATA * obj, OBJ_DATA * container)
{
  VAULT_DATA *vault;
  int weight;
  int amt; /* gold per-race multipliers */

  if (!CAN_WEAR(obj, ITEM_TAKE) && (ch->level < sysdata.level_getobjnotake))
  {
    send_to_char("You can't take that.\r\n", ch);
    return;
  }

  if (IS_SET(obj->magic_flags, ITEM_PKDISARMED) && !IS_NPC(ch))
  {
    if (CAN_PKILL(ch) && !get_timer(ch, TIMER_PKILLED))
    {
      if (!is_name(ch->name, obj->action_desc) && !IS_IMMORTAL(ch))
      {
        send_to_char_color("\r\n&bA godly force freezes your outstretched hand.\r\n", ch);
        return;
      }
      else
      {
        REMOVE_BIT(obj->magic_flags, ITEM_PKDISARMED);
        STRFREE(obj->action_desc);
        obj->action_desc = STRALLOC("");
      }
    }
    else
    {
      send_to_char_color("\r\n&BA godly force freezes your outstretched hand.\r\n", ch);
      return;
    }
  }

  if (IS_OBJ_STAT(obj, ITEM_PROTOTYPE) && !can_take_proto(ch))
  {
    send_to_char("A godly force prevents you from getting close to it.\r\n", ch);
    return;
  }

  if (ch->carry_number + get_obj_number(obj) > can_carry_n(ch))
  {
    act(AT_PLAIN, "$d: you can't carry that many items.", ch, NULL, obj->short_descr, TO_CHAR);
    return;
  }

  if (IS_OBJ_STAT(obj, ITEM_COVERING))
    weight = obj->weight;
  else
    weight = get_obj_weight(obj);

  /*
   * Money weight shouldn't count 
   */
  if (obj->item_type != ITEM_MONEY)
  {
    if (obj->in_obj)
    {
      OBJ_DATA *tobj = obj->in_obj;
      int inobj = 1;
      bool checkweight = FALSE;

      /*
       * need to make it check weight if its in a magic container 
       */
      if (tobj->item_type == ITEM_CONTAINER && IS_OBJ_STAT(tobj, ITEM_MAGIC))
        checkweight = TRUE;

      while (tobj->in_obj)
      {
        tobj = tobj->in_obj;
        inobj++;

        /*
         * need to make it check weight if its in a magic container 
         */
        if (tobj->item_type == ITEM_CONTAINER && IS_OBJ_STAT(tobj, ITEM_MAGIC))
          checkweight = TRUE;
      }

      /*
       * need to check weight if not carried by ch or in a magic container. 
       */
      if (!tobj->carried_by || tobj->carried_by != ch || checkweight)
      {
        if ((ch->carry_weight + weight) > can_carry_w(ch))
        {
          act(AT_PLAIN, "$d: you can't carry that much weight.", ch, NULL, obj->short_descr, TO_CHAR);
          return;
        }
      }
    }
    else if ((ch->carry_weight + weight) > can_carry_w(ch))
    {
      act(AT_PLAIN, "$d: you can't carry that much weight.", ch, NULL, obj->short_descr, TO_CHAR);
      return;
    }
  }

  if (container)
  {
    if (container->item_type == ITEM_KEYRING && !IS_OBJ_STAT(container, ITEM_COVERING)) {
      act(AT_ACTION, "You remove $p from $P", ch, obj, container, TO_CHAR);
      act(AT_ACTION, "$n removes $p from $P", ch, obj, container, TO_ROOM);
    }
    else if (container->item_type == ITEM_SHELF) {
      act(AT_ACTION, "$n gets $p from $P.", ch, obj, container, TO_ROOM);
      act(AT_ACTION, "You get $p from $P.", ch, obj, container, TO_CHAR);
    }
    else {
      act(AT_ACTION, IS_OBJ_STAT(container, ITEM_COVERING) ?
           "You get $p from beneath $P." : "You get $p from $P", ch, obj, container, TO_CHAR);
      act(AT_ACTION, IS_OBJ_STAT(container, ITEM_COVERING) ?
           "$n gets $p from beneath $P." : "$n gets $p from $P", ch, obj, container, TO_ROOM);
    }
    if (IS_OBJ_STAT(container, ITEM_CLANCORPSE) && !IS_NPC(ch) && str_cmp(container->name + 7, ch->name))
      container->value[5]++;
    obj_from_obj(obj);
  }
  else
  {
    act(AT_ACTION, "You get $p.", ch, obj, container, TO_CHAR);
    act(AT_ACTION, "$n gets $p.", ch, obj, container, TO_ROOM);
    obj_from_room(obj);
  }

  if (xIS_SET(ch->in_room->room_flags, ROOM_HOUSE))
    save_house_by_vnum(ch->in_room->vnum); /* House Object Saving */

  /*
   * Clan storeroom checks
   */
  if (xIS_SET(ch->in_room->room_flags, ROOM_CLANSTOREROOM) && (!container || container->carried_by == NULL))
  {
    for (vault = first_vault; vault; vault = vault->next)
      if (vault->vnum == ch->in_room->vnum)
        save_storeroom(ch, vault->vnum);
  }

  check_for_trap(ch, obj, TRAP_GET);
  if (char_died(ch))
    return;

  if (obj->item_type == ITEM_MONEY)
  {
    amt = obj->value[0] * obj->count;

    ch->gold += amt;
    extract_obj(obj);
  }
  else
    obj = obj_to_char(obj, ch);

  if (char_died(ch) || obj_extracted(obj))
    return;
  oprog_get_trigger(ch, obj);
  return;
}

void do_connect(CHAR_DATA *ch, const char *argument)
{
  OBJ_DATA *first_ob;
  OBJ_DATA *second_ob;
  OBJ_DATA *new_ob;
  char arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH];

  argument = one_argument(argument, arg1);
  argument = one_argument(argument, arg2);

  if (arg1[0] == '\0' || arg2[0] == '\0')
  {
    send_to_char("Connect what to what?\r\n", ch);
    return;
  }

  if ((first_ob = get_obj_carry(ch, arg1))  == NULL)
  {
    send_to_char("You aren't holding the necessary objects.\r\n", ch);
    return;
  }

  if ((second_ob = get_obj_carry(ch, arg2))  == NULL)
  {
    send_to_char("You aren't holding the necessary objects.\r\n", ch);
    return;
  }

  separate_obj(first_ob);
  separate_obj(second_ob);

  if (first_ob->item_type != ITEM_PIECE || second_ob->item_type !=ITEM_PIECE)
  {
    send_to_char("You stare at them for a moment, but these items clearly don't go together.\r\n", ch);
    return;
  }

  /* check to see if the pieces connect */
  if ((first_ob->value[0] == second_ob->pIndexData->vnum) || (first_ob->value[1] == second_ob->pIndexData->vnum))
    /* good connection  */
  {
    new_ob = create_object(get_obj_index(first_ob->value[2]), ch->level);
    extract_obj(first_ob);
    extract_obj(second_ob);
    obj_to_char(new_ob , ch);
    act(AT_ACTION, "$n cobbles some objects together.... suddenly they snap together, creating $p!", ch, new_ob,NULL, TO_ROOM);
    act(AT_ACTION, "You cobble the objects together.... suddenly they snap together, creating $p!", ch, new_ob, NULL, TO_CHAR);
  }
  else
  {
    act(AT_ACTION, "$n jiggles some objects against each other, but can't seem to make them connect.", ch, NULL, NULL, TO_ROOM);
    act(AT_ACTION, "You try to fit them together every which way, but they just don't want to connect.", ch, NULL, NULL, TO_CHAR);
  }
}

void do_get(CHAR_DATA* ch, const char* argument)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  OBJ_DATA *obj;
  OBJ_DATA *obj_next;
  OBJ_DATA *container;
  short number;
  bool found;

  argument = one_argument(argument, arg1);

  if (ch->carry_number < 0 || ch->carry_weight < 0)
  {
    send_to_char("Uh oh ... better contact an immortal about your number or weight of items carried.\r\n", ch);
    log_printf("%s has negative carry_number or carry_weight!", ch->name);
    return;
  }

  if (is_number(arg1))
  {
    number = atoi(arg1);
    if (number < 1)
    {
      send_to_char("That was easy...\r\n", ch);
      return;
    }
    if ((ch->carry_number + number) > can_carry_n(ch))
    {
      send_to_char("You can't carry that many.\r\n", ch);
      return;
    }
    argument = one_argument(argument, arg1);
  }
  else
    number = 0;
  argument = one_argument(argument, arg2);
  /*
   * munch optional words 
   */
  if (!str_cmp(arg2, "from") && argument[0] != '\0')
    argument = one_argument(argument, arg2);

  /*
   * Get type. 
   */
  if (arg1[0] == '\0')
  {
    send_to_char("Get what?\r\n", ch);
    return;
  }

  if (ms_find_obj(ch))
    return;

  if (arg2[0] == '\0')
  {
    if (number <= 1 && str_cmp(arg1, "all") && str_prefix("all.", arg1))
    {
      /*
       * 'get obj' 
       */
      obj = get_obj_list(ch, arg1, ch->in_room->first_content);
      if (!obj)
      {
        act(AT_PLAIN, "I see no $T here.", ch, NULL, arg1, TO_CHAR);
        return;
      }
      separate_obj(obj);
      get_obj(ch, obj, NULL);
      if (char_died(ch))
        return;
      if (IS_SET(sysdata.save_flags, SV_GET))
        save_char_obj(ch);
    }
    else
    {
      short cnt = 0;
      bool fAll;
      char *chk;

      if (xIS_SET(ch->in_room->room_flags, ROOM_DONATION))
      {
        send_to_char("The gods frown upon such a display of greed!\r\n", ch);
        return;
      }
      if (!str_cmp(arg1, "all"))
        fAll = TRUE;
      else
        fAll = FALSE;
      if (number > 1)
        chk = arg1;
      else
        chk = &arg1[4];
      /*
       * 'get all' or 'get all.obj' 
       */
      found = FALSE;
      for (obj = ch->in_room->last_content; obj; obj = obj_next)
      {
        obj_next = obj->prev_content;
        if ((fAll || nifty_is_name(chk, obj->name)) && can_see_obj(ch, obj))
        {
          found = TRUE;
          if (number && (cnt + obj->count) > number)
            split_obj(obj, number - cnt);
          cnt += obj->count;
          get_obj(ch, obj, NULL);
          if (char_died(ch)
              || ch->carry_number >= can_carry_n(ch)
              || ch->carry_weight >= can_carry_w(ch) || (number && cnt >= number))
          {
            if (IS_SET(sysdata.save_flags, SV_GET) && !char_died(ch))
              save_char_obj(ch);
            return;
          }
        }
      }

      if (!found)
      {
        if (fAll)
          send_to_char("I see nothing here.\r\n", ch);
        else
          act(AT_PLAIN, "I see no $T here.", ch, NULL, chk, TO_CHAR);
      }
      else if (IS_SET(sysdata.save_flags, SV_GET))
        save_char_obj(ch);
    }
  }
  else
  {
    /*
     * 'get ... container' 
     */
    if (!str_cmp(arg2, "all") || !str_prefix("all.", arg2))
    {
      send_to_char("You can't do that.\r\n", ch);
      return;
    }

    if ((container = get_obj_here(ch, arg2)) == NULL)
    {
      act(AT_PLAIN, "I see no $T here.", ch, NULL, arg2, TO_CHAR);
      return;
    }

    switch (container->item_type)
    {
    default:
      if (!IS_OBJ_STAT(container, ITEM_COVERING))
      {
        send_to_char("That's not a container.\r\n", ch);
        return;
      }
      if (ch->carry_weight + container->weight > can_carry_w(ch))
      {
        send_to_char("It's too heavy for you to lift.\r\n", ch);
        return;
      }
      break;

    case ITEM_CONTAINER:
    case ITEM_SHELF:
    case ITEM_CORPSE_NPC:
    case ITEM_KEYRING:
    case ITEM_QUIVER:
      break;

    case ITEM_CORPSE_PC:
    {
      char name[MAX_INPUT_LENGTH];
      CHAR_DATA *gch;
      const char *pd;

      if (IS_NPC(ch))
      {
        send_to_char("You can't do that.\r\n", ch);
        return;
      }

      pd = container->short_descr;
      pd = one_argument(pd, name);
      pd = one_argument(pd, name);
      pd = one_argument(pd, name);
      pd = one_argument(pd, name);

      if (IS_OBJ_STAT(container, ITEM_CLANCORPSE)
          && !IS_NPC(ch) && (get_timer(ch, TIMER_PKILLED) > 0) && str_cmp(name, ch->name))
      {
        send_to_char("You cannot loot from that corpse...yet.\r\n", ch);
        return;
      }

      /*
       * Killer/owner loot only if die to pkill blow --Blod 
       */
      /*
       * Added check for immortal so IMMS can get things out of
       * * corpses --Shaddai 
       */

      if (IS_OBJ_STAT(container, ITEM_CLANCORPSE)
          && !IS_NPC(ch) && !IS_IMMORTAL(ch)
          && container->action_desc[0] != '\0'
          && str_cmp(name, ch->name) && str_cmp(container->action_desc, ch->name))
      {
        send_to_char("You did not inflict the death blow upon this corpse.\r\n", ch);
        return;
      }

      if (IS_OBJ_STAT(container, ITEM_CLANCORPSE) && !IS_IMMORTAL(ch)
          && !IS_NPC(ch) && str_cmp(name, ch->name) && container->value[5] >= 3)
      {
        send_to_char("Frequent looting has left this corpse protected by the gods.\r\n", ch);
        return;
      }

      if (IS_OBJ_STAT(container, ITEM_CLANCORPSE)
          && !IS_NPC(ch) && IS_SET(ch->pcdata->flags, PCFLAG_DEADLY)
          && container->value[4] - ch->level < 6 && container->value[4] - ch->level > -6)
        break;

      if (!str_cmp(name, ch->name) && !IS_IMMORTAL(ch))
      {
        bool fGroup;

        fGroup = FALSE;
        for (gch = first_char; gch; gch = gch->next)
        {
          if (!IS_NPC(gch) && is_same_group(ch, gch) && !str_cmp(name, gch->name))
          {
            fGroup = TRUE;
            break;
          }
        }

        if (!fGroup)
        {
          send_to_char("That's someone else's corpse.\r\n", ch);
          return;
        }
      }
    }
    }

    if (!IS_OBJ_STAT(container, ITEM_COVERING) && IS_SET(container->value[1], CONT_CLOSED))
    {
      act(AT_PLAIN, "The $d is closed.", ch, NULL, container->name, TO_CHAR);
      return;
    }

    if (number <= 1 && str_cmp(arg1, "all") && str_prefix("all.", arg1))
    {
      /*
       * 'get obj container' 
       */
      obj = get_obj_list(ch, arg1, container->first_content);
      if (!obj)
      {
        act(AT_PLAIN, IS_OBJ_STAT(container, ITEM_COVERING) ?
             "I see nothing like that beneath the $T." : "I see nothing like that in the $T.", ch, NULL, container->short_descr, TO_CHAR);
        return;
      }
      separate_obj(obj);
      get_obj(ch, obj, container);
      /*
       * Oops no wonder corpses were duping oopsie did I do that
       * * --Shaddai
       */
      if (container->item_type == ITEM_CORPSE_PC)
        write_corpses(NULL, container->short_descr + 14, NULL);
      check_for_trap(ch, container, TRAP_GET);
      if (char_died(ch))
        return;
      if (IS_SET(sysdata.save_flags, SV_GET))
        save_char_obj(ch);
    }
    else
    {
      int cnt = 0;
      bool fAll;
      char *chk;

      /*
       * 'get all container' or 'get all.obj container' 
       */
      if (IS_OBJ_STAT(container, ITEM_DONATION))
      {
        send_to_char("The gods frown upon such an act of greed!\r\n", ch);
        return;
      }

      if (IS_OBJ_STAT(container, ITEM_CLANCORPSE)
          && !IS_IMMORTAL(ch) && !IS_NPC(ch) && str_cmp(ch->name, container->name + 7))
      {
        send_to_char("The gods frown upon such wanton greed!\r\n", ch);
        return;
      }

      if (!str_cmp(arg1, "all"))
        fAll = TRUE;
      else
        fAll = FALSE;
      if (number > 1)
        chk = arg1;
      else
        chk = &arg1[4];
      found = FALSE;
      for (obj = container->first_content; obj; obj = obj_next)
      {
        obj_next = obj->next_content;
        if ((fAll || nifty_is_name(chk, obj->name)) && can_see_obj(ch, obj))
        {
          found = TRUE;
          if (number && (cnt + obj->count) > number)
            split_obj(obj, number - cnt);
          cnt += obj->count;
          get_obj(ch, obj, container);
          if (char_died(ch)
              || ch->carry_number >= can_carry_n(ch)
              || ch->carry_weight >= can_carry_w(ch) || (number && cnt >= number))
          {
            if (container->item_type == ITEM_CORPSE_PC)
              write_corpses(NULL, container->short_descr + 14, NULL);
            if (found && IS_SET(sysdata.save_flags, SV_GET))
              save_char_obj(ch);
            return;
          }
        }
      }

      if (!found)
      {
        if (fAll)
        {
          if (container->item_type == ITEM_KEYRING && !IS_OBJ_STAT(container, ITEM_COVERING))
            act(AT_PLAIN, "The $T holds no keys.", ch, NULL, container->short_descr, TO_CHAR);
          else
            act(AT_PLAIN, IS_OBJ_STAT(container, ITEM_COVERING) ?
                 "I see nothing beneath the $T." : "I see nothing in the $T.", ch, NULL, container->short_descr, TO_CHAR);
        }
        else
        {
          if (container->item_type == ITEM_KEYRING && !IS_OBJ_STAT(container, ITEM_COVERING))
            act(AT_PLAIN, "The $T does not hold that key.", ch, NULL, container->short_descr, TO_CHAR);
          else
            act(AT_PLAIN, IS_OBJ_STAT(container, ITEM_COVERING) ?
                 "I see nothing like that beneath the $T." :
                 "I see nothing like that in the $T.", ch, NULL, container->short_descr, TO_CHAR);
        }
      }
      else
        check_for_trap(ch, container, TRAP_GET);
      if (char_died(ch))
        return;
      /*
       * Oops no wonder corpses were duping oopsie did I do that
       * * --Shaddai
       */
      if (container->item_type == ITEM_CORPSE_PC)
        write_corpses(NULL, container->short_descr + 14, NULL);
      if (found && IS_SET(sysdata.save_flags, SV_GET))
        save_char_obj(ch);
    }
  }
  return;
}

void do_put(CHAR_DATA* ch, const char* argument)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  OBJ_DATA *container;
  OBJ_DATA *obj;
  OBJ_DATA *obj_next;
  VAULT_DATA *vault;
  short count;
  int number;
  bool save_char = FALSE;

  argument = one_argument(argument, arg1);
  if (is_number(arg1))
  {
    number = atoi(arg1);
    if (number < 1)
    {
      send_to_char("That was easy...\r\n", ch);
      return;
    }
    argument = one_argument(argument, arg1);
  }
  else
    number = 0;
  argument = one_argument(argument, arg2);
  /*
   * munch optional words 
   */
  if ((!str_cmp(arg2, "into") || !str_cmp(arg2, "inside")
        || !str_cmp(arg2, "in") || !str_cmp(arg2, "under")
        || !str_cmp(arg2, "onto") || !str_cmp(arg2, "on")) && argument[0] != '\0')
    argument = one_argument(argument, arg2);

  if (arg1[0] == '\0' || arg2[0] == '\0')
  {
    send_to_char("Put what in what?\r\n", ch);
    return;
  }

  if (ms_find_obj(ch))
    return;

  if (!str_cmp(arg2, "all") || !str_prefix("all.", arg2))
  {
    send_to_char("You can't do that.\r\n", ch);
    return;
  }

  if ((container = get_obj_here(ch, arg2)) == NULL)
  {
    act(AT_PLAIN, "I see no $T here.", ch, NULL, arg2, TO_CHAR);
    return;
  }

  if (!container->carried_by && IS_SET(sysdata.save_flags, SV_PUT))
    save_char = TRUE;

  if (IS_OBJ_STAT(container, ITEM_COVERING))
  {
    if (ch->carry_weight + container->weight > can_carry_w(ch))
    {
      send_to_char("It's too heavy for you to lift.\r\n", ch);
      return;
    }
  }
  else
  {
    if (container->item_type != ITEM_CONTAINER
        && container->item_type != ITEM_SHELF
        && container->item_type != ITEM_KEYRING 
        && container->item_type != ITEM_QUIVER)
    {
      send_to_char("That's not a container.\r\n", ch);
      return;
    }

    if (IS_SET(container->value[1], CONT_CLOSED))
    {
      act(AT_PLAIN, "The $d is closed.", ch, NULL, container->name, TO_CHAR);
      return;
    }
  }

  if (number <= 1 && str_cmp(arg1, "all") && str_prefix("all.", arg1))
  {
    /*
     * 'put obj container' 
     */
    if ((obj = get_obj_carry(ch, arg1)) == NULL)
    {
      send_to_char("You do not have that item.\r\n", ch);
      return;
    }

    if (obj == container)
    {
      send_to_char("You can't fold it into itself.\r\n", ch);
      return;
    }

    if (!can_drop_obj(ch, obj))
    {
      send_to_char("You can't let go of it.\r\n", ch);
      return;
    }

    if (container->item_type == ITEM_KEYRING && obj->item_type != ITEM_KEY)
    {
      send_to_char("That's not a key.\r\n", ch);
      return;
    }

    if (container->item_type == ITEM_QUIVER && obj->item_type != ITEM_PROJECTILE)
    {
      send_to_char("That's not a projectile.\r\n", ch);
      return;
    }

    // Fix by Luc - Sometime in 2000?
    {
      int tweight = (get_real_obj_weight(container) / container->count) + (get_real_obj_weight(obj) / obj->count);

      if (IS_OBJ_STAT(container, ITEM_COVERING))
      {
        if (container->item_type == ITEM_CONTAINER
            ? tweight > container->value[0] : tweight - container->weight > container->weight)
        {
          send_to_char("It won't fit under there.\r\n", ch);
          return;
        }
      }
      else if (tweight - container->weight > container->value[0])
      {
        send_to_char("It won't fit.\r\n", ch);
        return;
      }
    }
    // Fix end

    if (container->in_room && container->in_room->max_weight
        && container->in_room->max_weight < get_real_obj_weight(obj) / obj->count + container->in_room->weight)
    {
      send_to_char("It won't fit.\r\n", ch);
      return;
    }

    separate_obj(obj);
    separate_obj(container);
    obj_from_char(obj);
    obj = obj_to_obj(obj, container);
    check_for_trap(ch, container, TRAP_PUT);
    if (char_died(ch))
      return;
    count = obj->count;
    obj->count = 1;
    if (container->item_type == ITEM_KEYRING && !IS_OBJ_STAT(container, ITEM_COVERING)) {
      act(AT_ACTION, "$n slips $p onto $P.", ch, obj, container, TO_ROOM);
      act(AT_ACTION, "You slip $p onto $P.", ch, obj, container, TO_CHAR);
    }
    else if (container->item_type == ITEM_SHELF) {
      snprintf(buf, MAX_STRING_LENGTH, "%s %s", prepositions[container->value[4]], container->short_descr);
      act(AT_ACTION, "$n puts $p $T.", ch, obj, buf, TO_ROOM);
      act(AT_ACTION, "You put $p $T.", ch, obj, buf, TO_CHAR);
    }
    else {
      act(AT_ACTION, IS_OBJ_STAT(container, ITEM_COVERING)
           ? "$n hides $p beneath $P." : "$n puts $p in $P.", ch, obj, container, TO_ROOM);
      act(AT_ACTION, IS_OBJ_STAT(container, ITEM_COVERING)
           ? "You hide $p beneath $P." : "You put $p in $P.", ch, obj, container, TO_CHAR);
    }
    obj->count = count;

    /*
     * Oops no wonder corpses were duping oopsie did I do that
     * * --Shaddai
     */
    if (container->item_type == ITEM_CORPSE_PC)
      write_corpses(NULL, container->short_descr + 14, NULL);

    if (save_char)
      save_char_obj(ch);

    if (xIS_SET(ch->in_room->room_flags, ROOM_HOUSE))
      save_house_by_vnum(ch->in_room->vnum); /* House Object Saving */

    /*
     * Clan storeroom check 
     */
    if (xIS_SET(ch->in_room->room_flags, ROOM_CLANSTOREROOM) && container->carried_by == NULL)
    {
      for (vault = first_vault; vault; vault = vault->next)
        if (vault->vnum == ch->in_room->vnum)
          save_storeroom(ch, vault->vnum);
    }
  }
  else
  {
    bool found = FALSE;
    int cnt = 0;
    bool fAll;
    char *chk;

    if (!str_cmp(arg1, "all"))
      fAll = TRUE;
    else
      fAll = FALSE;
    if (number > 1)
      chk = arg1;
    else
      chk = &arg1[4];

    separate_obj(container);
    /*
     * 'put all container' or 'put all.obj container' 
     */
    for (obj = ch->first_carrying; obj; obj = obj_next)
    {
      obj_next = obj->next_content;

      if ((fAll || nifty_is_name(chk, obj->name))
          && can_see_obj(ch, obj)
          && obj->wear_loc == WEAR_NONE
          && obj != container
          && can_drop_obj(ch, obj)
          && (container->item_type != ITEM_KEYRING || obj->item_type == ITEM_KEY)
          && (container->item_type != ITEM_QUIVER || obj->item_type == ITEM_PROJECTILE)
          && get_obj_weight(obj) + get_obj_weight(container) <= container->value[0])
      {
        if (number && (cnt + obj->count) > number)
          split_obj(obj, number - cnt);
        cnt += obj->count;
        obj_from_char(obj);
        if (container->item_type == ITEM_KEYRING) {
          act(AT_ACTION, "$n slips $p onto $P.", ch, obj, container, TO_ROOM);
          act(AT_ACTION, "You slip $p onto $P.", ch, obj, container, TO_CHAR);
        }
	else if (container->item_type == ITEM_SHELF) {
	  snprintf(buf, MAX_STRING_LENGTH, "%s %s", prepositions[container->value[4]], container->short_descr);
	  act(AT_ACTION, "$n puts $p $T.", ch, obj, buf, TO_ROOM);
	  act(AT_ACTION, "You put $p $T.", ch, obj, buf, TO_CHAR);
	}
        else {
          act(AT_ACTION, "$n puts $p in $P.", ch, obj, container, TO_ROOM);
          act(AT_ACTION, "You put $p in $P.", ch, obj, container, TO_CHAR);
        }
        obj = obj_to_obj(obj, container);
        found = TRUE;

        check_for_trap(ch, container, TRAP_PUT);
        if (char_died(ch))
          return;
        if (number && cnt >= number)
          break;
      }
    }

    /*
     * Don't bother to save anything if nothing was dropped   -Thoric
     */
    if (!found)
    {
      if (fAll)
        act(AT_PLAIN, "You are not carrying anything.", ch, NULL, NULL, TO_CHAR);
      else
        act(AT_PLAIN, "You are not carrying any $T.", ch, NULL, chk, TO_CHAR);
      return;
    }

    if (save_char)
      save_char_obj(ch);
    /*
     * Oops no wonder corpses were duping oopsie did I do that
     * * --Shaddai
     */
    if (container->item_type == ITEM_CORPSE_PC)
      write_corpses(NULL, container->short_descr + 14, NULL);

    if (xIS_SET(ch->in_room->room_flags, ROOM_HOUSE))
      save_house_by_vnum(ch->in_room->vnum); /* House Object Saving */

    /*
     * Clan storeroom check 
     */
    if (xIS_SET(ch->in_room->room_flags, ROOM_CLANSTOREROOM) && container->carried_by == NULL)
    {
      for (vault = first_vault; vault; vault = vault->next)
        if (vault->vnum == ch->in_room->vnum)
          save_storeroom(ch, vault->vnum);
    }
  }
  return;
}

void do_drop(CHAR_DATA* ch, const char* argument)
{
  char arg[MAX_INPUT_LENGTH];
  OBJ_DATA *obj;
  OBJ_DATA *obj_next;
  bool found;
  VAULT_DATA *vault;
  int number;

  argument = one_argument(argument, arg);
  if (is_number(arg))
  {
    number = atoi(arg);
    if (number < 1)
    {
      send_to_char("That was easy...\r\n", ch);
      return;
    }
    argument = one_argument(argument, arg);
  }
  else
    number = 0;

  if (arg[0] == '\0')
  {
    send_to_char("Drop what?\r\n", ch);
    return;
  }

  if (ms_find_obj(ch))
    return;

  if (!IS_NPC(ch) && xIS_SET(ch->act, PLR_LITTERBUG))
  {
    set_char_color(AT_YELLOW, ch);
    send_to_char("A godly force prevents you from dropping anything...\r\n", ch);
    return;
  }

  if (xIS_SET(ch->in_room->room_flags, ROOM_NODROP) && ch != supermob)
  {
    set_char_color(AT_MAGIC, ch);
    send_to_char("A magical force stops you!\r\n", ch);
    set_char_color(AT_TELL, ch);
    send_to_char("Someone tells you, 'No littering here!'\r\n", ch);
    return;
  }

  if (number > 0)
  {
    /*
     * 'drop NNNN coins' 
     */
    if (!str_cmp(arg, "coins") || !str_cmp(arg, "coin"))
    {
      if (ch->gold < number)
      {
        send_to_char("You haven't got that many coins.\r\n", ch);
        return;
      }

      ch->gold -= number;

      for (obj = ch->in_room->first_content; obj; obj = obj_next)
      {
        obj_next = obj->next_content;

        switch (obj->pIndexData->vnum)
        {
        case OBJ_VNUM_MONEY_ONE:
          number += 1;
          extract_obj(obj);
          break;

        case OBJ_VNUM_MONEY_SOME:
          number += obj->value[0];
          extract_obj(obj);
          break;
        }
      }

      act(AT_ACTION, "$n drops some gold.", ch, NULL, NULL, TO_ROOM);
      obj_to_room(create_money(number), ch->in_room);
      send_to_char("You let the gold slip from your hand.\r\n", ch);
      if (IS_SET(sysdata.save_flags, SV_DROP))
        save_char_obj(ch);
      return;
    }
  }

  if (number <= 1 && str_cmp(arg, "all") && str_prefix("all.", arg))
  {
    /*
     * 'drop obj' 
     */
    if ((obj = get_obj_carry(ch, arg)) == NULL)
    {
      send_to_char("You do not have that item.\r\n", ch);
      return;
    }

    if (!can_drop_obj(ch, obj))
    {
      send_to_char("You can't let go of it.\r\n", ch);
      return;
    }

    if (ch->in_room->max_weight > 0
        && ch->in_room->max_weight < get_real_obj_weight(obj) / obj->count + ch->in_room->weight)
    {
      send_to_char("There is not enough room on the ground for that.\r\n", ch);
      return;
    }

    separate_obj(obj);
    act(AT_ACTION, "$n drops $p.", ch, obj, NULL, TO_ROOM);
    act(AT_ACTION, "You drop $p.", ch, obj, NULL, TO_CHAR);

    obj_from_char(obj);
    obj = obj_to_room(obj, ch->in_room);
    oprog_drop_trigger(ch, obj);   /* mudprogs */

    if (char_died(ch) || obj_extracted(obj))
      return;

    if (xIS_SET(ch->in_room->room_flags, ROOM_HOUSE))
      save_house_by_vnum(ch->in_room->vnum); /* House Object Saving */

    /*
     * Clan storeroom saving 
     */
    if (xIS_SET(ch->in_room->room_flags, ROOM_CLANSTOREROOM))
    {
      for (vault = first_vault; vault; vault = vault->next)
        if (vault->vnum == ch->in_room->vnum)
          save_storeroom(ch, vault->vnum);
    }
  }
  else
  {
    int cnt = 0;
    char *chk;
    bool fAll;

    if (!str_cmp(arg, "all"))
      fAll = TRUE;
    else
      fAll = FALSE;
    if (number > 1)
      chk = arg;
    else
      chk = &arg[4];
    /*
     * 'drop all' or 'drop all.obj' 
     */
    if (xIS_SET(ch->in_room->room_flags, ROOM_NODROPALL) || xIS_SET(ch->in_room->room_flags, ROOM_CLANSTOREROOM))
    {
      send_to_char("You can't seem to do that here...\r\n", ch);
      return;
    }

    found = FALSE;
    for (obj = ch->first_carrying; obj; obj = obj_next)
    {
      obj_next = obj->next_content;

      if ((fAll || nifty_is_name(chk, obj->name))
          && can_see_obj(ch, obj) && obj->wear_loc == WEAR_NONE && can_drop_obj(ch, obj)
          && (!ch->in_room->max_weight || ch->in_room->max_weight > get_real_obj_weight(obj) / obj->count + ch->in_room->weight))
      {
        found = TRUE;
        if (HAS_PROG(obj->pIndexData, DROP_PROG) && obj->count > 1)
        {
          ++cnt;
          separate_obj(obj);
          obj_from_char(obj);
          if (!obj_next)
            obj_next = ch->first_carrying;
        }
        else
        {
          if (number && (cnt + obj->count) > number)
            split_obj(obj, number - cnt);
          cnt += obj->count;
          obj_from_char(obj);
        }
        act(AT_ACTION, "$n drops $p.", ch, obj, NULL, TO_ROOM);
        act(AT_ACTION, "You drop $p.", ch, obj, NULL, TO_CHAR);
        obj = obj_to_room(obj, ch->in_room);
        oprog_drop_trigger(ch, obj);   /* mudprogs */
        if (char_died(ch))
          return;
        if (number && cnt >= number)
          break;
      }
    }

    if (!found)
    {
      if (fAll)
        act(AT_PLAIN, "You are not carrying anything.", ch, NULL, NULL, TO_CHAR);
      else
        act(AT_PLAIN, "You are not carrying any $T.", ch, NULL, chk, TO_CHAR);
    }
  }

  if (xIS_SET(ch->in_room->room_flags, ROOM_HOUSE))
    save_house_by_vnum(ch->in_room->vnum); /* House Object Saving */

  /*
   * Clan storeroom saving 
   */
  if (xIS_SET(ch->in_room->room_flags, ROOM_CLANSTOREROOM))
  {
    for (vault = first_vault; vault; vault = vault->next)
      if (vault->vnum == ch->in_room->vnum)
        save_storeroom(ch, vault->vnum);
  }

  if (IS_SET(sysdata.save_flags, SV_DROP))
    save_char_obj(ch); /* duping protector */
  return;
}

void do_give(CHAR_DATA* ch, const char* argument)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char buf[MAX_INPUT_LENGTH];
  CHAR_DATA *victim;
  OBJ_DATA *obj;

  argument = one_argument(argument, arg1);
  argument = one_argument(argument, arg2);
  if (!str_cmp(arg2, "to") && argument[0] != '\0')
    argument = one_argument(argument, arg2);

  if (arg1[0] == '\0' || arg2[0] == '\0')
  {
    send_to_char("Give what to whom?\r\n", ch);
    return;
  }

  if (ms_find_obj(ch))
    return;

  if (is_number(arg1))
  {
    /*
     * 'give NNNN coins victim' 
     */
    int amount;

    amount = atoi(arg1);
    if (amount <= 0 || (str_cmp(arg2, "coins") && str_cmp(arg2, "coin")))
    {
      send_to_char("Sorry, you can't do that.\r\n", ch);
      return;
    }

    argument = one_argument(argument, arg2);
    if (!str_cmp(arg2, "to") && argument[0] != '\0')
      argument = one_argument(argument, arg2);
    if (arg2[0] == '\0')
    {
      send_to_char("Give what to whom?\r\n", ch);
      return;
    }

    if ((victim = get_char_room(ch, arg2)) == NULL)
    {
      send_to_char("They aren't here.\r\n", ch);
      return;
    }

    if (ch->gold < amount)
    {
      send_to_char("Very generous of you, but you haven't got that much gold.\r\n", ch);
      return;
    }

    ch->gold -= amount;
    victim->gold += amount;
    mudstrlcpy(buf, "$n gives you ", MAX_STRING_LENGTH);
    mudstrlcat(buf, arg1, MAX_STRING_LENGTH);
    mudstrlcat(buf, (amount > 1) ? " coins." : " coin.", MAX_STRING_LENGTH);

    set_char_color(AT_GOLD, victim);
    act(AT_ACTION, buf, ch, NULL, victim, TO_VICT);
    act(AT_ACTION, "$n gives $N some gold.", ch, NULL, victim, TO_NOTVICT);
    act(AT_ACTION, "You give $N some gold.", ch, NULL, victim, TO_CHAR);
    mprog_bribe_trigger(victim, ch, amount);
    if (IS_SET(sysdata.save_flags, SV_GIVE) && !char_died(ch))
      save_char_obj(ch);
    if (IS_SET(sysdata.save_flags, SV_RECEIVE) && !char_died(victim))
      save_char_obj(victim);
    return;
  }

  if ((obj = get_obj_carry(ch, arg1)) == NULL)
  {
    send_to_char("You do not have that item.\r\n", ch);
    return;
  }

  if (obj->wear_loc != WEAR_NONE)
  {
    send_to_char("You must remove it first.\r\n", ch);
    return;
  }

  if ((victim = get_char_room(ch, arg2)) == NULL)
  {
    send_to_char("They aren't here.\r\n", ch);
    return;
  }

  if (!can_drop_obj(ch, obj))
  {
    send_to_char("You can't let go of it.\r\n", ch);
    return;
  }

  if (victim->carry_number + (get_obj_number(obj) / obj->count) > can_carry_n(victim))
  {
    act(AT_PLAIN, "$N has $S hands full.", ch, NULL, victim, TO_CHAR);
    return;
  }

  if (victim->carry_weight + (get_obj_weight(obj) / obj->count) > can_carry_w(victim))
  {
    act(AT_PLAIN, "$N can't carry that much weight.", ch, NULL, victim, TO_CHAR);
    return;
  }

  if (!can_see_obj(victim, obj))
  {
    act(AT_PLAIN, "$N can't see it.", ch, NULL, victim, TO_CHAR);
    return;
  }

  if (IS_OBJ_STAT(obj, ITEM_PROTOTYPE) && !can_take_proto(victim))
  {
    act(AT_PLAIN, "You cannot give that to $N!", ch, NULL, victim, TO_CHAR);
    return;
  }

  separate_obj(obj);
  obj_from_char(obj);
  act(AT_ACTION, "$n gives $p to $N.", ch, obj, victim, TO_NOTVICT);
  act(AT_ACTION, "$n gives you $p.", ch, obj, victim, TO_VICT);
  act(AT_ACTION, "You give $p to $N.", ch, obj, victim, TO_CHAR);
  obj = obj_to_char(obj, victim);
  mprog_give_trigger(victim, ch, obj);
  if (IS_SET(sysdata.save_flags, SV_GIVE) && !char_died(ch))
    save_char_obj(ch);
  if (IS_SET(sysdata.save_flags, SV_RECEIVE) && !char_died(victim))
    save_char_obj(victim);
  return;
}

/*
 * Damage an object.                                            -Thoric
 * Affect player's AC if necessary.
 * Make object into scraps if necessary.
 * Send message about damaged object.
 */
obj_ret damage_obj(OBJ_DATA * obj)
{
  CHAR_DATA *ch;
  obj_ret objcode;

  if (IS_OBJ_STAT(obj, ITEM_PERMANENT))
    return rNONE;

  ch = obj->carried_by;
  objcode = rNONE;

  separate_obj(obj);
  if (!IS_NPC(ch) && (!IS_PKILL(ch) || (IS_PKILL(ch) && !IS_SET(ch->pcdata->flags, PCFLAG_GAG))))
    act(AT_OBJECT, "($p gets damaged)", ch, obj, NULL, TO_CHAR);
  else if (obj->in_room && (ch = obj->in_room->first_person) != NULL)
  {
    act(AT_OBJECT, "($p gets damaged)", ch, obj, NULL, TO_ROOM);
    act(AT_OBJECT, "($p gets damaged)", ch, obj, NULL, TO_CHAR);
    ch = NULL;
  }

  if (obj->item_type != ITEM_LIGHT)
    oprog_damage_trigger(ch, obj);
  else if (!in_arena(ch))
    oprog_damage_trigger(ch, obj);

  if (obj_extracted(obj))
    return global_objcode;

  switch (obj->item_type)
  {
  default:
    make_scraps(obj);
    objcode = rOBJ_SCRAPPED;
    break;
  case ITEM_CONTAINER:
  case ITEM_KEYRING:
  case ITEM_QUIVER:
    if (--obj->value[3] <= 0)
    {
      if (!in_arena(ch))
      {
        make_scraps(obj);
        objcode = rOBJ_SCRAPPED;
      }
      else
        obj->value[3] = 1;
    }
    break;
  case ITEM_LIGHT:
    if (--obj->value[0] <= 0)
    {
      if (!in_arena(ch))
      {
        make_scraps(obj);
        objcode = rOBJ_SCRAPPED;
      }
      else
        obj->value[0] = 1;
    }
    break;
  case ITEM_ARMOR:
    if (ch && obj->value[0] >= 1)
      ch->armor += apply_ac(obj, obj->wear_loc);
    if (--obj->value[0] <= 0)
    {
      if (!IS_PKILL(ch) && !in_arena(ch))
      {
        make_scraps(obj);
        objcode = rOBJ_SCRAPPED;
      }
      else
      {
        obj->value[0] = 1;
        ch->armor -= apply_ac(obj, obj->wear_loc);
      }
    }
    else if (ch && obj->value[0] >= 1)
      ch->armor -= apply_ac(obj, obj->wear_loc);
    break;
  case ITEM_WEAPON:
    if (--obj->value[0] <= 0)
    {
      if (!IS_PKILL(ch) && !in_arena(ch))
      {
        make_scraps(obj);
        objcode = rOBJ_SCRAPPED;
      }
      else
        obj->value[0] = 1;
    }
    break;
  }
  if (ch != NULL)
    save_char_obj(ch); /* Stop scrap duping - Samson 1-2-00 */

  if (objcode == rOBJ_SCRAPPED && !IS_NPC(ch))
    log_printf("%s scrapped %s (vnum: %d)", ch->name, obj->short_descr, obj->pIndexData->vnum);

  return objcode;
}

/*
 * Remove an object.
 */
bool remove_obj(CHAR_DATA * ch, int iWear, bool fReplace)
{
  OBJ_DATA *obj, *tmpobj;

  if ((obj = get_eq_char(ch, iWear)) == NULL)
    return TRUE;

  if (!fReplace && ch->carry_number + get_obj_number(obj) > can_carry_n(ch))
  {
    act(AT_PLAIN, "$d: you can't carry that many items.", ch, NULL, obj->short_descr, TO_CHAR);
    return FALSE;
  }

  if (!fReplace)
    return FALSE;

  if (IS_OBJ_STAT(obj, ITEM_NOREMOVE))
  {
    act(AT_PLAIN, "You can't remove $p.", ch, obj, NULL, TO_CHAR);
    return FALSE;
  }

  if (obj == get_eq_char(ch, WEAR_WIELD) && (tmpobj = get_eq_char(ch, WEAR_DUAL_WIELD)) != NULL)
    tmpobj->wear_loc = WEAR_WIELD;

  unequip_char(ch, obj);

  act(AT_ACTION, "$n stops using $p.", ch, obj, NULL, TO_ROOM);
  act(AT_ACTION, "You stop using $p.", ch, obj, NULL, TO_CHAR);
  oprog_remove_trigger(ch, obj);
  /*
   * Added check in case, the trigger forces them to rewear the item
   * * --Shaddai
   */
  if (!(obj = get_eq_char(ch, iWear)))
    return TRUE;
  else
    return FALSE;
}

/*
 * See if char could be capable of dual-wielding                -Thoric
 */
bool could_dual(CHAR_DATA * ch)
{
  if (IS_NPC(ch) || ch->pcdata->learned[gsn_dual_wield])
    return TRUE;

  return FALSE;
}

bool can_dual(CHAR_DATA * ch)
{
  bool wield = FALSE, nwield = FALSE;

  if (!could_dual(ch))
    return FALSE;
  if (get_eq_char(ch, WEAR_WIELD))
    wield = TRUE;
  /*
   * Check for missile wield or dual wield 
   */
  if (get_eq_char(ch, WEAR_MISSILE_WIELD) || get_eq_char(ch, WEAR_DUAL_WIELD))
    nwield = TRUE;
  if (wield && nwield)
  {
    send_to_char("You are already wielding two weapons... grow some more arms!\r\n", ch);
    return FALSE;
  }
  if ((wield || nwield) && get_eq_char(ch, WEAR_SHIELD))
  {
    send_to_char("You cannot dual wield, you're already holding a shield!\r\n", ch);
    return FALSE;
  }
  if ((wield || nwield) && get_eq_char(ch, WEAR_HOLD))
  {
    send_to_char("You cannot hold another weapon, you're already holding something in that hand!\r\n", ch);
    return FALSE;
  }
  return TRUE;
}

/*
 * Check to see if there is room to wear another object on this location
 * (Layered clothing support)
 */
bool can_layer(CHAR_DATA * ch, OBJ_DATA * obj, short wear_loc)
{
  OBJ_DATA *otmp;
  short bitlayers = 0;
  short objlayers = obj->pIndexData->layers;

  for (otmp = ch->first_carrying; otmp; otmp = otmp->next_content)
  {
    if (otmp->wear_loc == wear_loc)
    {
      if (!otmp->pIndexData->layers)
        return FALSE;
      else
        bitlayers |= otmp->pIndexData->layers;
    }
  }

  if ((bitlayers && !objlayers) || bitlayers > objlayers)
    return FALSE;
  if (!bitlayers || ((bitlayers & ~objlayers) == bitlayers))
    return TRUE;
  return FALSE;
}

/*
 * Wear one object.
 * Optional replacement of existing objects.
 * Big repetitive code, ick.
 *
 * Restructured a bit to allow for specifying body location     -Thoric
 * & Added support for layering on certain body locations
 */
void wear_obj(CHAR_DATA * ch, OBJ_DATA * obj, bool fReplace, short wear_bit)
{
  OBJ_DATA *tmpobj = NULL;
  short bit, tmp;

  separate_obj(obj);
  if (get_trust(ch) < obj->level)
  {
    ch_printf(ch, "You must be level %d to use this object.\r\n", obj->level);
    act(AT_ACTION, "$n tries to use $p, but is too inexperienced.", ch, obj, NULL, TO_ROOM);
    return;
  }

  if (!IS_IMMORTAL(ch)
      && ((IS_OBJ_STAT(obj, ITEM_ANTI_WARRIOR) && ch->Class == CLASS_WARRIOR)
           || (IS_OBJ_STAT(obj, ITEM_ANTI_WARRIOR) && ch->Class == CLASS_CRUSADER)
           || (IS_OBJ_STAT(obj, ITEM_ANTI_MAGE) && ch->Class == CLASS_MYSTIC)
           || (IS_OBJ_STAT(obj, ITEM_ANTI_MAGE) && ch->Class == CLASS_PHILOSOPHER)
           || (IS_OBJ_STAT(obj, ITEM_ANTI_ROGUE) && ch->Class == CLASS_ROGUE)
           || (IS_OBJ_STAT(obj, ITEM_MAGIC) && ch->Class == CLASS_INVENTOR)
           || (IS_OBJ_STAT(obj, ITEM_ANTI_MAGE) && ch->Class == CLASS_SHAMAN)
           || (IS_OBJ_STAT(obj, ITEM_ANTI_WARRIOR) && ch->Class == CLASS_NATURALIST)
           || (IS_OBJ_STAT(obj, ITEM_ANTI_MAGE) && ch->Class == CLASS_BARD)
           || (IS_OBJ_STAT(obj, ITEM_ANTI_ALCHEMIST) && ch->Class == CLASS_ALCHEMIST)
           || (IS_OBJ_STAT(obj, ITEM_ANTI_GOOD) && ch->alignment > 350)
           || (IS_OBJ_STAT(obj, ITEM_ANTI_NEUTRAL) && ch->alignment >= -350 && ch->alignment <= 350)
           || (IS_OBJ_STAT(obj, ITEM_ANTI_EVIL) && ch->alignment < -350)))
  {
    act(AT_MAGIC, "You are forbidden to use that item.", ch, NULL, NULL, TO_CHAR);
    act(AT_ACTION, "$n tries to use $p, but is forbidden to do so.", ch, obj, NULL, TO_ROOM);
    return;
  }

  if (IS_OBJ_STAT(obj, ITEM_PERSONAL) && str_cmp(ch->name, obj->owner))
  {
    send_to_char("That item is personalized and belongs to someone else.\r\n", ch);
    if (obj->carried_by)
      obj_from_char(obj);
    obj_to_room(obj, ch->in_room);
    return;
  }

  if (wear_bit > -1)
  {
    bit = wear_bit;
    if (!CAN_WEAR(obj, 1 << bit))
    {
      if (fReplace)
      {
        switch (1 << bit)
        {
        case ITEM_HOLD:
          send_to_char("You cannot hold that.\r\n", ch);
          break;
        case ITEM_WIELD:
        case ITEM_MISSILE_WIELD:
          send_to_char("You cannot wield that.\r\n", ch);
          break;
        default:
          ch_printf(ch, "You cannot wear that on your %s.\r\n", w_flags[bit]);
          break;
        }
      }
      return;
    }
  }
  else
  {
    for (bit = -1, tmp = 1; tmp < 31; tmp++)
    {
      if (CAN_WEAR(obj, 1 << tmp))
      {
        bit = tmp;
        break;
      }
    }
  }

  /*
   * currently cannot have a light in non-light position 
   */
  if (obj->item_type == ITEM_LIGHT)
  {
    if (!remove_obj(ch, WEAR_LIGHT, fReplace))
      return;
    if (!oprog_use_trigger(ch, obj, NULL, NULL))
    {
      act(AT_ACTION, "$n holds $p as a light.", ch, obj, NULL, TO_ROOM);
      act(AT_ACTION, "You hold $p as your light.", ch, obj, NULL, TO_CHAR);
    }
    equip_char(ch, obj, WEAR_LIGHT);
    oprog_wear_trigger(ch, obj);
    return;
  }

  if (bit == -1)
  {
    if (fReplace)
      send_to_char("You can't wear, wield, or hold that.\r\n", ch);
    return;
  }

  switch (1 << bit)
  {
  default:
    bug("%s: uknown/unused item_wear bit %d", __func__, bit);
    if (fReplace)
      send_to_char("You can't wear, wield, or hold that.\r\n", ch);
    return;

  case ITEM_WEAR_FINGER:
    if (get_eq_char(ch, WEAR_FINGER_L)
        && get_eq_char(ch, WEAR_FINGER_R)
        && !remove_obj(ch, WEAR_FINGER_L, fReplace) && !remove_obj(ch, WEAR_FINGER_R, fReplace))
      return;

    if (!get_eq_char(ch, WEAR_FINGER_L))
    {
      if (!oprog_use_trigger(ch, obj, NULL, NULL))
      {
        act(AT_ACTION, "$n slips $s left finger into $p.", ch, obj, NULL, TO_ROOM);
        act(AT_ACTION, "You slip your left finger into $p.", ch, obj, NULL, TO_CHAR);
      }
      equip_char(ch, obj, WEAR_FINGER_L);
      oprog_wear_trigger(ch, obj);
      return;
    }

    if (!get_eq_char(ch, WEAR_FINGER_R))
    {
      if (!oprog_use_trigger(ch, obj, NULL, NULL))
      {
        act(AT_ACTION, "$n slips $s right finger into $p.", ch, obj, NULL, TO_ROOM);
        act(AT_ACTION, "You slip your right finger into $p.", ch, obj, NULL, TO_CHAR);
      }
      equip_char(ch, obj, WEAR_FINGER_R);
      oprog_wear_trigger(ch, obj);
      return;
    }

    bug("%s: no free finger.", __func__);
    send_to_char("You already wear something on both fingers.\r\n", ch);
    return;

  case ITEM_WEAR_NECK:
    if (get_eq_char(ch, WEAR_NECK_1) != NULL
        && get_eq_char(ch, WEAR_NECK_2) != NULL
        && !remove_obj(ch, WEAR_NECK_1, fReplace) && !remove_obj(ch, WEAR_NECK_2, fReplace))
      return;

    if (!get_eq_char(ch, WEAR_NECK_1))
    {
      if (!oprog_use_trigger(ch, obj, NULL, NULL))
      {
        act(AT_ACTION, "$n wears $p around $s neck.", ch, obj, NULL, TO_ROOM);
        act(AT_ACTION, "You wear $p around your neck.", ch, obj, NULL, TO_CHAR);
      }
      equip_char(ch, obj, WEAR_NECK_1);
      oprog_wear_trigger(ch, obj);
      return;
    }

    if (!get_eq_char(ch, WEAR_NECK_2))
    {
      if (!oprog_use_trigger(ch, obj, NULL, NULL))
      {
        act(AT_ACTION, "$n wears $p around $s neck.", ch, obj, NULL, TO_ROOM);
        act(AT_ACTION, "You wear $p around your neck.", ch, obj, NULL, TO_CHAR);
      }
      equip_char(ch, obj, WEAR_NECK_2);
      oprog_wear_trigger(ch, obj);
      return;
    }

    bug("%s: no free neck.", __func__);
    send_to_char("You already wear two neck items.\r\n", ch);
    return;

  case ITEM_WEAR_BODY:
    if (!can_layer(ch, obj, WEAR_BODY))
    {
      send_to_char("It won't fit overtop of what you're already wearing.\r\n", ch);
      return;
    }
    if (!oprog_use_trigger(ch, obj, NULL, NULL))
    {
      act(AT_ACTION, "$n fits $p on $s body.", ch, obj, NULL, TO_ROOM);
      act(AT_ACTION, "You fit $p on your body.", ch, obj, NULL, TO_CHAR);
    }
    equip_char(ch, obj, WEAR_BODY);
    oprog_wear_trigger(ch, obj);
    return;

  case ITEM_WEAR_HEAD:
    if (!remove_obj(ch, WEAR_HEAD, fReplace))
      return;
    if (!oprog_use_trigger(ch, obj, NULL, NULL))
    {
      act(AT_ACTION, "$n dons $p upon $s head.", ch, obj, NULL, TO_ROOM);
      act(AT_ACTION, "You don $p upon your head.", ch, obj, NULL, TO_CHAR);
    }
    equip_char(ch, obj, WEAR_HEAD);
    oprog_wear_trigger(ch, obj);
    return;

  case ITEM_WEAR_EYES:
    if (!remove_obj(ch, WEAR_EYES, fReplace))
      return;
    if (!oprog_use_trigger(ch, obj, NULL, NULL))
    {
      act(AT_ACTION, "$n places $p on $s eyes.", ch, obj, NULL, TO_ROOM);
      act(AT_ACTION, "You place $p on your eyes.", ch, obj, NULL, TO_CHAR);
    }
    equip_char(ch, obj, WEAR_EYES);
    oprog_wear_trigger(ch, obj);
    return;

  case ITEM_WEAR_FACE:
    if (!remove_obj(ch, WEAR_FACE, fReplace))
      return;
    if (!oprog_use_trigger(ch, obj, NULL, NULL))
    {
      act(AT_ACTION, "$n places $p on $s face.", ch, obj, NULL, TO_ROOM);
      act(AT_ACTION, "You place $p on your face.", ch, obj, NULL, TO_CHAR);
    }
    equip_char(ch, obj, WEAR_FACE);
    oprog_wear_trigger(ch, obj);
    return;

  case ITEM_WEAR_EARS:
    if (!remove_obj(ch, WEAR_EARS, fReplace))
      return;
    if (!oprog_use_trigger(ch, obj, NULL, NULL))
    {
      act(AT_ACTION, "$n wears $p on $s ears.", ch, obj, NULL, TO_ROOM);
      act(AT_ACTION, "You wear $p on your ears.", ch, obj, NULL, TO_CHAR);
    }
    equip_char(ch, obj, WEAR_EARS);
    oprog_wear_trigger(ch, obj);
    return;

  case ITEM_WEAR_LEGS:
    if (!can_layer(ch, obj, WEAR_LEGS))
    {
      send_to_char("It won't fit overtop of what you're already wearing.\r\n", ch);
      return;
    }
    if (!oprog_use_trigger(ch, obj, NULL, NULL))
    {
      act(AT_ACTION, "$n slips into $p.", ch, obj, NULL, TO_ROOM);
      act(AT_ACTION, "You slip into $p.", ch, obj, NULL, TO_CHAR);
    }
    equip_char(ch, obj, WEAR_LEGS);
    oprog_wear_trigger(ch, obj);
    return;

  case ITEM_WEAR_FEET:
    if (!can_layer(ch, obj, WEAR_FEET))
    {
      send_to_char("It won't fit overtop of what you're already wearing.\r\n", ch);
      return;
    }
    if (!oprog_use_trigger(ch, obj, NULL, NULL))
    {
      act(AT_ACTION, "$n wears $p on $s feet.", ch, obj, NULL, TO_ROOM);
      act(AT_ACTION, "You wear $p on your feet.", ch, obj, NULL, TO_CHAR);
    }
    equip_char(ch, obj, WEAR_FEET);
    oprog_wear_trigger(ch, obj);
    return;

  case ITEM_WEAR_HANDS:
    if (!can_layer(ch, obj, WEAR_HANDS))
    {
      send_to_char("It won't fit overtop of what you're already wearing.\r\n", ch);
      return;
    }
    if (!oprog_use_trigger(ch, obj, NULL, NULL))
    {
      act(AT_ACTION, "$n wears $p on $s hands.", ch, obj, NULL, TO_ROOM);
      act(AT_ACTION, "You wear $p on your hands.", ch, obj, NULL, TO_CHAR);
    }
    equip_char(ch, obj, WEAR_HANDS);
    oprog_wear_trigger(ch, obj);
    return;

  case ITEM_WEAR_ARMS:
    if (!can_layer(ch, obj, WEAR_ARMS))
    {
      send_to_char("It won't fit overtop of what you're already wearing.\r\n", ch);
      return;
    }
    if (!oprog_use_trigger(ch, obj, NULL, NULL))
    {
      act(AT_ACTION, "$n wears $p on $s arms.", ch, obj, NULL, TO_ROOM);
      act(AT_ACTION, "You wear $p on your arms.", ch, obj, NULL, TO_CHAR);
    }
    equip_char(ch, obj, WEAR_ARMS);
    oprog_wear_trigger(ch, obj);
    return;

  case ITEM_WEAR_ABOUT:
    if (!can_layer(ch, obj, WEAR_ABOUT))
    {
      send_to_char("It won't fit overtop of what you're already wearing.\r\n", ch);
      return;
    }
    if (!oprog_use_trigger(ch, obj, NULL, NULL))
    {
      act(AT_ACTION, "$n wears $p about $s body.", ch, obj, NULL, TO_ROOM);
      act(AT_ACTION, "You wear $p about your body.", ch, obj, NULL, TO_CHAR);
    }
    equip_char(ch, obj, WEAR_ABOUT);
    oprog_wear_trigger(ch, obj);
    return;

  case ITEM_WEAR_BACK:
    if (!remove_obj(ch, WEAR_BACK, fReplace))
      return;
    if (!oprog_use_trigger(ch, obj, NULL, NULL))
    {
      act(AT_ACTION, "$n slings $p on $s back.", ch, obj, NULL, TO_ROOM);
      act(AT_ACTION, "You sling $p on your back.", ch, obj, NULL, TO_CHAR);
    }
    equip_char(ch, obj, WEAR_BACK);
    oprog_wear_trigger(ch, obj);
    return;

  case ITEM_WEAR_WAIST:
    if (!can_layer(ch, obj, WEAR_WAIST))
    {
      send_to_char("It won't fit overtop of what you're already wearing.\r\n", ch);
      return;
    }
    if (!oprog_use_trigger(ch, obj, NULL, NULL))
    {
      act(AT_ACTION, "$n wears $p about $s waist.", ch, obj, NULL, TO_ROOM);
      act(AT_ACTION, "You wear $p about your waist.", ch, obj, NULL, TO_CHAR);
    }
    equip_char(ch, obj, WEAR_WAIST);
    oprog_wear_trigger(ch, obj);
    return;

  case ITEM_WEAR_WRIST:
    if (get_eq_char(ch, WEAR_WRIST_L)
        && get_eq_char(ch, WEAR_WRIST_R)
        && !remove_obj(ch, WEAR_WRIST_L, fReplace) && !remove_obj(ch, WEAR_WRIST_R, fReplace))
      return;

    if (!get_eq_char(ch, WEAR_WRIST_L))
    {
      if (!oprog_use_trigger(ch, obj, NULL, NULL))
      {
        act(AT_ACTION, "$n fits $p around $s left wrist.", ch, obj, NULL, TO_ROOM);
        act(AT_ACTION, "You fit $p around your left wrist.", ch, obj, NULL, TO_CHAR);
      }
      equip_char(ch, obj, WEAR_WRIST_L);
      oprog_wear_trigger(ch, obj);
      return;
    }

    if (!get_eq_char(ch, WEAR_WRIST_R))
    {
      if (!oprog_use_trigger(ch, obj, NULL, NULL))
      {
        act(AT_ACTION, "$n fits $p around $s right wrist.", ch, obj, NULL, TO_ROOM);
        act(AT_ACTION, "You fit $p around your right wrist.", ch, obj, NULL, TO_CHAR);
      }
      equip_char(ch, obj, WEAR_WRIST_R);
      oprog_wear_trigger(ch, obj);
      return;
    }

    bug("%s", "Wear_obj: no free wrist.");
    send_to_char("You already wear two wrist items.\r\n", ch);
    return;

  case ITEM_WEAR_ANKLE:
    if (get_eq_char(ch, WEAR_ANKLE_L)
        && get_eq_char(ch, WEAR_ANKLE_R)
        && !remove_obj(ch, WEAR_ANKLE_L, fReplace) && !remove_obj(ch, WEAR_ANKLE_R, fReplace))
      return;

    if (!get_eq_char(ch, WEAR_ANKLE_L))
    {
      if (!oprog_use_trigger(ch, obj, NULL, NULL))
      {
        act(AT_ACTION, "$n fits $p around $s left ankle.", ch, obj, NULL, TO_ROOM);
        act(AT_ACTION, "You fit $p around your left ankle.", ch, obj, NULL, TO_CHAR);
      }
      equip_char(ch, obj, WEAR_ANKLE_L);
      oprog_wear_trigger(ch, obj);
      return;
    }

    if (!get_eq_char(ch, WEAR_ANKLE_R))
    {
      if (!oprog_use_trigger(ch, obj, NULL, NULL))
      {
        act(AT_ACTION, "$n fits $p around $s right ankle.", ch, obj, NULL, TO_ROOM);
        act(AT_ACTION, "You fit $p around your right ankle.", ch, obj, NULL, TO_CHAR);
      }
      equip_char(ch, obj, WEAR_ANKLE_R);
      oprog_wear_trigger(ch, obj);
      return;
    }

    bug("%s: no free ankle.", __func__);
    send_to_char("You already wear two ankle items.\r\n", ch);
    return;

  case ITEM_WEAR_SHIELD:
    if (get_eq_char(ch, WEAR_DUAL_WIELD)
        || (get_eq_char(ch, WEAR_WIELD) && get_eq_char(ch, WEAR_MISSILE_WIELD))
        || (get_eq_char(ch, WEAR_WIELD) && get_eq_char(ch, WEAR_HOLD)))
    {
      if (get_eq_char(ch, WEAR_HOLD))
        send_to_char("You can't use a shield while using a weapon and holding something!\r\n", ch);
      else
        send_to_char("You can't use a shield AND two weapons!\r\n", ch);
      return;
    }
    if (!remove_obj(ch, WEAR_SHIELD, fReplace))
      return;
    if (!oprog_use_trigger(ch, obj, NULL, NULL))
    {
      act(AT_ACTION, "$n uses $p as a shield.", ch, obj, NULL, TO_ROOM);
      act(AT_ACTION, "You use $p as a shield.", ch, obj, NULL, TO_CHAR);
    }
    equip_char(ch, obj, WEAR_SHIELD);
    oprog_wear_trigger(ch, obj);
    return;

  case ITEM_MISSILE_WIELD:
  case ITEM_WIELD:
    if (!could_dual(ch))
    {
      if (!remove_obj(ch, WEAR_MISSILE_WIELD, fReplace))
        return;
      if (!remove_obj(ch, WEAR_WIELD, fReplace))
        return;
      tmpobj = NULL;
    }
    else
    {
      OBJ_DATA *mw, *dw, *hd, *sd;
      tmpobj = get_eq_char(ch, WEAR_WIELD);
      mw = get_eq_char(ch, WEAR_MISSILE_WIELD);
      dw = get_eq_char(ch, WEAR_DUAL_WIELD);
      hd = get_eq_char(ch, WEAR_HOLD);
      sd = get_eq_char(ch, WEAR_SHIELD);

      if (hd && sd)
      {
        send_to_char("You are already holding something and wearing a shield.\r\n", ch);
        return;
      }

      if (tmpobj)
      {
        if (!can_dual(ch))
          return;

        if (get_obj_weight(obj) + get_obj_weight(tmpobj) > str_app[get_curr_str(ch)].wield)
        {
          send_to_char("It is too heavy for you to wield.\r\n", ch);
          return;
        }

        if (mw || dw)
        {
          send_to_char("You're already wielding two weapons.\r\n", ch);
          return;
        }

        if (hd || sd)
        {
          send_to_char("You're already wielding a weapon AND holding something.\r\n", ch);
          return;
        }

        if (!oprog_use_trigger(ch, obj, NULL, NULL))
        {
          act(AT_ACTION, "$n dual-wields $p.", ch, obj, NULL, TO_ROOM);
          act(AT_ACTION, "You dual-wield $p.", ch, obj, NULL, TO_CHAR);
        }
        if (1 << bit == ITEM_MISSILE_WIELD)
          equip_char(ch, obj, WEAR_MISSILE_WIELD);
        else
          equip_char(ch, obj, WEAR_DUAL_WIELD);
        oprog_wear_trigger(ch, obj);
        return;
      }

      if (mw)
      {
        if (!can_dual(ch))
          return;

        if (1 << bit == ITEM_MISSILE_WIELD)
        {
          send_to_char("You're already wielding a missile weapon.\r\n", ch);
          return;
        }

        if (get_obj_weight(obj) + get_obj_weight(mw) > str_app[get_curr_str(ch)].wield)
        {
          send_to_char("It is too heavy for you to wield.\r\n", ch);
          return;
        }

        if (tmpobj || dw)
        {
          send_to_char("You're already wielding two weapons.\r\n", ch);
          return;
        }

        if (hd || sd)
        {
          send_to_char("You're already wielding a weapon AND holding something.\r\n", ch);
          return;
        }

        if (!oprog_use_trigger(ch, obj, NULL, NULL))
        {
          act(AT_ACTION, "$n wields $p.", ch, obj, NULL, TO_ROOM);
          act(AT_ACTION, "You wield $p.", ch, obj, NULL, TO_CHAR);
        }
        equip_char(ch, obj, WEAR_WIELD);
        oprog_wear_trigger(ch, obj);
        return;
      }
    }

    if (get_obj_weight(obj) > str_app[get_curr_str(ch)].wield)
    {
      send_to_char("It is too heavy for you to wield.\r\n", ch);
      return;
    }

    if (!oprog_use_trigger(ch, obj, NULL, NULL))
    {
      act(AT_ACTION, "$n wields $p.", ch, obj, NULL, TO_ROOM);
      act(AT_ACTION, "You wield $p.", ch, obj, NULL, TO_CHAR);
    }
    if (1 << bit == ITEM_MISSILE_WIELD)
      equip_char(ch, obj, WEAR_MISSILE_WIELD);
    else
      equip_char(ch, obj, WEAR_WIELD);
    oprog_wear_trigger(ch, obj);
    return;

  case ITEM_HOLD:
    if (get_eq_char(ch, WEAR_DUAL_WIELD)
        || (get_eq_char(ch, WEAR_WIELD)
             && (get_eq_char(ch, WEAR_MISSILE_WIELD) || get_eq_char(ch, WEAR_SHIELD))))
    {
      if (get_eq_char(ch, WEAR_SHIELD))
        send_to_char("You cannot hold something while using a weapon and a shield!\r\n", ch);
      else
        send_to_char("You cannot hold something AND two weapons!\r\n", ch);
      return;
    }
    if (!remove_obj(ch, WEAR_HOLD, fReplace))
      return;
    if (obj->item_type == ITEM_WAND
        || obj->item_type == ITEM_STAFF
        || obj->item_type == ITEM_FOOD
        || obj->item_type == ITEM_COOK
        || obj->item_type == ITEM_PILL
        || obj->item_type == ITEM_POTION
        || obj->item_type == ITEM_SCROLL
        || obj->item_type == ITEM_DRINK_CON
        || obj->item_type == ITEM_PIPE
        || obj->item_type == ITEM_HERB || obj->item_type == ITEM_KEY || !oprog_use_trigger(ch, obj, NULL, NULL))
    {
      act(AT_ACTION, "$n holds $p in $s hands.", ch, obj, NULL, TO_ROOM);
      act(AT_ACTION, "You hold $p in your hands.", ch, obj, NULL, TO_CHAR);
    }
    equip_char(ch, obj, WEAR_HOLD);
    oprog_wear_trigger(ch, obj);
    return;
  }
}

void do_wear(CHAR_DATA* ch, const char* argument)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  OBJ_DATA *obj;
  short wear_bit;

  argument = one_argument(argument, arg1);
  argument = one_argument(argument, arg2);
  if ((!str_cmp(arg2, "on") || !str_cmp(arg2, "upon") || !str_cmp(arg2, "around")) && argument[0] != '\0')
    argument = one_argument(argument, arg2);

  if (arg1[0] == '\0')
  {
    send_to_char("Wear, wield, or hold what?\r\n", ch);
    return;
  }

  if (ms_find_obj(ch))
    return;

  if (!str_cmp(arg1, "all"))
  {
    OBJ_DATA *obj_next;

    for (obj = ch->first_carrying; obj; obj = obj_next)
    {
      obj_next = obj->next_content;
      if (obj->wear_loc == WEAR_NONE && can_see_obj(ch, obj))
      {
        wear_obj(ch, obj, FALSE, -1);
        if (char_died(ch))
          return;
      }
    }
    return;
  }
  else
  {
    if ((obj = get_obj_carry(ch, arg1)) == NULL)
    {
      send_to_char("You do not have that item.\r\n", ch);
      return;
    }
    if (arg2[0] != '\0')
      wear_bit = get_wflag(arg2);
    else
      wear_bit = -1;
    wear_obj(ch, obj, TRUE, wear_bit);
  }

  return;
}



void do_remove(CHAR_DATA* ch, const char* argument)
{
  char arg[MAX_INPUT_LENGTH];
  OBJ_DATA *obj, *obj_next;


  one_argument(argument, arg);

  if (arg[0] == '\0')
  {
    send_to_char("Remove what?\r\n", ch);
    return;
  }

  if (ms_find_obj(ch))
    return;

  if (!str_cmp(arg, "all"))  /* SB Remove all */
  {
    for (obj = ch->first_carrying; obj != NULL; obj = obj_next)
    {
      obj_next = obj->next_content;
      if (obj->wear_loc != WEAR_NONE && can_see_obj(ch, obj))
        remove_obj(ch, obj->wear_loc, TRUE);
    }
    return;
  }

  if ((obj = get_obj_wear(ch, arg)) == NULL)
  {
    send_to_char("You are not using that item.\r\n", ch);
    return;
  }
  if ((obj_next = get_eq_char(ch, obj->wear_loc)) != obj)
  {
    act(AT_PLAIN, "You must remove $p first.", ch, obj_next, NULL, TO_CHAR);
    return;
  }

  remove_obj(ch, obj->wear_loc, TRUE);
  return;
}


void do_bury(CHAR_DATA* ch, const char* argument)
{
  char arg[MAX_INPUT_LENGTH];
  OBJ_DATA *obj;
  bool shovel;
  short move;

  one_argument(argument, arg);

  if (arg[0] == '\0')
  {
    send_to_char("What do you wish to bury?\r\n", ch);
    return;
  }

  if (ms_find_obj(ch))
    return;

  shovel = FALSE;
  for (obj = ch->first_carrying; obj; obj = obj->next_content)
    if (obj->item_type == ITEM_SHOVEL)
    {
      shovel = TRUE;
      break;
    }

  obj = get_obj_list_rev(ch, arg, ch->in_room->last_content);
  if (!obj)
  {
    send_to_char("You can't find it.\r\n", ch);
    return;
  }

  separate_obj(obj);

  if (!CAN_WEAR(obj, ITEM_TAKE))
  {
    act(AT_PLAIN, "You cannot bury $p.", ch, obj, NULL, TO_CHAR);
    return;
  }

  switch(ch->in_room->sector_type)
  {
  case SECT_CITY:
  case SECT_INSIDE:
    send_to_char("The floor is too hard to dig through.\r\n", ch);
    return;
  case SECT_FRESH_WATER:
  case SECT_SALT_WATER:
  case SECT_WATER_NOSWIM:
  case SECT_UNDERWATER:
    send_to_char("You cannot bury something here.\r\n", ch);
    return;
  case SECT_AIR:
    send_to_char("What? In the air?!\r\n", ch);
    return;
  }

  if (obj->weight > (UMAX(5, (can_carry_w(ch) / 10))) && !shovel)
  {
    send_to_char("You'd need a shovel to bury something that big.\r\n", ch);
    return;
  }

  move = (obj->weight * 50 * (shovel ? 1 : 5)) / UMAX(1, can_carry_w(ch));
  move = URANGE(2, move, 1000);
  if (move > ch->move)
  {
    send_to_char("You don't have the energy to bury something of that size.\r\n", ch);
    return;
  }
  ch->move -= move;
  if (obj->item_type == ITEM_CORPSE_NPC || obj->item_type == ITEM_CORPSE_PC)
    adjust_favor(ch, 6, 1);

  act(AT_ACTION, "You solemnly bury $p...", ch, obj, NULL, TO_CHAR);
  act(AT_ACTION, "$n solemnly buries $p...", ch, obj, NULL, TO_ROOM);
  xSET_BIT(obj->extra_flags, ITEM_BURIED);
  WAIT_STATE(ch, URANGE(10, move / 2, 100));
  return;
}

void do_sacrifice(CHAR_DATA* ch, const char* argument)
{
  char arg[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  char name[50];
  OBJ_DATA *obj;

  one_argument(argument, arg);

  if (arg[0] == '\0' || !str_cmp(arg, ch->name))
  {
    act(AT_ACTION, "$n offers $mself to $s deity, who graciously declines.", ch, NULL, NULL, TO_ROOM);
    send_to_char("Your deity appreciates your offer and may accept it later.\r\n", ch);
    return;
  }

  if (ms_find_obj(ch))
    return;

  obj = get_obj_list_rev(ch, arg, ch->in_room->last_content);
  if (!obj)
  {
    send_to_char("You can't find it.\r\n", ch);
    return;
  }

  separate_obj(obj);

  if (!CAN_WEAR(obj, ITEM_TAKE))
  {
    act(AT_PLAIN, "$p is not an acceptable sacrifice.", ch, obj, NULL, TO_CHAR);
    return;
  }

  if (xIS_SET(ch->in_room->room_flags, ROOM_CLANSTOREROOM))
  {
    send_to_char("The gods would not accept such a foolish sacrifice.\r\n", ch);
    return;
  }

  if (IS_SET(obj->magic_flags, ITEM_PKDISARMED) && !IS_NPC(ch) && !IS_IMMORTAL(ch))
  {
    if (CAN_PKILL(ch) && !get_timer(ch, TIMER_PKILLED))
    {
      if (ch->level - obj->value[5] > 5 || obj->value[5] - ch->level > 5)
      {
        send_to_char_color("\r\n&bA godly force freezes your outstretched hand.\r\n", ch);
        return;
      }
    }
  }
  if (!IS_NPC(ch) && ch->pcdata->deity && ch->pcdata->deity->name[0] != '\0')
  {
    mudstrlcpy(name, ch->pcdata->deity->name, 50);
  }
  else if (!IS_NPC(ch) && IS_GUILDED(ch) && sysdata.guild_overseer[0] != '\0')
  {
    mudstrlcpy(name, sysdata.guild_overseer, 50);
  }
  else if (!IS_NPC(ch) && ch->pcdata->clan && ch->pcdata->clan->deity[0] != '\0')
  {
    mudstrlcpy(name, ch->pcdata->clan->deity, 50);
  }
  else
  {
    mudstrlcpy(name, "Thoric", 50);
  }
  ch->gold += 1;
  if (obj->item_type == ITEM_CORPSE_NPC || obj->item_type == ITEM_CORPSE_PC)
    adjust_favor(ch, 5, 1);
  ch_printf(ch, "%s gives you one gold coin for your sacrifice.\r\n", name);

  if (obj->item_type == ITEM_PAPER)
    snprintf(buf, MAX_STRING_LENGTH, "$n sacrifices a note to %s.", name);
  else
    snprintf(buf, MAX_STRING_LENGTH, "$n sacrifices $p to %s.", name);
  act(AT_ACTION, buf, ch, obj, NULL, TO_ROOM);

  if (obj->item_type != ITEM_PAPER)
    oprog_sac_trigger(ch, obj);

  if (obj_extracted(obj))
  {
    if (xIS_SET(ch->in_room->room_flags, ROOM_HOUSE))
      save_house_by_vnum(ch->in_room->vnum); /* Prevent House Object Duplication */
    return;
  }

  if (cur_obj == obj->serial)
    global_objcode = rOBJ_SACCED;
  /* Separate again.  There was a problem here with sac_progs in that if the
     object respawned a copy of itself, it would sometimes link it to the
     one that was being extracted, resulting in them both getting that evil
     extraction :) -- Alty */
  separate_obj(obj);
  extract_obj(obj);

  if (xIS_SET(ch->in_room->room_flags, ROOM_HOUSE))
    save_house_by_vnum(ch->in_room->vnum); /* Prevent House Object Duplication */
  return;
}

void do_brandish(CHAR_DATA* ch, const char* argument)
{
  CHAR_DATA *vch;
  CHAR_DATA *vch_next;
  OBJ_DATA *staff;
  ch_ret retcode;
  int sn;

  if ((staff = get_eq_char(ch, WEAR_HOLD)) == NULL)
  {
    send_to_char("You hold nothing in your hand.\r\n", ch);
    return;
  }

  if (staff->item_type != ITEM_STAFF)
  {
    send_to_char("You can brandish only with a staff.\r\n", ch);
    return;
  }

  if ((sn = staff->value[3]) < 0 || sn >= num_skills || skill_table[sn]->spell_fun == NULL)
  {
    bug("%s: bad sn %d.", __func__, sn);
    return;
  }

  WAIT_STATE(ch, 2 * PULSE_VIOLENCE);

  if (staff->value[2] > 0)
  {
    if (!oprog_use_trigger(ch, staff, NULL, NULL))
    {
      act(AT_MAGIC, "$n brandishes $p.", ch, staff, NULL, TO_ROOM);
      act(AT_MAGIC, "You brandish $p.", ch, staff, NULL, TO_CHAR);
    }

    for (vch = ch->in_room->first_person; vch; vch = vch_next)
    {
      vch_next = vch->next_in_room;

      if (!IS_NPC(vch) && xIS_SET(vch->act, PLR_WIZINVIS) && vch->pcdata->wizinvis >= LEVEL_IMMORTAL)
        continue;
      else
        switch (skill_table[sn]->target)
        {
        default:
          bug("%s: bad target for sn %d.", __func__, sn);
          return;

        case TAR_IGNORE:
          if (vch != ch)
            continue;
          break;

        case TAR_CHAR_OFFENSIVE:
          if (IS_NPC(ch) ? IS_NPC(vch) : !IS_NPC(vch))
            continue;
          break;

        case TAR_CHAR_DEFENSIVE:
          if (IS_NPC(ch) ? !IS_NPC(vch) : IS_NPC(vch))
            continue;
          break;

        case TAR_CHAR_SELF:
          if (vch != ch)
            continue;
          break;
        }

      retcode = obj_cast_spell(staff->value[3], staff->value[0], ch, vch, NULL);
      if (retcode == rCHAR_DIED || retcode == rBOTH_DIED)
      {
        bug("%s: char died", __func__);
        return;
      }
    }
  }

  if (--staff->value[2] <= 0)
  {
    act(AT_MAGIC, "$p blazes bright and vanishes from $n's hands!", ch, staff, NULL, TO_ROOM);
    act(AT_MAGIC, "$p blazes bright and is gone!", ch, staff, NULL, TO_CHAR);
    if (staff->serial == cur_obj)
      global_objcode = rOBJ_USED;
    extract_obj(staff);
  }

  return;
}

void do_zap(CHAR_DATA* ch, const char* argument)
{
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *victim;
  OBJ_DATA *wand;
  OBJ_DATA *obj;
  ch_ret retcode;

  one_argument(argument, arg);
  if (arg[0] == '\0' && !ch->fighting)
  {
    send_to_char("Zap whom or what?\r\n", ch);
    return;
  }

  if ((wand = get_eq_char(ch, WEAR_HOLD)) == NULL)
  {
    send_to_char("You hold nothing in your hand.\r\n", ch);
    return;
  }

  if (wand->item_type != ITEM_WAND)
  {
    send_to_char("You can zap only with a wand.\r\n", ch);
    return;
  }

  obj = NULL;
  if (arg[0] == '\0')
  {
    if (ch->fighting)
    {
      victim = who_fighting(ch);
    }
    else
    {
      send_to_char("Zap whom or what?\r\n", ch);
      return;
    }
  }
  else
  {
    if ((victim = get_char_room(ch, arg)) == NULL && (obj = get_obj_here(ch, arg)) == NULL)
    {
      send_to_char("You can't find it.\r\n", ch);
      return;
    }
  }

  WAIT_STATE(ch, 1 * PULSE_VIOLENCE);

  if (wand->value[2] > 0)
  {
    if (victim)
    {
      if (!oprog_use_trigger(ch, wand, victim, NULL))
      {
        act(AT_MAGIC, "$n aims $p at $N.", ch, wand, victim, TO_ROOM);
        act(AT_MAGIC, "You aim $p at $N.", ch, wand, victim, TO_CHAR);
      }
    }
    else
    {
      if (!oprog_use_trigger(ch, wand, NULL, obj))
      {
        act(AT_MAGIC, "$n aims $p at $P.", ch, wand, obj, TO_ROOM);
        act(AT_MAGIC, "You aim $p at $P.", ch, wand, obj, TO_CHAR);
      }
    }

    retcode = obj_cast_spell(wand->value[3], wand->value[0], ch, victim, obj);
    if (retcode == rCHAR_DIED || retcode == rBOTH_DIED)
    {
      bug("%s", "do_zap: char died");
      return;
    }
  }

  if (--wand->value[2] <= 0)
  {
    act(AT_MAGIC, "$p explodes into fragments.", ch, wand, NULL, TO_ROOM);
    act(AT_MAGIC, "$p explodes into fragments.", ch, wand, NULL, TO_CHAR);
    if (wand->serial == cur_obj)
      global_objcode = rOBJ_USED;
    extract_obj(wand);
  }
  return;
}

/* put an item on auction, or see the stats on the current item or bet */
void do_auction(CHAR_DATA* ch, const char* argument)
{
  OBJ_DATA *obj;
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char arg3[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  int i;

  argument = one_argument(argument, arg1);
  argument = one_argument(argument, arg2);
  argument = one_argument(argument, arg3);

  set_char_color(AT_LBLUE, ch);

  if (IS_NPC(ch))   /* NPC can be extracted at any time and thus can't auction! */
    return;

  if (ch->level < 3)
  {
    send_to_char("You must be at least level three to use the auction...\r\n", ch);
    return;
  }

  if ((time_info.hour > 18 || time_info.hour < 9) && auction->item == NULL && !IS_IMMORTAL(ch))
  {
    send_to_char("\r\nThe auctioneer works between the hours of 9 AM and 6 PM\r\n", ch);
    return;
  }

  if (arg1[0] == '\0')
  {
    if (auction->item != NULL)
    {
      AFFECT_DATA *paf;
      obj = auction->item;

      /*
       * show item data here 
       */
      set_char_color(AT_BLUE, ch);
      if (auction->bet > 0)
        ch_printf(ch, "\r\nCurrent bid on this item is %s gold.\r\n", num_punct(auction->bet));
      else
        send_to_char("\r\nNo bids on this item have been received.\r\n", ch);
/*          spell_identify (0, LEVEL_HERO - 1, ch, auction->item); */

      set_char_color(AT_LBLUE, ch);
      ch_printf(ch,
                 "Object '%s' is %s, special properties: %s\r\nIts weight is %d, value is %d, and level is %d.\r\n",
                 obj->name, aoran(item_type_name(obj)), extra_bit_name(&obj->extra_flags),
/*              magic_bit_name(obj->magic_flags), -- currently unused */
                 obj->weight, obj->cost, obj->level);
      if (obj->item_type != ITEM_LIGHT && obj->wear_flags - 1 > 0)
        ch_printf(ch, "Item's wear location: %s\r\n", flag_string(obj->wear_flags - 1, w_flags));

      set_char_color(AT_BLUE, ch);

      switch (obj->item_type)
      {
      case ITEM_CONTAINER:
      case ITEM_KEYRING:
      case ITEM_QUIVER:
        ch_printf(ch, "%s appears to %s.\r\n", capitalize(obj->short_descr),
                   obj->value[0] < 76 ? "have a small capacity" :
                   obj->value[0] < 150 ? "have a small to medium capacity" :
                   obj->value[0] < 300 ? "have a medium capacity" :
                   obj->value[0] < 500 ? "have a medium to large capacity" :
                   obj->value[0] < 751 ? "have a large capacity" : "have a giant capacity");
        break;

      case ITEM_PILL:
      case ITEM_SCROLL:
      case ITEM_POTION:
        ch_printf(ch, "Level %d spells of:", obj->value[0]);

        if (obj->value[1] >= 0 && obj->value[1] < num_skills)
        {
          send_to_char(" '", ch);
          send_to_char(skill_table[obj->value[1]]->name, ch);
          send_to_char("'", ch);
        }

        if (obj->value[2] >= 0 && obj->value[2] < num_skills)
        {
          send_to_char(" '", ch);
          send_to_char(skill_table[obj->value[2]]->name, ch);
          send_to_char("'", ch);
        }

        if (obj->value[3] >= 0 && obj->value[3] < num_skills)
        {
          send_to_char(" '", ch);
          send_to_char(skill_table[obj->value[3]]->name, ch);
          send_to_char("'", ch);
        }

        send_to_char(".\r\n", ch);
        break;

      case ITEM_WAND:
      case ITEM_STAFF:
        ch_printf(ch, "Has %d(%d) charges of level %d", obj->value[1], obj->value[2], obj->value[0]);

        if (obj->value[3] >= 0 && obj->value[3] < num_skills)
        {
          send_to_char(" '", ch);
          send_to_char(skill_table[obj->value[3]]->name, ch);
          send_to_char("'", ch);
        }

        send_to_char(".\r\n", ch);
        break;

      case ITEM_MISSILE_WEAPON:
      case ITEM_WEAPON:
        ch_printf(ch, "Damage is %d to %d (average %d).%s\r\n",
                   obj->value[1], obj->value[2],
                   (obj->value[1] + obj->value[2]) / 2,
                   IS_OBJ_STAT(obj, ITEM_POISONED) ? "\r\nThis weapon is poisoned." : "");
        break;

      case ITEM_ARMOR:
        ch_printf(ch, "Armor class is %d.\r\n", obj->value[0]);
        break;
      }

      for (paf = obj->pIndexData->first_affect; paf; paf = paf->next)
        showaffect(ch, paf);

      for (paf = obj->first_affect; paf; paf = paf->next)
        showaffect(ch, paf);
      if ((obj->item_type == ITEM_CONTAINER || obj->item_type == ITEM_KEYRING
            || obj->item_type == ITEM_QUIVER) && obj->first_content)
      {
        set_char_color(AT_OBJECT, ch);
        send_to_char("Contents:\r\n", ch);
        show_list_to_char(obj->first_content, ch, TRUE, FALSE);
      }

      if (IS_IMMORTAL(ch))
      {
        ch_printf(ch, "Seller: %s.  Bidder: %s.  Round: %d.\r\n",
                   auction->seller->name, auction->buyer->name, (auction->going + 1));
        ch_printf(ch, "Time left in round: %d.\r\n", auction->pulse);
      }
      return;
    }
    else
    {
      set_char_color(AT_LBLUE, ch);
      send_to_char("\r\nThere is nothing being auctioned right now.  What would you like to auction?\r\n", ch);
      return;
    }
  }

  if (IS_IMMORTAL(ch) && !str_cmp(arg1, "stop"))
  {
    if (auction->item == NULL)
    {
      send_to_char("There is no auction to stop.\r\n", ch);
      return;
    }
    else  /* stop the auction */
    {
      set_char_color(AT_LBLUE, ch);
      snprintf(buf, MAX_STRING_LENGTH, "Sale of %s has been stopped by an Immortal.", auction->item->short_descr);
      talk_auction(buf);
      obj_to_char(auction->item, auction->seller);
      if (IS_SET(sysdata.save_flags, SV_AUCTION))
        save_char_obj(auction->seller);
      auction->item = NULL;
      if (auction->buyer != NULL && auction->buyer != auction->seller) /* return money to the buyer */
      {
        auction->buyer->gold += auction->bet;
        send_to_char("Your money has been returned.\r\n", auction->buyer);
      }
      return;
    }
  }

  if (!str_cmp(arg1, "bid"))
  {
    if (auction->item != NULL)
    {
      int newbet;

      if (ch->level < auction->item->level)
      {
        send_to_char("This object's level is too high for your use.\r\n", ch);
        return;
      }

      if (ch == auction->seller)
      {
        send_to_char("You can't bid on your own item!\r\n", ch);
        return;
      }

      /*
       * make - perhaps - a bet now 
       */
      if (arg2[0] == '\0')
      {
        send_to_char("Bid how much?\r\n", ch);
        return;
      }

      newbet = parsebet(auction->bet, arg2);
/*          ch_printf(ch, "Bid: %d\r\n",newbet);        */

      if (newbet < auction->starting)
      {
        send_to_char("You must place a bid that is higher than the starting bet.\r\n", ch);
        return;
      }

      /*
       * to avoid slow auction, use a bigger amount than 100 if the bet
       * is higher up - changed to 10000 for our high economy
       */
      if (newbet < auction->bet || newbet < (auction->bet + 10000))
      {
        send_to_char("You must at least bid 10000 coins over the current bid.\r\n", ch);
        return;
      }

      if (newbet > ch->gold)
      {
        send_to_char("You don't have that much money!\r\n", ch);
        return;
      }

      if (newbet > 2000000000)
      {
        send_to_char("You can't bid over 2 billion coins.\r\n", ch);
        return;
      }

      /*
       * Is it the item they really want to bid on? --Shaddai 
       */
      if (arg3[0] != '\0' && !nifty_is_name(arg3, auction->item->name))
      {
        send_to_char("That item is not being auctioned right now.\r\n", ch);
        return;
      }

      /*
       * the actual bet is OK! 
       * return the gold to the last buyer, if one exists 
       */
      if (auction->buyer != NULL && auction->buyer != auction->seller)
        auction->buyer->gold += auction->bet;

      ch->gold -= newbet;  /* substract the gold - important :) */
      if (IS_SET(sysdata.save_flags, SV_AUCTION))
        save_char_obj(ch);
      auction->buyer = ch;
      auction->bet = newbet;
      auction->going = 0;
      auction->pulse = PULSE_AUCTION;  /* start the auction over again */
      snprintf(buf, MAX_STRING_LENGTH, "A bid of %s gold has been received on %s.", num_punct(newbet), auction->item->short_descr);
      talk_auction(buf);
      return;
    }
    else
    {
      send_to_char("There isn't anything being auctioned right now.\r\n", ch);
      return;
    }
  }

  /*
   * finally... 
   */
  if (ms_find_obj(ch))
    return;

  obj = get_obj_carry(ch, arg1); /* does char have the item ? */

  if (obj == NULL)
  {
    send_to_char("You aren't carrying that.\r\n", ch);
    return;
  }

  if (obj->timer > 0)
  {
    send_to_char("You can't auction objects that are decaying.\r\n", ch);
    return;
  }

  if (IS_OBJ_STAT(obj, ITEM_CLANOBJECT))
  {
    send_to_char("You can't auction clan items.\r\n", ch);
    return;
  }

  if (IS_OBJ_STAT(obj, ITEM_PERMANENT))
  {
    send_to_char("This item cannot leave your possession.\r\n", ch);
    return;
  }

  if (IS_OBJ_STAT(obj, ITEM_PERSONAL))
  {
    send_to_char("Personal items may not be auctioned.\r\n", ch);
    return;
  }

  /*
   * prevent repeat auction items 
   */
  for (i = 0; i < AUCTION_MEM && auction->history[i]; i++)
  {
    if (auction->history[i] == obj->pIndexData)
    {
      send_to_char("Such an item has been auctioned recently, try again later.\r\n", ch);
      return;
    }
  }

  if (arg2[0] == '\0')
  {
    auction->starting = 0;
    mudstrlcpy(arg2, "0", MAX_INPUT_LENGTH);
  }

  if (!is_number(arg2))
  {
    send_to_char("You must input a number at which to start the auction.\r\n", ch);
    return;
  }

  if (atoi(arg2) < 0)
  {
    send_to_char("You can't auction something for less than 0 gold!\r\n", ch);
    return;
  }

  if (auction->item == NULL)
  {
    switch (obj->item_type)
    {

    default:
      act(AT_TELL, "You cannot auction $Ts.", ch, NULL, item_type_name(obj), TO_CHAR);
      return;

/* insert any more item types here... items with a timer MAY NOT BE 
   AUCTIONED! 
*/
    case ITEM_PAPER:
    case ITEM_LIGHT:
    case ITEM_TREASURE:
    case ITEM_POTION:
    case ITEM_KEYRING:
    case ITEM_QUIVER:
    case ITEM_DRINK_CON:
    case ITEM_FOOD:
    case ITEM_COOK:
    case ITEM_PEN:
    case ITEM_BOAT:
    case ITEM_PILL:
    case ITEM_PIPE:
    case ITEM_HERB_CON:
    case ITEM_INCENSE:
    case ITEM_FIRE:
    case ITEM_RUNEPOUCH:
    case ITEM_MAP:
    case ITEM_BOOK:
    case ITEM_RUNE:
    case ITEM_MATCH:
    case ITEM_HERB:
    case ITEM_WEAPON:
    case ITEM_MISSILE_WEAPON:
    case ITEM_ARMOR:
    case ITEM_STAFF:
    case ITEM_WAND:
    case ITEM_SCROLL:
      separate_obj(obj);
      obj_from_char(obj);
      if (IS_SET(sysdata.save_flags, SV_AUCTION))
        save_char_obj(ch);
      auction->item = obj;
      auction->bet = 0;
      auction->buyer = ch;
      auction->seller = ch;
      auction->pulse = PULSE_AUCTION;
      auction->going = 0;
      auction->starting = atoi(arg2);

      /*
       * add the new item to the history 
       */
      if (AUCTION_MEM > 0)
      {
        memmove((char *)auction->history + sizeof(OBJ_INDEX_DATA *),
                 auction->history, (AUCTION_MEM - 1) * sizeof(OBJ_INDEX_DATA *));
        auction->history[0] = obj->pIndexData;
      }

      /*
       * reset the history timer 
       */
      auction->hist_timer = 0;

      if (auction->starting > 0)
        auction->bet = auction->starting;
      snprintf(buf, MAX_STRING_LENGTH, "A new item is being auctioned: %s at %d gold.", obj->short_descr,
                auction->starting);
      talk_auction(buf);

      return;

    }  /* switch */
  }
  else
  {
    act(AT_TELL, "Try again later - $p is being auctioned right now!", ch, auction->item, NULL, TO_CHAR);
    if (!IS_IMMORTAL(ch))
      WAIT_STATE(ch, PULSE_VIOLENCE);
    return;
  }
}

/* Make objects in rooms that are nofloor fall - Scryn 1/23/96 */
void obj_fall(OBJ_DATA * obj, bool through)
{
  EXIT_DATA *pexit;
  ROOM_INDEX_DATA *to_room;
  static int fall_count;
  static bool is_falling; /* Stop loops from the call to obj_to_room()  -- Altrag */

  if (!obj->in_room || is_falling)
    return;

  if (fall_count > 30)
  {
    bug("%s", "object falling in loop more than 30 times");
    extract_obj(obj);
    fall_count = 0;
    return;
  }

  if (xIS_SET(obj->in_room->room_flags, ROOM_NOFLOOR) && CAN_GO(obj, DIR_DOWN) && !IS_OBJ_STAT(obj, ITEM_MAGIC))
  {

    pexit = get_exit(obj->in_room, DIR_DOWN);
    to_room = pexit->to_room;

    if (through)
      fall_count++;
    else
      fall_count = 0;

    if (obj->in_room == to_room)
    {
      bug("Object falling into same room, room %d", to_room->vnum);
      extract_obj(obj);
      return;
    }

    if (obj->in_room->first_person)
    {
      act(AT_PLAIN, "$p falls far below...", obj->in_room->first_person, obj, NULL, TO_ROOM);
      act(AT_PLAIN, "$p falls far below...", obj->in_room->first_person, obj, NULL, TO_CHAR);
    }
    obj_from_room(obj);
    is_falling = TRUE;
    obj = obj_to_room(obj, to_room);
    is_falling = FALSE;

    if (obj->in_room->first_person)
    {
      act(AT_PLAIN, "$p falls from above...", obj->in_room->first_person, obj, NULL, TO_ROOM);
      act(AT_PLAIN, "$p falls from above...", obj->in_room->first_person, obj, NULL, TO_CHAR);
    }

    if (!xIS_SET(obj->in_room->room_flags, ROOM_NOFLOOR) && through)
    {
/*              int dam = (int)9.81*sqrt(fall_count*2/9.81)*obj->weight/2;
 */ int dam = fall_count * obj->weight / 2;
/*
 * Damage players 
 */
if (obj->in_room->first_person && number_percent() > 15)
{
  CHAR_DATA *rch;
  CHAR_DATA *vch = NULL;
  int chcnt = 0;

  for (rch = obj->in_room->first_person; rch; rch = rch->next_in_room, chcnt++)
    if (number_range(0, chcnt) == 0)
      vch = rch;
  act(AT_WHITE, "$p falls on $n!", vch, obj, NULL, TO_ROOM);
  act(AT_WHITE, "$p falls on you!", vch, obj, NULL, TO_CHAR);

  damage(vch, vch, dam * vch->level, TYPE_UNDEFINED);
}
/*
 * Damage objects 
 */
switch (obj->item_type)
{
case ITEM_WEAPON:
case ITEM_ARMOR:
  if ((obj->value[0] - dam) <= 0)
  {
    if (obj->in_room->first_person)
    {
      act(AT_PLAIN, "$p is destroyed by the fall!", obj->in_room->first_person, obj, NULL, TO_ROOM);
      act(AT_PLAIN, "$p is destroyed by the fall!", obj->in_room->first_person, obj, NULL, TO_CHAR);
    }
    make_scraps(obj);
  }
  else
    obj->value[0] -= dam;
  break;
default:
  if ((dam * 15) > get_obj_resistance(obj))
  {
    if (obj->in_room->first_person)
    {
      act(AT_PLAIN, "$p is destroyed by the fall!", obj->in_room->first_person, obj, NULL, TO_ROOM);
      act(AT_PLAIN, "$p is destroyed by the fall!", obj->in_room->first_person, obj, NULL, TO_CHAR);
    }
    make_scraps(obj);
  }
  break;
}
    }
    obj_fall(obj, TRUE);
  }
  return;
}

/* Scryn, by request of Darkur, 12/04/98 */
/* Reworked recursive_note_find to fix crash bug when the note was left 
 * blank.  7/6/98 -- Shaddai
 */
OBJ_DATA *recursive_note_find(OBJ_DATA * obj, const char *argument)
{
  OBJ_DATA *returned_obj;
  bool match = TRUE;
  const char *argcopy;
  const char *subject;

  char arg[MAX_INPUT_LENGTH];
  char subj[MAX_STRING_LENGTH];

  if (!obj)
    return NULL;

  switch (obj->item_type)
  {
  case ITEM_PAPER:

    if ((subject = get_extra_descr("_subject_", obj->first_extradesc)) == NULL)
      break;
    snprintf(subj, MAX_STRING_LENGTH, "%s", strlower(subject));
    subject = strlower(subj);

    argcopy = argument;

    while (match)
    {
      argcopy = one_argument(argcopy, arg);

      if (arg[0] == '\0')
        break;

      if (!strstr(subject, arg))
        match = FALSE;
    }


    if (match)
      return obj;
    break;

  case ITEM_CONTAINER:
  case ITEM_CORPSE_NPC:
  case ITEM_CORPSE_PC:
    if (obj->first_content)
    {
      returned_obj = recursive_note_find(obj->first_content, argument);
      if (returned_obj)
        return returned_obj;
    }
    break;

  default:
    break;
  }

  return recursive_note_find(obj->next_content, argument);
}

void do_findnote(CHAR_DATA* ch, const char* argument)
{
  OBJ_DATA *obj;

  if (IS_NPC(ch))
  {
    send_to_char("Huh?\r\n", ch);
    return;
  }

  if (argument[0] == '\0')
  {
    send_to_char("You must specify at least one keyword.\r\n", ch);
    return;
  }

  obj = recursive_note_find(ch->first_carrying, argument);

  if (obj)
  {
    if (obj->in_obj)
    {
      obj_from_obj(obj);
      obj = obj_to_char(obj, ch);
    }
    wear_obj(ch, obj, TRUE, -1);
  }
  else
    send_to_char("Note not found.\r\n", ch);
  return;
}

const char *get_chance_verb(OBJ_DATA * obj)
{
  return (obj->action_desc[0] != '\0') ? obj->action_desc : "roll$q";
}

const char *get_ed_number(OBJ_DATA * obj, int number)
{
  EXTRA_DESCR_DATA *ed;
  int count;

  for (ed = obj->first_extradesc, count = 1; ed; ed = ed->next, count++)
  {
    if (count == number)
      return ed->description;
  }

  return NULL;
}

void do_rolldie(CHAR_DATA* ch, const char* argument)
{
  OBJ_DATA *die;

  char output_string[MAX_STRING_LENGTH];
  char roll_string[MAX_STRING_LENGTH];
  char total_string[MAX_STRING_LENGTH];

  const char *verb;

/*  char* face_string = NULL;
    char** face_table = NULL;*/
  int rollsum = 0;
  int roll_count = 0;

  int numsides;
  int numrolls;

  bool *face_seen_table = NULL;

  if ((die = get_eq_char(ch, WEAR_HOLD)) == NULL || die->item_type != ITEM_CHANCE)
  {
    ch_printf(ch, "You must be holding an item of chance!\r\n");
    return;
  }

  numrolls = (is_number(argument)) ? atoi(argument) : 1;
  verb = get_chance_verb(die);

  if (numrolls > 100)
  {
    ch_printf(ch, "You can't %s more than 100 times!\r\n", verb);
    return;
  }

  numsides = die->value[0];

  if (numsides <= 1)
  {
    ch_printf(ch, "There is no element of chance in this game!\r\n");
    return;
  }

  if (die->value[3] == 1)
  {
    if (numrolls > numsides)
    {
      ch_printf(ch, "Nice try, but you can only %s %d times.\r\n", verb, numsides);
      return;
    }
    face_seen_table = (bool *) calloc(numsides, sizeof(bool));
    if (!face_seen_table)
    {
      bug("%s", "do_rolldie: cannot allocate memory for face_seen_table array, terminating.\r\n");
      return;
    }
  }

  snprintf(roll_string, MAX_STRING_LENGTH, "%s", " ");

  while (roll_count++ < numrolls)
  {
    int current_roll;
    char current_roll_string[MAX_STRING_LENGTH];

    do
    {
      current_roll = number_range(1, numsides);
    }
    while (die->value[3] == 1 && face_seen_table[current_roll - 1] == TRUE);

    if (die->value[3] == 1)
      face_seen_table[current_roll - 1] = TRUE;

    rollsum += current_roll;

    if (roll_count > 1)
      mudstrlcat(roll_string, ", ", MAX_STRING_LENGTH);
    if (numrolls > 1 && roll_count == numrolls)
      mudstrlcat(roll_string, "and ", MAX_STRING_LENGTH);

    if (die->value[1] == 1)
    {
      const char *face_name = get_ed_number(die, current_roll);
      if (face_name)
      {
        char *face_name_copy = strdup(face_name);  /* Since I want to tokenize without modifying the original string */
        snprintf(current_roll_string, MAX_STRING_LENGTH, "%s", strtok(face_name_copy, "\n"));
        free(face_name_copy);
      }
      else
        snprintf(current_roll_string, MAX_STRING_LENGTH, "%d", current_roll);
    }
    else
      snprintf(current_roll_string, MAX_STRING_LENGTH, "%d", current_roll);
    mudstrlcat(roll_string, current_roll_string, MAX_STRING_LENGTH);
  }

  if (numrolls > 1 && die->value[2] == 1)
  {
    snprintf(total_string, MAX_STRING_LENGTH, ", for a total of %d", rollsum);
    mudstrlcat(roll_string, total_string, MAX_STRING_LENGTH);
  }

  mudstrlcat(roll_string, ".\r\n", MAX_STRING_LENGTH);

  snprintf(output_string, MAX_STRING_LENGTH, "You %s%s", verb, roll_string);
  act(AT_GREEN, output_string, ch, NULL, NULL, TO_CHAR);

  snprintf(output_string, MAX_STRING_LENGTH, "$n %s%s", verb, roll_string);
  act(AT_GREEN, output_string, ch, NULL, NULL, TO_ROOM);

  if (face_seen_table)
    free(face_seen_table);
  return;
}

/*dice chance deal throw*/

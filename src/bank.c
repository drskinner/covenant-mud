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
 ****************************************************************************  
 *                           SMAUG Banking Support Code                     *
 ***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "mud.h"
#include "bank.h"

CHAR_DATA *find_banker(CHAR_DATA *ch)
{
  CHAR_DATA *bank;
  
  for (bank = ch->in_room->first_person; bank; bank = bank->next_in_room)
    if (IS_NPC(bank) && xIS_SET(bank->act, ACT_BANKER))
      break;

  return bank;
}

void do_deposit(CHAR_DATA *ch, const char *argument)
{
  CHAR_DATA *banker;
  char arg1[MAX_INPUT_LENGTH];
  char buf [MAX_STRING_LENGTH];
  int amount;
  
  if (!(banker = find_banker(ch))) {
    send_to_char("You have to go to a bank if you want to make a", ch);
    send_to_char(" deposit.\r\n", ch);
    return;
  }

  if (!ch->furniture) {
    send_to_char("You'll need to stand at the teller's window to make", ch);
    send_to_char(" a transaction.\n\r", ch);
    return;
  }

  if (!nifty_is_name("window", ch->furniture->name)) {
    send_to_char("No, no, no. Stand at the *window* for service.\n\r", ch);
    return;
  }

  if (argument[0] == '\0') {
    do_say(banker, "If you need help, see HELP BANK.");
    return;
  }
  
  argument = one_argument(argument, arg1);
          
  if (arg1 == '\0') {
    sprintf(buf, "%s How many cerons do you wish to deposit?", ch->name);
    do_tell(banker, buf);
    return;
  }
    
  if (str_cmp(arg1, "all") && !is_number(arg1)) {
    sprintf(buf, "%s How many cerons do you wish to deposit?", ch->name);
    do_tell(banker, buf);
    return;
  }
    
  if (!str_cmp(arg1, "all"))
    amount = ch->gold;
  else
    amount = atoi(arg1);
  
  if (amount > ch->gold) {
    sprintf(buf, "%s Sorry, but you don't have that much money to deposit.",
	    ch->name);
    do_tell(banker, buf);
    return;
  }
    
  if (amount <= 0) {
    sprintf(buf, "%s Don't try to be cute...", ch->name);
    do_tell(banker, buf);
    return;
  }
    
  ch->gold -= amount;
  ch->pcdata->balance += amount;
  
  sprintf(buf, "You deposit %d ceron%s.\n\r", amount,
	  (amount != 1) ? "s" : "");
  set_char_color(AT_PLAIN, ch);
  send_to_char(buf, ch);
  sprintf(buf, "$n deposits some money.\n\r");
  act(AT_PLAIN, buf, ch, NULL, NULL, TO_ROOM);
  return;
}

void do_withdraw(CHAR_DATA *ch, const char *argument)
{
  CHAR_DATA *banker;
  char arg1[MAX_INPUT_LENGTH];
  char buf [MAX_STRING_LENGTH];
  int amount;
  
  if (!(banker = find_banker(ch))) {
    send_to_char("There's no bank to withdraw from here.\n\r", ch);
    return;
  }

  if (!ch->furniture) {
    send_to_char("You'll need to stand at the teller's window to make", ch);
    send_to_char(" a transaction.\n\r", ch);
    return;
  }

  if (!nifty_is_name("window", ch->furniture->name)) {
    send_to_char("No, no, no. Stand at the *window* for service.\n\r", ch);
    return;
  }

  if (argument[0] == '\0') {
    do_say(banker, "If you need help, see HELP BANK.");
    return;
  }
  
  argument = one_argument(argument, arg1);
    
  if (arg1 == '\0') {
    sprintf(buf, "%s How much money do you wish to withdraw?", ch->name);
    do_tell(banker, buf);
    return;
  }

  if (str_cmp(arg1, "all") && !is_number(arg1)) { 
    sprintf(buf, "%s How much money do you wish to withdraw?", ch->name);
    do_tell(banker, buf);
    return;
  }
    
  if (!str_cmp(arg1, "all"))
    amount = ch->pcdata->balance;    
  else
    amount = atoi(arg1);
  
  if (amount > ch->pcdata->balance) {
    sprintf(buf, "%s Your balance is insufficient to cover that amount.",
	    ch->name);
    do_tell(banker, buf);
    return;
  }
    
  if (amount <= 0) {
    sprintf(buf, "%s How's that again?", ch->name);
    do_tell(banker, buf);
    return;
  }
    
  ch->pcdata->balance -= amount;
  ch->gold += amount;
  sprintf(buf, "You withdraw %d ceron%s.\n\r", amount,
	  (amount != 1) ? "s" : "");
  set_char_color(AT_PLAIN, ch);
  send_to_char(buf, ch);

  sprintf(buf, "$n makes a withdrawl.\n\r");
  act(AT_PLAIN, buf, ch, NULL, NULL, TO_ROOM);
  return;
}

void do_balance(CHAR_DATA *ch, const char *argument)
{
  CHAR_DATA *banker;
  char buf [MAX_STRING_LENGTH];
    
  if (!(banker = find_banker(ch))) {
    send_to_char("There's no bank here!\n\r", ch);
    return;
  }

  if (!ch->furniture) {
    send_to_char("You'll need to stand at the teller's window to inquire", ch);
    send_to_char(" about your balance.\n\r", ch);
    return;
  }

  if (!nifty_is_name("window", ch->furniture->name)) {
    send_to_char("No, no, no. Stand at the *window* for service.\n\r", ch);
    return;
  }

  set_char_color(AT_PLAIN, ch);
  sprintf(buf, "You have %d ceron%s in the bank.\n\r",
	  ch->pcdata->balance, (ch->pcdata->balance == 1) ? "" : "s");
  send_to_char(buf, ch);
  return;
}

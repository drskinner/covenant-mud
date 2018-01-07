#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include <string.h>
#include <time.h>
#include "mud.h"

#define TWOPIOVER360 0.0174533
#define XSCALE       0.1547

struct hex_data * map_data[MAP_WIDTH][MAP_HEIGHT];

char *maphex_bottom(int xhex, int yhex);
char *maphex_top(int xhex, int yhex);

const char *const top_of_hex[SECT_MAX + 1] =
{
  "&W@@", "&w##", "&W==", "&W##", "  ", "&G``", "&g\"\"", "&Y..", "  ",
  "&Y%%", "&y^^^^", "&W++", "&W--", "&c~&g'", "&B~~", "&B~~", "&b~~", "&RXX",
  "&RXX", "&RXX", "&RXX", "&R&&&&", "&X::", "&w??", "&RXX", "]["
};

const char *const bottom_of_hex[SECT_MAX + 1] =
{
  "&W@@", "&w##", "&W==", "&W++", "__", "&G``", "&g\"\"", "&Y..", "__",
  "&Y%%", "&y^^^^", "&W++", "&W--", "&g'&c~", "&B~~", "&B~~", "&b~~", "&RXX",
  "&RXX", "&RXX", "&RXX", "&R&&&&", "&X::", "&w??", "&RXX", "]["
};

const char *const bottom_with_elevation[SECT_MAX + 1] =
{
  "&W@", "&w#", "&W=", "&W+", "_", "&G`", "&g\"", "&Y.", "_",
  "&Y%", "&y^^", "&W-", "&B~", "&B~", "&B~", "&b~", "&b~", "&RX",
  "&RX", "&RX", "&R&&", "&X:", "&w?", "&RX", "]"
};

void load_map()
{
  FILE *fpMap;
  char filename[256];
  char tchar;
  short echar;
  int x, y;
  struct hex_data *hex;

  snprintf(filename, 256, "%s%s", MAP_DIR, MAP_LIST);
  if ((fpMap = fopen(filename, "r")) == NULL) {
    perror(filename);
    exit(1);
  }

  /* Throw away the first line...btech needs it, we do not */

  fread_to_eol(fpMap);

  /* x is the inside loop; the map is read left-to-right first */

  for (y = 0; y < MAP_HEIGHT; y++) {
    for (x = 0; x < MAP_WIDTH; x++) {

      tchar = fread_letter(fpMap);
      echar = fread_number(fpMap);

      CREATE(hex, struct hex_data, 1);
      hex->terrain = 0;
      hex->elevation = echar;
      hex->first_content = NULL;
      hex->last_content = NULL;

      switch (tchar) {
      default:  hex->terrain = SECT_UNKNOWN;  break;
      case '@': hex->terrain = SECT_INSIDE;   break;
      case '#': hex->terrain = SECT_CITY;     break;
      case '=': hex->terrain = SECT_WALL;     break;
      case '/': hex->terrain = SECT_BRIDGE;   break;
      case '.': hex->terrain = SECT_FIELD;    break;
      case '`': hex->terrain = SECT_LIGHT_WOODS; break;
      case '"': hex->terrain = SECT_HEAVY_WOODS; break;
      case 'd': hex->terrain = SECT_DESERT;   break;
      case '%': hex->terrain = SECT_ROUGH;    break;
      case '^': hex->terrain = SECT_MOUNTAIN; break;
      case '-': hex->terrain = SECT_SNOW;     break;
      case '~':
        if (hex->elevation == 0)
          hex->terrain = SECT_SWAMP;
        else if (hex->elevation > 0 && hex->elevation < 5)
          hex->terrain = SECT_FRESH_WATER;
        else
          hex->terrain = SECT_WATER_NOSWIM;
        break;
      case '&': hex->terrain = SECT_FIRE;    break;
      case ':': hex->terrain = SECT_SMOKE;   break;
      case '?': hex->terrain = SECT_UNKNOWN; break;
      }

      map_data[x][y] = hex;
    }
  }
}

char *maphex_bottom(int xhex, int yhex)
{
  static char bottom[12];

  if (get_elevation(xhex, yhex) == 0)
    snprintf(bottom, 12, "%s&g/", bottom_of_hex[get_terrain(xhex, yhex)]);
  else
    snprintf(bottom, 12, "%s%d&g/", bottom_with_elevation[get_terrain(xhex, yhex)],
	     get_elevation(xhex, yhex));

  return bottom;
}

char *maphex_top(int xhex, int yhex)
{
  static char top[12];

  snprintf(top, 12, "%s&g\\", top_of_hex[get_terrain(xhex, yhex)]);

  if ((xhex >= 0) && (yhex >= 0) && (xhex < MAP_WIDTH) && (yhex < MAP_HEIGHT))
    if (map_data[xhex][yhex]->first_person)
      snprintf(top, 12, "&Rxx&g\\");

  return top;
}

void display_map(CHAR_DATA* ch, int xhex, int yhex, int width, int height)
{
  int xorg, yorg;
  int xmax, ymax;
  int dx, dy;
  int digit;
  char buf[MAX_STRING_LENGTH];
  char line1[MAX_STRING_LENGTH];
  char line2[MAX_STRING_LENGTH];

  xorg = xhex - (width / 2);
  yorg = yhex - (height / 2);

  xmax = xorg + width;
  ymax = yorg + height;

  if (IS_IMMORTAL(ch)) {
    for (dy = 100; dy >= 1; dy /= 10) {
      snprintf(line1, MAX_STRING_LENGTH, "&g      ");
      for (dx = xorg; dx <= xmax; dx++) {
        digit = (dx / dy) % 10;
        if (digit < 0)
          digit *= -1;
        if (((dx < dy) && (digit == 0)) && !((dx == 0) && (dy == 1)))
          snprintf(buf, MAX_STRING_LENGTH, "   ");
        else
          snprintf(buf, MAX_STRING_LENGTH, "%1d  ", digit);
        strcat(line1, buf);
      }
      ch_printf(ch, "%s\r\n", line1);
    }
  }


  for (dy = yorg; dy <= ymax; dy++) {

    if (xorg % 2 == 0) {          /* even */
      snprintf(line1, MAX_STRING_LENGTH, "     &g\\");
      if (IS_IMMORTAL(ch))
        snprintf(line2, MAX_STRING_LENGTH, " %3d &g/", dy);
      else
        snprintf(line2, MAX_STRING_LENGTH, "     &g/");
    }
    else {                        /* odd */
      snprintf(line1, MAX_STRING_LENGTH, "     &g/");
      if (IS_IMMORTAL(ch))
        snprintf(line2, MAX_STRING_LENGTH, " %3d &g\\", dy);
      else
        snprintf(line2, MAX_STRING_LENGTH, "     &g\\");
    }
    for (dx = xorg; dx <= xmax; dx++) {
      if (dx % 2 == 0)
        snprintf(buf, MAX_STRING_LENGTH, "%s", (dy - 1) < yorg ? "][/" : maphex_bottom(dx, dy-1));
      else {
        if ((dx == xhex) && (dy == yhex))
          snprintf(buf, MAX_STRING_LENGTH, "&G**&g\\");
        else
          snprintf(buf, MAX_STRING_LENGTH, "%s", dy >= ymax ? "][\\" : maphex_top(dx, dy));
      }
      strcat(line1, buf);

      if (dx % 2 == 0) {
        if ((dx == xhex) && (dy == yhex))
          snprintf(buf, MAX_STRING_LENGTH, "&G**&g\\");
        else
          snprintf(buf, MAX_STRING_LENGTH, "%s", maphex_top(dx, dy));
      }
      else
        snprintf(buf, MAX_STRING_LENGTH, "%s", maphex_bottom(dx, dy));
      strcat(line2, buf);
    }

    if (dy < ymax)
      ch_printf(ch, "%s\r\n%s&w\r\n", line1, line2);
    else
      ch_printf(ch, "%s&w\r\n", line1);
  }

  return;
}

void do_map(CHAR_DATA* ch, const char* argument)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  int xhex, yhex;

  argument = one_argument(argument, arg1);
  argument = one_argument(argument, arg2);

  if (!IS_IMMORTAL(ch)) {
    if (ch->in_hex)
      display_map(ch, ch->xhex, ch->yhex, 20, 9);
    else
      send_to_char("You need to be on the map to do that.\r\n", ch);
    return;
  }

  if ((arg1[0] == '\0') || (arg2[0] == '\0')) {
    if (ch->in_hex)
      display_map(ch, ch->xhex, ch->yhex, 20, 9);
    else
      send_to_char("Syntax: map <xhex> <yhex>\r\n", ch);
    return;
  }

  xhex = atoi(arg1);
  if ((xhex < 0) || (xhex >= MAP_WIDTH)) {
    ch_printf(ch, "xhex out of range. Valid values are 0 to %d\r\n",
              MAP_WIDTH - 1);
    return;
  }

  yhex = atoi(arg2);
  if ((yhex < 0) || (yhex >= MAP_HEIGHT)) {
    ch_printf(ch, "yhex out of range. Valid values are 0 to %d\r\n",
              MAP_HEIGHT - 1);
    return;
  }

  /* 20 is display width, 9 is display height.                                                                                      Values fixed for now; could be made settable -- Shamus */

  display_map(ch, xhex, yhex, 20, 9);
  return;
}

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

const char *const speed_name[SPEED_MAX] =
{
  "Stopped  ", "Walking   ", "Jogging  ", "Running  ", "Sprinting",
  "Walking  ", "Trotting  ", "Cantering", "Galloping"
};

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

char * const inav_display[13] =
{
  "&g              0                ",
  "&g         ___________           ",
  "&g        /           \\          ",
  "&g  300  /             \\  60     ",
  "&g      /               \\        ",
  "&g     /                 \\       ",
  "&g270 (                   )  90  ",
  "&g     \\                 /       ",
  "&g      \\               /        ",
  "&g  240  \\             /  120    ",
  "&g        \\___________/          ",
  "&g                               ",
  "&g              180              "
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

int acceptable_degree(int d)
{
  while (d < 0)
    d += 360;
  while (d >= 360)
    d -= 360;
  return d;
}

int find_bearing(double x0, double y0, double x1, double y1)
{
  double deltax, deltay;
  double temp, rads;
  int degrees;

  deltax = (x0 - x1);
  deltay = (y0 - y1);

  if (deltax == 0.0) {     /* Quick handling inserted here */
    if (deltay > 0.0)
      return 0;
    else
      return 180;
  }
  temp = deltay / deltax;
  if (temp < 0.0)
    temp = -temp;
  rads = atan (temp);
  degrees = (int) (rads * 10.0 / TWOPIOVER360);

  /* Round off degrees */

  degrees = (degrees + 5) / 10;
  if (deltax < 0.0 && deltay < 0.0)
    degrees += 180;
  else if (deltax < 0.0)
    degrees = 180 - degrees;
  else if (deltay < 0.0)
    degrees = 360 - degrees;
  return acceptable_degree(degrees - 90);
}

double find_hex_range(double x0, double y0, double x1, double y1)
{
  double bearing, xscale;

  bearing = (double) (find_bearing (x0, y0, x1, y1) % 60) / 60.0;
  if (bearing > 0.5)
    bearing = 1.0 - bearing;
  bearing = bearing * 2.0;      /* 0 - 1 correction */
  xscale = (1.0 + XSCALE * bearing);
  xscale = xscale * xscale;
  return sqrt (xscale * (x0 - x1) * (x0 - x1) + (y0 - y1) * (y0 - y1));
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

/*
 * Convert a hex number to cartesian cooridinates. "alpha" is one-third
 * the ratio between the height of a hex and its width at the widest point.
 * This function assumes that the center of the hex is what we're interested
 * in converting. -- Shamus
 */

void hex_to_cartesian(int xhex, int yhex, double *xcart, double *ycart)
{
  double alpha = 0.288675134;

  if (xhex % 2) { /* xhex is odd */
    *xcart = (double) (xhex - 1.0) * 3.0 * alpha + 5.0 * alpha;
    *ycart = (double) (yhex);
  }
  else {
    *xcart = (double) (xhex) * 3.0 * alpha + 2.0 * alpha;
    *ycart = (double) yhex  + 0.5;
  }
}

#define NAVIGATE_LINES 13.0
#define NAVIGATE_CENTERX 14.0
#define NAVIGATE_CENTERY 6.0

#define NAVIGATE_MAX_X_DIFF 9.0
#define NAVIGATE_MAX_Y_DIFF 4.0

#define SHOW_CHAR(ch, letter) \
  bearing = find_bearing(fx, fy, ch->xcart, ch->ycart); \
  range = find_hex_range(fx, fy, ch->xcart, ch->ycart); \
  if (range > 0.5) \
    range = 0.5; \
  bearing = acceptable_degree(bearing + 180); \
  bear = ((double)bearing + 90.0) * TWOPIOVER360;                           \
  tx = NAVIGATE_CENTERX + range * NAVIGATE_MAX_X_DIFF * 2 * (f1=cos(bear)); \
  ty = NAVIGATE_CENTERY + range * NAVIGATE_MAX_Y_DIFF * 2 * (f2=sin(bear)); \
  tx = URANGE_DOUBLE(NAVIGATE_CENTERX - NAVIGATE_MAX_X_DIFF, tx,  \
                     NAVIGATE_CENTERX + NAVIGATE_MAX_X_DIFF); \
  ty = URANGE_DOUBLE(NAVIGATE_CENTERY - NAVIGATE_MAX_Y_DIFF, ty,  \
                     NAVIGATE_CENTERY + NAVIGATE_MAX_Y_DIFF); \
  left[(int)ty][(int)(tx + 2)] = letter;

/* The above has to be [tx + 2] to account for the colour code &g */

void navigate(CHAR_DATA *ch)
{
  char left [13][MAX_STRING_LENGTH];
  char mid  [13][MAX_STRING_LENGTH];
  char right[13][MAX_STRING_LENGTH];
  char buf[MAX_STRING_LENGTH];
  CHAR_DATA *vch;

  int xmin, bearing;
  int dy, ymin, ymax;
  int oddmod = 0;
  int i = 0;
  int level = 0;
  double fx, fy, f1, f2, range, bear;
  double tx, ty;

  hex_to_cartesian(ch->xhex, ch->yhex, &fx, &fy);

  xmin = ch->xhex - 2;

  ymin = ch->yhex - 2;
  ymax = ymin + 4;

  if (ch->xhex % 2 == 1)
    oddmod = -1;

  sprintf(right[i], "&g       __");
  i++;

  for (dy = ymin; dy <= ymax; dy++) {

    if (dy == ymin)
      sprintf(right[i], "    &g__/");
    else if (dy == ymax)
      sprintf(right[i], "   &g\\%s", maphex_bottom(xmin + 1, dy + oddmod));
    else
      sprintf(right[i], "&g/%s%s", maphex_top(xmin, dy),
              maphex_bottom(xmin + 1, dy + oddmod));

    sprintf(buf, "%s", dy == ch->yhex ? "&G**&g\\" : maphex_top(xmin + 2, dy));
    strcat(right[i], buf);

    if (dy == ymin)
      sprintf(buf, "__");
    else if (dy == ymax)
      sprintf(buf, "%s", maphex_bottom(xmin + 3, dy + oddmod));
    else
      sprintf(buf, "%s%s", maphex_bottom(xmin + 3, dy + oddmod), maphex_top(xmin + 4, dy));
    strcat(right[i], buf);
    i++;

    if (dy == ymin)
      sprintf(right[i], " &g__/%s", maphex_top(xmin + 1, dy + 1 + oddmod));
    else if (dy == ymax)
      sprintf(right[i], "      &g\\");
    else
      sprintf(right[i], "&g\\%s%s", maphex_bottom(xmin, dy),
              maphex_top(xmin + 1, dy + 1 + oddmod));

    sprintf(buf, "%s", maphex_bottom(xmin + 2, dy));
    strcat(right[i], buf);

    if (dy == ymin)
      sprintf(buf, "%s__", maphex_top(xmin + 3, dy + 1 + oddmod));
    else if (dy == ymax)
      sprintf(buf, "  ");
    else
      sprintf(buf, "%s%s", maphex_top(xmin + 3, dy + 1 + oddmod),
              maphex_bottom(xmin + 4, dy));

    strcat(right[i], buf);
    i++;
  }

  sprintf(right[i], "  ");
  i++;
  sprintf(right[i], "  ");

  for (i = 0; i < 13; i++) {
    sprintf(left[i], "%s", inav_display[i]);
    sprintf(mid[i], "                          ");
  }

  /* mid-section of display */

  level = get_elevation(ch->xhex, ch->yhex);
  if (IS_WATER_SECT(ch->in_hex->terrain))
    level *= -1;

  sprintf(mid[ 2], " Location: %3d, %3d, %2d   ", ch->xhex, ch->yhex, level);
  sprintf(mid[ 3], " Terrain:  %-14.14s ",
          sector_name[(ch->in_hex->terrain)]);
  if (IS_IMMORTAL(ch))
    sprintf(mid[ 6], " Speed:      %1.6f     ", ch->pcdata->realspeed);
  else
    sprintf(mid[ 6], " Speed:      %-9.9s    ",
            speed_name[ch->pcdata->speed]);
  sprintf(mid[ 8], " Heading:    %-3d          ", ch->pcdata->heading);

  for (vch = ch->in_hex->first_person; vch; vch = vch->next_in_room) {
    if ((can_see(ch, vch)) && (ch != vch)) {
      SHOW_CHAR(vch, 'x');
    }
  }

  SHOW_CHAR(ch, '*');

  send_to_char("\r\n", ch);
  for (i = 0; i < 13; i++)
    ch_printf_color(ch, "%s%s%s\r\n", left[i], mid[i], right[i]);

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

  /* 20 is display width, 9 is display height.
   * Values fixed for now; could be made settable -- Shamus */

  display_map(ch, xhex, yhex, 20, 9);
  return;
}

void do_hexgoto(CHAR_DATA* ch, const char* argument)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  int xhex, yhex;

  argument = one_argument(argument, arg1);
  argument = one_argument(argument, arg2);

  if ((arg1[0] == '\0') || (arg2[0] == '\0')) {
    send_to_char("Syntax: hexgoto <xhex> <yhex>\r\n", ch);
    return;
  }

  xhex = atoi(arg1);
  if ((xhex < 0) || (xhex >= MAP_WIDTH)) {
    ch_printf(ch, "Out of range. Valid values are 0 to %d", MAP_WIDTH - 1);
    return;
  }

  yhex = atoi(arg2);
  if ((yhex < 0) || (yhex >= MAP_HEIGHT)) {
    ch_printf(ch, "Out of range. Valid values are 0 to %d", MAP_HEIGHT - 1);
    return;
  }

  if (!xIS_SET(ch->act, PLR_WIZINVIS)) {
    if (ch->pcdata && ch->pcdata->bamfout[0] != '\0') {
      snprintf(buf, MAX_STRING_LENGTH, "%s %s", ch->name, ch->pcdata->bamfout);
      act(AT_IMMORT, "$T", ch, NULL, wordwrap(buf, 78), TO_ROOM);
    }
    else
      act(AT_IMMORT, "$n leaves in a swirling mist.", ch, NULL, NULL, TO_ROOM);
  }

  if (ch->in_room) {
    ch->regoto = ch->in_room->vnum;
    char_from_room(ch);
  }

  if (ch->in_hex)
    char_from_hex(ch);

  if (ch->mount) {
    if (ch->mount->in_room)
      char_from_room(ch->mount);
    else
      char_from_hex(ch->mount);
    char_to_hex(ch->mount, xhex, yhex);
    hex_to_cartesian(xhex, yhex, &ch->mount->xcart, &ch->mount->ycart);
  }


  char_to_hex(ch, xhex, yhex);
  ch->pcdata->speed = 0;
  ch->pcdata->realspeed = 0;
  hex_to_cartesian(xhex, yhex, &ch->xcart, &ch->ycart);
  do_look(ch, "auto");

  if (!xIS_SET(ch->act, PLR_WIZINVIS)) {
    if (ch->pcdata && ch->pcdata->bamfin[0] != '\0') {
      snprintf(buf, MAX_STRING_LENGTH, "%s %s", ch->name, ch->pcdata->bamfin);
      act(AT_IMMORT, "$T", ch, NULL, wordwrap(buf, 78), TO_ROOM);
    }
    else
      act(AT_IMMORT, "$n appears in a swirling mist.", ch, NULL, NULL,
          TO_ROOM);
  }

  return;
}

void do_hexstat(CHAR_DATA* ch, const char* argument)
{
  char buf[MAX_STRING_LENGTH];
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  HEX_DATA *location;
  OBJ_DATA *obj;
  CHAR_DATA *rch;
  int xhex = 0;
  int yhex = 0;

  argument = one_argument(argument, arg1);
  argument = one_argument(argument, arg2);

  if ((arg1[0] == '\0') || (arg2[0] == '\0')) {
    if (ch->in_hex) {
      xhex = ch->xhex;
      yhex = ch->yhex;
    }
    else {
      send_to_char("Syntax: hexstat <xhex> <yhex>\r\n", ch);
      return;
    }
  }

  if ((xhex == 0) && (arg1[0] != '\0')) {
    xhex = atoi(arg1);
    if ((xhex < 0) || (xhex >= MAP_WIDTH)) {
      ch_printf(ch, "Out of range. Valid values are 0 to %d.\r\n", MAP_WIDTH - 1);
      return;
    }
  }

  if ((yhex == 0) && (arg2[0] != '\0')) {
    yhex = atoi(arg2);
    if ((yhex < 0) || (yhex >= MAP_HEIGHT)) {
      ch_printf(ch, "Out of range. Valid values are 0 to %d.\r\n", MAP_HEIGHT - 1);
      return;
    }
  }

  location = map_data[xhex][yhex];

  ch_printf_color(ch, "&cHex: &w%d, %d   ", xhex, yhex);

  if ((location->terrain >= 0) && (location->terrain < SECT_MAX))
    snprintf(buf, MAX_STRING_LENGTH, "%s", sector_name[location->terrain]);
  else
    snprintf(buf, MAX_STRING_LENGTH, "&RError!&w");

  ch_printf_color(ch, "&cTerrain: &w(%d) %s  &c%s: &w%d\r\n",
                  location->terrain, buf,
                  IS_WATER_SECT(location->terrain) ?
                  "Depth" : "Elevation",
                  location->elevation);

  send_to_char_color("&cCharacters: &w", ch);
  for (rch = location->first_person; rch; rch = rch->next_in_room) {
    if (can_see(ch, rch)) {
      send_to_char(" ", ch);
      one_argument(rch->name, buf);
      send_to_char(buf, ch);
    }
  }

  send_to_char_color("\r\n&cObjects:    &w", ch);
  for (obj = location->first_content; obj; obj = obj->next_content) {
    send_to_char(" ", ch);
    one_argument(obj->name, buf);
    send_to_char(buf, ch);
  }
  send_to_char("\r\n", ch);

  return;
}

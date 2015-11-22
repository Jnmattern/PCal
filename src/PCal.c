#include <pebble.h>

// Languages
#define LANG_DUTCH 0
#define LANG_ENGLISH 1
#define LANG_FRENCH 2
#define LANG_GERMAN 3
#define LANG_SPANISH 4
#define LANG_ITALIAN 5
#define LANG_SWEDISH 6
#define LANG_MAX 7

// Non Working Days Country
#define NWD_NONE 0
#define NWD_USA 1
#define NWD_FRANCE 2
#define NWD_SWEDEN 3
#define NWD_MAX 4

#define SUN 0
#define MON 1
#define TUE 2
#define WED 3
#define THU 4
#define FRI 5
#define SAT 6

#define JAN 0
#define FEB 1
#define MAR 2
#define APR 3
#define MAY 4
#define JUN 5
#define JUL 6
#define AUG 7
#define SEP 8
#define OCT 9
#define NOV 10
#define DEC 11

enum {
	CONFIG_KEY_LANG			= 30,
	CONFIG_KEY_WEEKSTART		= 31,
	CONFIG_KEY_NWDCOUNTRY 	= 32,
	CONFIG_KEY_SHOWWEEKNUM	= 33,
};

char buffer[256] = "";

int curLang = LANG_ENGLISH;
int weekStartsOnMonday = 0;
int nwdCountry = 0;
int showWeekNum = 1;

static const char *monthNames[LANG_MAX][12] = {
	{ "Januari", "Februari", "Maart", "April", "Mei", "Juni", "Juli", "Augustus", "September", "Oktober", "November", "December" },			// Dutch
	{ "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" },			// English
	{ "Janvier", "Février", "Mars", "Avril", "Mai", "Juin", "Juillet", "Août", "Septembre", "Octobre", "Novembre", "Décembre" },			// French
	{ "Januar", "Februar", "März", "April", "Mai", "Juni", "Juli", "August", "September", "Oktober", "November", "Dezember" },				// German
	{ "Enero", "Febrero", "Marzo", "Abril", "Mayo", "Junio", "Julio", "Augusto", "Septiembre", "Octubre", "Noviembre", "Diciembre" },		// Spanish
	{ "Gennaio", "Febbraio", "Marzo", "Aprile", "Maggio", "Giugno", "Luglio", "Agosto", "Settembre", "Ottobre", "Novembre", "Dicembre" },	// Italian
	{ "Januari", "Februari", "Mars", "April", "Maj", "Juni", "Juli", "Augusti", "September", "Oktober", "November", "December" }			// Swedish
};

static const char *weekDays[LANG_MAX][7] = {
	{ "Zo", "Ma", "Di", "Wo", "Do", "Vr", "Za" },	// Dutch
	{ "Su", "Mo", "Tu", "We", "Th", "Fr", "Sa" },	// English
	{ "Di", "Lu", "Ma", "Me", "Je", "Ve", "Sa" },	// French
	{ "So", "Mo", "Di", "Mi", "Do", "Fr", "Sa" },	// German
	{ "Do", "Lu", "Ma", "Mi", "Ju", "Vi", "Sá" },	// Spanish
	{ "Do", "Lu", "Ma", "Me", "Gi", "Ve", "Sa" },	// Italian
	{ "Sö", "Må", "Ti", "On", "To", "Fr", "Lö" }		// Swedish
};

#define REPEATING_CLICK_INTERVAL 200
#define SCREENW 144
#define SCREENH 168
#define MONTHNAME_LAYER_HEIGHT 20
#define MONTH_LAYER_HEIGHT (SCREENH-20-MONTHNAME_LAYER_HEIGHT)

#if defined(PBL_RECT)
#define XOFFSET 0
#define YOFFSET 0
#elif defined(PBL_ROUND)
#define XOFFSET 18
#define YOFFSET 10
#endif

typedef struct {
	int day;
	int month;
	int year;
} Date;

#define Date(d, m, y) ((Date){ (d), (m), (y) })

Window *window;
Layer *monthLayer;
TextLayer *monthNameLayer;
Date today;
int displayedMonth, displayedYear;
//GFont myFont;

static int DX, DY, DW, DH;

static char monthName[40] = "";

static int julianDay(const Date *theDate) {
	int a = (int)((13-theDate->month)/12);
	int y = theDate->year+4800-a;
	int m = theDate->month + 12*a - 2;
	
	int day = theDate->day + (int)((153*m+2)/5) + y*365 + (int)(y/4) - (int)(y/100) + (int)(y/400) - 32045;
	return day;
}

static int dayOfWeek(const Date *theDate) {
	int J = julianDay(theDate);
	return (J+1)%7;
}


static bool isLeapYear(const int Y) {
  return (((Y%4 == 0) && (Y%100 != 0)) || (Y%400 == 0));
}

static int numDaysInMonth(const int M, const int Y) {
  static const int nDays[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };

  return nDays[M] + (M == FEB)*isLeapYear(Y);
}

static int ordinalDay(const Date *theDate) {
  // Returns the day number in the year
  if (theDate->month == JAN) {
    return theDate->day;
  } else {
    int daynum = theDate->day;
    for (int m=theDate->month-1; m >= JAN; m--) {
      daynum += numDaysInMonth(m, theDate->year);
    }
    return daynum;
  }
}

static void nthWeekdayOfMonth(const int Y, const int M, const int weekday, const int n, Date *theDate) {
  int firstDayOfMonth, curWeekday, count = 0;

  theDate->day = 1;
  theDate->month = M;
  theDate->year = Y;

  firstDayOfMonth = curWeekday = dayOfWeek(theDate);

  if (firstDayOfMonth == weekday) {
    count = 1;
  }
  while (count < n) {
    theDate->day++;
    curWeekday = (curWeekday+1)%7;
    if (curWeekday == weekday) {
      count++;
    }
  }
}

static void lastWeekdayOfMonth(const int Y, const int M, const int weekday, Date *theDate) {
  int curWeekday;

  theDate->day = numDaysInMonth(M, Y);
  theDate->month = M;
  theDate->year = Y;

  curWeekday = dayOfWeek(theDate);
  while (curWeekday != weekday) {
    theDate->day--;
    curWeekday = (curWeekday+6)%7;
  }
}

static int weekNumber(const Date *theDate) {
  if (weekStartsOnMonday) {
    // ISO Week numbers
    int J = julianDay(theDate);
    
    int d4 = (J+31741-(J%7))%146097%36524%1461;
    int L = (int)(d4/1460);
    int d1 = ((d4-L)%365)+L;
    
    return (int)(d1/7)+1;
  } else {
    // US Week numbers (First week begins on 1st of January, next week begins on next Sunday. If 1st of January is a sunday, week #2 begins on 8th)
    Date januaryFirst = Date(1, JAN, theDate->year);
    int weekdayJanFirst = dayOfWeek(&januaryFirst);
    int daynum = ordinalDay(theDate);

    return 1 + ((daynum+weekdayJanFirst-1)/7);
  }
}


static void dateAddDays(Date *date, int numDays);

static void dateSubDays(Date *date, int numDays) {
	int i;

	if (numDays > 0) {
		for (i=0; i<numDays; i++) {
			if (date->day == 1) {
				if (date->month == 0) {
					date->year--;
					date->month = 11;
					date->day = 31;
				} else {
					date->month--;
					date->day = numDaysInMonth(date->month, date->year);
				}
			} else {
				date->day--;
			}
		}
	} else if (numDays < 0) {
		dateAddDays(date, -numDays);
	}
}

static void dateAddDays(Date *date, int numDays) {
	int i;
	
	if (numDays > 0) {
		for (i=0; i<numDays; i++) {
			if (date->day == numDaysInMonth(date->month, date->year)) {
				if (date->month == 11) {
					date->year++;
					date->month = 0;
					date->day = 1;
				} else {
					date->month++;
					date->day = 1;
				}
			} else {
				date->day++;
			}
		}
	} else if (numDays < 0) {
		dateSubDays(date, -numDays);
	}
}

/*
static int compareDates(Date *d1, Date *d2) {
	if (d1->year < d2->year) {
		return -1;
	} else if (d1->year > d2->year) {
		return 1;
	} else {
		if (d1->month < d2->month) {
			return -1;
		} else if (d1->month > d2->month) {
			return 1;
		} else {
			if (d1->day < d2->day) {
				return -1;
			} else if (d1->day > d2->day) {
				return 1;
			} else {
				return 0;
			}
		}
	}
}
*/

static void easterMonday(const int Y, Date *theDate) {
	int a = Y-(int)(Y/19)*19;
	int b = (int)(Y/100);
	int C = Y-(int)(Y/100)*100;
	int P = (int)(b/4);
	int E = b-(int)(b/4)*4;
	int F = (int)((b + 8) / 25);
	int g = (int)((b - F + 1) / 3);
	int h = (19 * a + b - P - g + 15)-(int)((19 * a + b - P - g + 15)/30)*30;
	int i = (int)(C / 4);
	int K = C-(int)(C/4)*4;
	int r = (32 + 2 * E + 2 * i - h - K) - (int)((32 + 2 * E + 2 * i - h - K)/7)*7;
	int N = (int)((a + 11 * h + 22 * r) / 451);
	int M = (int)((h + r - 7 * N + 114) / 31);
	int D = ((h + r - 7 * N + 114)-(int)((h + r - 7 * N + 114)/31)*31) + 1;
	
	if (D == numDaysInMonth(M-1, Y)) {
		theDate->day = 1;
		theDate->month = M;
	} else {
		theDate->day = D+1;
		theDate->month = M-1;
	}
	theDate->year = Y;
}

static void ascensionDay(const int Y, Date *theDate) {
	easterMonday(Y, theDate);
	dateAddDays(theDate, 38);
}

static void whitSunday(const int Y, Date *theDate) {
	easterMonday(Y, theDate);
	dateAddDays(theDate, 48);
}

static void whitMonday(const int Y, Date *theDate) {
	easterMonday(Y, theDate);
	dateAddDays(theDate, 49);
}

static void MLKBirthday(const int Y, Date *theDate) {
	// Third monday in January
	nthWeekdayOfMonth(Y, JAN, MON, 3, theDate);
}

static void presidentDay(const int Y, Date *theDate) {
	// Third monday in February
	nthWeekdayOfMonth(Y, FEB, MON, 3, theDate);
}

static void memorialDay(const int Y, Date *theDate) {
	// Last monday in May
	lastWeekdayOfMonth(Y, MAY, MON, theDate);
}

static void laborDay(const int Y, Date *theDate) {
	// First monday in September
	nthWeekdayOfMonth(Y, SEP, MON, 1, theDate);
}

static void columbusDay(const int Y, Date *theDate) {
	// Second monday in October
	nthWeekdayOfMonth(Y, OCT, MON, 2, theDate);
}

static void thanksgivingThursday(const int Y, Date *theDate) {
	// Fourth thursday in november
	nthWeekdayOfMonth(Y, NOV, THU, 4, theDate);
}

static void thanksgivingFriday(const int Y, Date *theDate) {
	// Friday next to the fourth thursday in november
	thanksgivingThursday(Y, theDate);
	dateAddDays(theDate, 1);
}

static void midsommardagen(const int Y, Date *theDate) {
	// Saturday between June 20th and 26th
	theDate->day = 20;
	theDate->month = JUN;
	theDate->year = Y;
	
	while (dayOfWeek(theDate) != SAT) {
		dateAddDays(theDate, 1);
	}
}

static void allaHelgonsDag(const int Y, Date *theDate) {
	// Saturday between October 31st and November 6th
	theDate->day = 31;
	theDate->month = OCT;
	theDate->year = Y;
	
	while (dayOfWeek(theDate) != SAT) {
		dateAddDays(theDate, 1);
	}
}

static bool isNonWorkingDay(const Date *theDate) {
	Date d, d1, d2;
	
	switch (nwdCountry) {
		case NWD_NONE:
			return false;
			break;

		case NWD_FRANCE:
			if (theDate->day == 1  && theDate->month == JAN) return true; // New year's day
			if (theDate->day == 11 && theDate->month == NOV) return true; // Armistice 1918 // Veteran's day
			if (theDate->day == 25 && theDate->month == DEC) return true; // Noël // Christmas
			if (theDate->day == 1  && theDate->month == MAY) return true; // Fête du travail
			if (theDate->day == 8  && theDate->month == MAY) return true; // Armistice 1945
			if (theDate->day == 14 && theDate->month == JUL) return true; // Fête nationale
			if (theDate->day == 15 && theDate->month == AUG) return true; // Assomption
			if (theDate->day == 1  && theDate->month == NOV) return true; // Toussaint
			
			easterMonday(theDate->year, &d);
			if (theDate->day == d.day && theDate->month == d.month) return true; // Lundi de Pâques
			ascensionDay(theDate->year, &d);
			if (theDate->day == d.day && theDate->month == d.month) return true; // Jeudi de l'ascension
			whitMonday(theDate->year, &d);
			if (theDate->day == d.day && theDate->month == d.month) return true; // Lundi de Pentecôte
			
			break;
			
		case NWD_USA:
			if (theDate->day == 1  && theDate->month == JAN) return true; // New year's day
			if (theDate->day == 11 && theDate->month == NOV) return true; // Armistice 1918 // Veteran's day
			if (theDate->day == 25 && theDate->month == DEC) return true; // Noël // Christmas
			if (theDate->day == 4 && theDate->month == JUL) return true; // Independence day

			switch (theDate->month) {
				case JAN:
					MLKBirthday(theDate->year, &d); 
					if (theDate->day == d.day && theDate->month == d.month) return true; // Martin Luther King Jr.'s Birthday
					break;
				case FEB:
					presidentDay(theDate->year, &d);
						if (theDate->day == d.day && theDate->month == d.month) return true; // President's day
					break;
				case MAY:
					memorialDay(theDate->year, &d);
						if (theDate->day == d.day && theDate->month == d.month) return true; // Memorial Day
					break;
				case SEP:
					laborDay(theDate->year, &d);
						if (theDate->day == d.day && theDate->month == d.month) return true; // Labor Day
					break;
				case OCT:
					columbusDay(theDate->year, &d);
						if (theDate->day == d.day && theDate->month == d.month) return true; // Columbus Day
					break;
				case NOV:
					thanksgivingThursday(theDate->year, &d);
						if (theDate->day == d.day && theDate->month == d.month) return true; // Thanksgiving thursday
					thanksgivingFriday(theDate->year, &d);
						if (theDate->day == d.day && theDate->month == d.month) return true; // Thanksgiving friday
					break;
			}
			break;
			
		case NWD_SWEDEN:
			if (theDate->day == 1  && theDate->month == JAN) return true; // Nyårsdagen / New year's day
			if (theDate->day == 6  && theDate->month == JAN) return true; // Trettondedag / Epiphany
			if (theDate->day == 1  && theDate->month == MAY) return true; // Första maj / Labor Day
			if (theDate->day == 6  && theDate->month == JUN) return true; // Nationaldagen / National Day
			if (theDate->day == 25 && theDate->month == DEC) return true; // Juldagen // Christmas
			if (theDate->day == 26 && theDate->month == DEC) return true; // Annandag jul // Boxing Day

			easterMonday(theDate->year, &d);
			d1 = d2 = d;
			dateSubDays(&d1, 1); // easter Sunday
			dateSubDays(&d2, 3); // Good Friday
			if (theDate->day == d2.day && theDate->month == d2.month) return true; // Långfredagen / Good Friday
			if (theDate->day == d1.day && theDate->month == d1.month) return true; // Påskdagen / Easter Sunday
			if (theDate->day == d.day && theDate->month == d.month) return true; // Annandag påsk / Easter Monday

			ascensionDay(theDate->year, &d);
			if (theDate->day == d.day && theDate->month == d.month) return true; // Kristi Himmelsfärdsdag / Ascension Day
			
			whitSunday(theDate->year, &d);
			if (theDate->day == d.day && theDate->month == d.month) return true; // Pingsdagen / Whit Sunday

			midsommardagen(theDate->year, &d);
			if (theDate->day == d.day && theDate->month == d.month) return true; // Midsommardagen / Midsummer Day
			
			allaHelgonsDag(theDate->year, &d);
			if (theDate->day == d.day && theDate->month == d.month) return true; // Alla helgons dag / All Saints' Day

			break;
	}

	return false;
}

void updateMonthText() {
	snprintf(monthName, sizeof(monthName), "%s %d", monthNames[curLang][displayedMonth], displayedYear);
	text_layer_set_text(monthNameLayer, monthName);
}

void updateMonth(Layer *layer, GContext *ctx) {
	static char numStr[3] = "";
	int i, x, s, numWeeks, dy, firstday, numDays, l=0, c=0, w;
	Date first, d;
	GFont f, fontNorm, fontBold;
	GRect rect, fillRect;
	
	first = Date(1, displayedMonth, displayedYear);
	
	if (weekStartsOnMonday) {
		firstday = (dayOfWeek(&first)+6)%7;
	} else {
		firstday = dayOfWeek(&first);
	}

	numDays = numDaysInMonth(displayedMonth, displayedYear);
	
	numWeeks = (firstday+6+numDays)/7;
	
	dy = DY + DH*(6-numWeeks)/2;
	
	graphics_context_set_stroke_color(ctx, GColorBlack);
	graphics_context_set_fill_color(ctx, GColorBlack);
	
	// Calendar Grid
	if (showWeekNum) {
		x = DX+DW;
	} else {
		x = DX;
	}

	// Black Top Line with days of week
	graphics_fill_rect(ctx, GRect(x, dy, DW*7+1, DH), 4, GCornersTop);

	if (showWeekNum) {
		// Black left column for week numbers
		graphics_fill_rect(ctx, GRect(DX, dy+DH, DW, numWeeks*DH+1), 4, GCornersLeft);
		x = DX+DW;
		w = DW*7;
	} else {
		x = DX+1;
		w = DW*7-1;
	}

	// Double line on the outside
	graphics_draw_round_rect(ctx, GRect(x, dy+DH, w, numWeeks*DH), 0);
	
	// Column(s) for the week-end or sunday
	if (weekStartsOnMonday) {
		x = DX+5*DW+1;
	} else {
		x = DX+DW+1;
	}

	if (showWeekNum) {
		x += DW;
	}

	graphics_draw_line(ctx, GPoint(x, dy+DH), GPoint(x, dy+DH+numWeeks*DH-1));
	
	x = showWeekNum;

	// Vertical lines
	for (i=x; i<=x+7; i++) {
		graphics_draw_line(ctx, GPoint(DX+DW*i,dy+DH), GPoint(DX+DW*i,dy+(numWeeks+1)*DH));
	}
	// Horizontal lines
	for (i=1; i<=(numWeeks+1); i++) {
		graphics_draw_line(ctx, GPoint(DX+x*DW,dy+DH*i), GPoint(DX+DW*(7+x),dy+DH*i));
	}
	
	fontNorm = fonts_get_system_font(FONT_KEY_GOTHIC_18);
	fontBold = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
	f = fontNorm;
	
	s = weekStartsOnMonday;
	x = showWeekNum;

	// Days of week
	graphics_context_set_text_color(ctx, GColorWhite);
	
	for (i=s; i<s+7; i++) {
		graphics_draw_text(ctx, weekDays[curLang][i%7], fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(DX+DW*(i+x-s), dy, DW+2, DH+1), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
	}
	
	if (showWeekNum) {
		// Week numbers
		for (i=0, d=first; i<=numWeeks; i++, d.day+=7) {
			snprintf(numStr, sizeof(numStr), "%d", weekNumber(&d));
			graphics_draw_text(ctx, numStr, fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(DX, dy+DH*(i+1), DW, DH+1), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
		}
	}
	
	// Day numbers
	graphics_context_set_text_color(ctx, GColorBlack);
	
	for (i=1; i<=numDays; i++) {
		c = (firstday - 1 + i)%7;
		if (c == 0 && i != 1) {
			l++;
		}
		
		snprintf(numStr, sizeof(numStr), "%d", i);

		if (isNonWorkingDay(&Date(i, displayedMonth, displayedYear))) {
			f = fontBold;
		} else {
			f = fontNorm;
		}
		
		fillRect = GRect(DX+DW*(c+x), dy+DH*(l+1), DW, DH);
		rect = GRect(DX+DW*(c+x), dy+DH*(l+1)-3, DW+1, DH+1);
		
		if (today.day == i && today.month == displayedMonth && today.year == displayedYear) {
			graphics_fill_rect(ctx, fillRect, 0, GCornerNone);
			graphics_context_set_text_color(ctx, GColorWhite);
			graphics_draw_text(ctx, numStr, f, rect, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
		} else {
			graphics_context_set_text_color(ctx, GColorBlack);
			graphics_draw_text(ctx, numStr, f, rect, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
		}
	}
}

void logVariables(const char *msg) {
	snprintf(buffer, 256, "MSG: %s\n\tcurLang=%d\n\tshowWeekNum=%d\n\tweekStartsOnMonday=%d\n\tnwdCountry=%d\n", msg, curLang, showWeekNum, weekStartsOnMonday, nwdCountry);
	
	APP_LOG(APP_LOG_LEVEL_DEBUG, buffer);
}

static int numClicks = 0;
void btn_up_handler(ClickRecognizerRef recognizer, void *context) {
	numClicks = 0;
}

void up_single_click_handler(ClickRecognizerRef recognizer, void *context) {
	numClicks++;
	if (numClicks < 13) {
		displayedMonth--;
		if (displayedMonth == -1) {
			displayedMonth = 11;
			displayedYear--;
		}
	} else {
		displayedYear--;
	}
	
	updateMonthText();
	layer_mark_dirty(monthLayer);
}


void down_single_click_handler(ClickRecognizerRef recognizer, void *context) {
	numClicks++;
	if (numClicks < 13) {
		displayedMonth++;
		if (displayedMonth == 12) {
			displayedMonth = 0;
			displayedYear++;
		}
	} else {
		displayedYear++;
	}

	updateMonthText();
	layer_mark_dirty(monthLayer);
	
}

void select_single_click_handler(ClickRecognizerRef recognizer, void *context) {
	displayedMonth = today.month;
	displayedYear = today.year;
	
	updateMonthText();
	layer_mark_dirty(monthLayer);
}

void click_config_provider(void *context) {
	window_single_click_subscribe(BUTTON_ID_SELECT, (ClickHandler) select_single_click_handler);
	window_raw_click_subscribe(BUTTON_ID_UP, NULL, btn_up_handler, NULL);
	window_single_repeating_click_subscribe(BUTTON_ID_UP, REPEATING_CLICK_INTERVAL, (ClickHandler) up_single_click_handler);
	window_raw_click_subscribe(BUTTON_ID_DOWN, NULL, btn_up_handler, NULL);
	window_single_repeating_click_subscribe(BUTTON_ID_DOWN, REPEATING_CLICK_INTERVAL, (ClickHandler) down_single_click_handler);
}

void setOffsets() {
	if (showWeekNum) {
		DW = (SCREENW-2)/8;
		DX = 1 + (SCREENW-2 - 8*DW)/2;
	} else {
		DW = (SCREENW-2)/7;
		DX = 1 + (SCREENW-2 - 7*DW)/2;
	}
}

void handle_tick(struct tm *now, TimeUnits units_changed) {
	today.day = now->tm_mday;
	today.month = now->tm_mon;
	today.year = now->tm_year + 1900;
	
	displayedMonth = today.month;
	displayedYear = today.year;

	updateMonthText();
	layer_mark_dirty(monthLayer);
}

void applyConfig() {
	setOffsets();
	updateMonthText();
	layer_mark_dirty(monthLayer);
}

bool checkAndSaveInt(int *var, int val, int key) {
	if (*var != val) {
		*var = val;
		persist_write_int(key, val);
		return true;
	} else {
		return false;
	}
}

void in_dropped_handler(AppMessageResult reason, void *context) {
}

void in_received_handler(DictionaryIterator *received, void *context) {
	bool somethingChanged = false;
	
	Tuple *lang = dict_find(received, CONFIG_KEY_LANG);
	Tuple *showweeknum = dict_find(received, CONFIG_KEY_SHOWWEEKNUM);
	Tuple *weekstart = dict_find(received, CONFIG_KEY_WEEKSTART);
	Tuple *nwdcountry = dict_find(received, CONFIG_KEY_NWDCOUNTRY);
	
	if (lang && showweeknum && weekstart && nwdcountry) {
		somethingChanged |= checkAndSaveInt(&curLang, lang->value->int32, CONFIG_KEY_LANG);
		somethingChanged |= checkAndSaveInt(&showWeekNum, showweeknum->value->int32, CONFIG_KEY_SHOWWEEKNUM);
		somethingChanged |= checkAndSaveInt(&weekStartsOnMonday, weekstart->value->int32, CONFIG_KEY_WEEKSTART);
		somethingChanged |= checkAndSaveInt(&nwdCountry, nwdcountry->value->int32, CONFIG_KEY_NWDCOUNTRY);
		
		logVariables("ReceiveHandler");
		
		if (somethingChanged) {
			applyConfig();
		}
	}
}


void readConfig() {
	if (persist_exists(CONFIG_KEY_LANG)) {
		curLang = persist_read_int(CONFIG_KEY_LANG);
	} else {
		curLang = LANG_ENGLISH;
	}
	
	if (persist_exists(CONFIG_KEY_WEEKSTART)) {
		weekStartsOnMonday = persist_read_int(CONFIG_KEY_WEEKSTART);
	} else {
		weekStartsOnMonday = 1;
	}
	
	if (persist_exists(CONFIG_KEY_SHOWWEEKNUM)) {
		showWeekNum = persist_read_int(CONFIG_KEY_SHOWWEEKNUM);
	} else {
		showWeekNum = 1;
	}
	
	if (persist_exists(CONFIG_KEY_NWDCOUNTRY)) {
		nwdCountry = persist_read_int(CONFIG_KEY_NWDCOUNTRY);
	} else {
		nwdCountry = 0;
	}
	
	logVariables("readConfig");
	
}

static void app_message_init(void) {
	app_message_register_inbox_received(in_received_handler);
	app_message_register_inbox_dropped(in_dropped_handler);
	app_message_open(64, 64);
}


void handle_init() {
	Layer *rootLayer;
	time_t curTime;
	struct tm *now;
	
	window = window_create();
	window_stack_push(window, true);
	rootLayer = window_get_root_layer(window);

	app_message_init();
	readConfig();

	curTime = time(NULL);
	now = localtime(&curTime);
	today.day = now->tm_mday;
	today.month = now->tm_mon;
	today.year = now->tm_year + 1900;
	
	displayedMonth = today.month;
	displayedYear = today.year;
	
	setOffsets();
	
	DH = MONTH_LAYER_HEIGHT/7;
	DY = (MONTH_LAYER_HEIGHT - 7*DH)/2;

	monthNameLayer = text_layer_create(GRect(XOFFSET, YOFFSET, SCREENW, MONTHNAME_LAYER_HEIGHT));
	text_layer_set_background_color(monthNameLayer, GColorWhite);
	text_layer_set_text_color(monthNameLayer, GColorBlack);
	text_layer_set_font(monthNameLayer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
	text_layer_set_text_alignment(monthNameLayer, GTextAlignmentCenter);
	layer_add_child(rootLayer, text_layer_get_layer(monthNameLayer));
	
	updateMonthText();
	
	monthLayer = layer_create(GRect(XOFFSET, MONTHNAME_LAYER_HEIGHT+YOFFSET, SCREENW, MONTH_LAYER_HEIGHT));
	layer_set_update_proc(monthLayer, updateMonth);
	layer_add_child(rootLayer, monthLayer);
	
	window_set_click_config_provider(window, (ClickConfigProvider) click_config_provider);
	
	tick_timer_service_subscribe(DAY_UNIT, handle_tick);
}

void handle_deinit() {
	tick_timer_service_unsubscribe();
	
	text_layer_destroy(monthNameLayer);
	layer_destroy(monthLayer);
	window_destroy(window);
}

int main(void) {
	handle_init();
	app_event_loop();
	handle_deinit();
}

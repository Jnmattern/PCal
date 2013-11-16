#include <pebble.h>

#include <string.h>
#include "xprintf.h"

// Languages
#define LANG_DUTCH 0
#define LANG_ENGLISH 1
#define LANG_FRENCH 2
#define LANG_GERMAN 3
#define LANG_SPANISH 4
#define LANG_ITALIAN 5
#define LANG_MAX 6

// Non Working Days Country
#define NWD_NONE 0
#define NWD_FRANCE 1
#define NWD_USA 2
#define NWD_MAX 3

// Compilation-time options
#define LANG_CUR LANG_FRENCH
#define NWD_COUNTRY NWD_FRANCE
#define WEEK_STARTS_ON_SUNDAY false
#define SHOW_WEEK_NUMBERS true

#if LANG_CUR == LANG_DUTCH
#define APP_NAME "Kalender"
#elif LANG_CUR == LANG_FRENCH
#define APP_NAME "Calendrier"
#elif LANG_CUR == LANG_GERMAN
#define APP_NAME "Kalender"
#elif LANG_CUR == LANG_SPANISH
#define APP_NAME "Calendario"
#elif LANG_CUR == LANG_ITALIAN
#define APP_NAME "Calendario"
#else // Defaults to English
#define APP_NAME "Calendar"
#endif

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


static const char *windowName = APP_NAME;

static const char *monthNames[] = {
#if LANG_CUR == LANG_DUTCH
	"Januari", "Februari", "Maart", "April", "Mei", "Juni", "Juli", "Augustus", "September", "Oktober", "November", "December"
#elif LANG_CUR == LANG_FRENCH
	"Janvier", "Février", "Mars", "Avril", "Mai", "Juin", "Juillet", "Août", "Septembre", "Octobre", "Novembre", "Décembre"
#elif LANG_CUR == LANG_GERMAN
	"Januar", "Februar", "März", "April", "Mai", "Juni", "Juli", "August", "September", "Oktober", "November", "Dezember"
#elif LANG_CUR == LANG_SPANISH
	"Enero", "Febrero", "Marzo", "Abril", "Mayo", "Junio", "Julio", "Augusto", "Septiembre", "Octubre", "Noviembre", "Diciembre"
#elif LANG_CUR == LANG_ITALIAN
	"Gennaio", "Febbraio", "Marzo", "Aprile", "Maggio", "Giugno", "Luglio", "Agosto", "Settembre", "Ottobre", "Novembre", "Dicembre"
#else // Defaults to English
	"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"
#endif
};

static const char *weekDays[] = {
#if LANG_CUR == LANG_DUTCH
	"Zo", "Ma", "Di", "Wo", "Do", "Vr", "Za"	// Dutch
#elif LANG_CUR == LANG_FRENCH
	"Di", "Lu", "Ma", "Me", "Je", "Ve", "Sa"	// French
#elif LANG_CUR == LANG_GERMAN
	"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"	// German
#elif LANG_CUR == LANG_SPANISH
	"Do", "Lu", "Ma", "Mi", "Ju", "Vi", "Sá"	// Spanish
#elif LANG_CUR == LANG_ITALIAN
	"Do", "Lu", "Ma", "Me", "Gi", "Ve", "Sa"	// Italian
#else // Defaults to English
	"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"	// English
#endif
};

#define SCREENW 144
#define SCREENH 168
#define MENUBAR_HEIGHT 16
#define MONTHNAME_LAYER_HEIGHT 20
#define MONTH_LAYER_HEIGHT (SCREENH-MENUBAR_HEIGHT-MONTHNAME_LAYER_HEIGHT)
#define REPEATING_CLICK_INTERVAL 200

typedef struct {
	int day;
	int month;
	int year;
} Date;

Window *window;
Layer *monthLayer;
TextLayer *monthNameLayer;
Date today;
int displayedMonth, displayedYear;
//GFont myFont;

static int DX, DY, DW, DH;

#define NUM_NON_WORKING_DAYS 11
typedef struct {
	char name[30];
	Date date;
} nonWorkingDay;

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
#if SHOW_WEEK_NUMBERS
static int weekNumber(const Date *theDate) {
	int J = julianDay(theDate);
	
	int d4 = (J+31741-(J%7))%146097%36524%1461;
	int L = (int)(d4/1460);
	int d1 = ((d4-L)%365)+L;
	
	return (int)(d1/7)+1;
}
#endif

static bool isLeapYear(const int Y) {
	return (((Y%4 == 0) && (Y%100 != 0)) || (Y%400 == 0));
}

static int numDaysInMonth(const int M, const int Y) {
	static const int nDays[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
	
	return nDays[M] + (M == FEB)*isLeapYear(Y);
}

#define Date(d, m, y) ((Date){ (d), (m), (y) })

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

static void dateAddDays(Date *date, int numDays) {
	int i;
	
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
}

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

#if NWD_COUNTRY == NWD_FRANCE
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

static void whitMonday(const int Y, Date *theDate) {
	easterMonday(Y, theDate);
	dateAddDays(theDate, 49);
}
#endif // NWD_COUNTRY == NWD_FRANCE

#if NWD_COUNTRY == NWD_USA
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
#endif // NWD_COUNTRY == NWD_USA

# if NWD_COUNTRY != NWD_NONE
static bool isNonWorkingDay(const Date *theDate) {
	Date d;
	
	// Common public holidays
	if (theDate->day == 1  && theDate->month == JAN) return true; // New year's day
	if (theDate->day == 11 && theDate->month == NOV) return true; // Armistice 1918 // Veteran's day
	if (theDate->day == 25 && theDate->month == DEC) return true; // Noël // Christmas

#if NWD_COUNTRY == NWD_FRANCE
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

#elif NWD_COUNTRY == NWD_USA
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

#endif
	return false;
}

#else // NWD_COUNTRY != NWD_NONE
static bool isNonWorkingDay(const Date *theDate) {
	return false;
}
#endif //  NWD_COUNTRY != NWD_NONE


void updateMonthText() {
	xsprintf(monthName, "%s %d", monthNames[displayedMonth], displayedYear);
	text_layer_set_text(monthNameLayer, monthName);
}

void updateMonth(Layer *layer, GContext *ctx) {
	static char numStr[3] = "";
	int i, x, s, numWeeks, dy, firstday, numDays, l=0, c=0, w;
	Date first, d;
	GFont f, fontNorm, fontBold;
	GRect rect, fillRect;
	
	first = Date(1, displayedMonth, displayedYear);
#if WEEK_STARTS_ON_SUNDAY
	firstday = dayOfWeek(&first);
#else
	firstday = (dayOfWeek(&first)+6)%7;
#endif
	numDays = numDaysInMonth(displayedMonth, displayedYear);
	
	numWeeks = (firstday+6+numDays)/7;
	
	dy = DY + DH*(6-numWeeks)/2;
	
	graphics_context_set_stroke_color(ctx, GColorBlack);
	graphics_context_set_fill_color(ctx, GColorBlack);
	
	// Calendar Grid
#if SHOW_WEEK_NUMBERS
	x = DX+DW;
#else
	x = DX;
#endif

	// Black Top Line with days of week
	graphics_fill_rect(ctx, GRect(x, dy, DW*7+1, DH), 4, GCornersTop);

#if SHOW_WEEK_NUMBERS
	// Black left column for week numbers
	graphics_fill_rect(ctx, GRect(DX, dy+DH, DW, numWeeks*DH+1), 4, GCornersLeft);
#endif

#if SHOW_WEEK_NUMBERS
	x = DX+DW;
	w = DW*7;
#else
	x = DX+1;
	w = DW*7-1;
#endif
	// Double line on the outside
	graphics_draw_round_rect(ctx, GRect(x, dy+DH, w, numWeeks*DH), 0);
	
	// Column(s) for the week-end or sunday
#if WEEK_STARTS_ON_SUNDAY
	x = DX+DW+1;
#else
	x = DX+5*DW+1;
#endif

#if SHOW_WEEK_NUMBERS
	x += DW;
#endif

	graphics_draw_line(ctx, GPoint(x, dy+DH), GPoint(x, dy+DH+numWeeks*DH-1));
	
#if SHOW_WEEK_NUMBERS
	x = 1;
#else
	x = 0;
#endif

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
	
#if WEEK_STARTS_ON_SUNDAY
	s = 0;
#else
	s = 1;
#endif

#if SHOW_WEEK_NUMBERS
	x = 1;
#else
	x = 0;
#endif

	// Days of week
	graphics_context_set_text_color(ctx, GColorWhite);
	
	for (i=s; i<s+7; i++) {
		graphics_draw_text(ctx, weekDays[i%7], fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(DX+DW*(i+x-s), dy, DW+2, DH+1), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
	}
	
#if SHOW_WEEK_NUMBERS
	// Week numbers
	for (i=0, d=first; i<=numWeeks; i++, d.day+=7) {
		xsprintf(numStr, "%d", weekNumber(&d));
		graphics_draw_text(ctx, numStr, fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(DX, dy+DH*(i+1), DW, DH+1), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
	}
#endif
	
	// Day numbers
	graphics_context_set_text_color(ctx, GColorBlack);
	
	for (i=1; i<=numDays; i++) {
		c = (firstday - 1 + i)%7;
		if (c == 0 && i != 1) {
			l++;
		}
		
		xsprintf(numStr, "%d", i);

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
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Entering down_single_click_handler");
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

void handle_tick(struct tm *now, TimeUnits units_changed) {
	updateMonthText();
	layer_mark_dirty(monthLayer);
}

void handle_init() {
	Layer *rootLayer;
	time_t curTime;
	struct tm *now;
	
	window = window_create();
	window_stack_push(window, true);
	rootLayer = window_get_root_layer(window);

	curTime = time(NULL);
	now = localtime(&curTime);
	today.day = now->tm_mday;
	today.month = now->tm_mon;
	today.year = now->tm_year + 1900;
	
	displayedMonth = today.month;
	displayedYear = today.year;
	
#if SHOW_WEEK_NUMBERS
	DW = (SCREENW-2)/8;
	DX = 1 + (SCREENW-2 - 8*DW)/2;
#else
	DW = (SCREENW-2)/7;
	DX = 1 + (SCREENW-2 - 7*DW)/2;
#endif
	DH = MONTH_LAYER_HEIGHT/7;
	DY = (MONTH_LAYER_HEIGHT - 7*DH)/2;

	monthNameLayer = text_layer_create(GRect(0, 0, SCREENW, MONTHNAME_LAYER_HEIGHT));
	text_layer_set_background_color(monthNameLayer, GColorWhite);
	text_layer_set_text_color(monthNameLayer, GColorBlack);
	text_layer_set_font(monthNameLayer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
	text_layer_set_text_alignment(monthNameLayer, GTextAlignmentCenter);
	layer_add_child(rootLayer, text_layer_get_layer(monthNameLayer));
	
	updateMonthText();
	
	monthLayer = layer_create(GRect(0, MONTHNAME_LAYER_HEIGHT, SCREENW, MONTH_LAYER_HEIGHT));
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

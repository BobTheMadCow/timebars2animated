#include <pebble.h>
#include <settings.h>

//uncomment to disable app logging
#undef APP_LOG
#define APP_LOG(...)
	
/********************
	 vHOUR_LOC
	+--------+ 
	| 12  59 | NUM_LABEL_OFFSET
	|,--.,--.| BAR_MAX_LOC
	||  ||  || 
	||  ||  || 
	|+--++--+| BAR_MIN_LOC 
	|HOURMINS| TEXT_LABEL_MAX_LOC
	+--------+ TEXT_LABEL_MIN_LOC
	     ^MINUTE_LOC
*********************/
	
#define TEXT_FONT_ID RESOURCE_ID_FONT_TYPEONE_24
#define NUM_FONT_ID RESOURCE_ID_FONT_TYPEONE_34

#define CORNER_MASK GCornersTop
#define CORNER_SIZE 12
#define BAR_MAX_LOC 22
#define BAR_MIN_LOC 150
#define MAX_HEIGHT 128 //(BAR_MIN_LOC - BAR_MAX_LOC)
#define HOUR_LOC 4
#define MINUTE_LOC 74
#define HOUR_WIDTH 66
#define MINUTE_WIDTH 66
#define HOUR_UNIT_HEIGHT (MAX_HEIGHT/12.0f)
#define MINUTE_UNIT_HEIGHT (MAX_HEIGHT/60.0f)

float adjusted_hour_unit_height = HOUR_UNIT_HEIGHT;

#define TEXT_LABEL_MAX_LOC 142
#define TEXT_LABEL_MIN_LOC 168
#define TEXT_LABEL_HEIGHT 26 //(TEXT_LABEL_MIN_LOC - TEXT_LABEL_MAX_LOC)
#define NUM_LABEL_OFFSET -14 //(BAR_MAX_LOC + 14)
#define NUM_LABEL_HEIGHT 36

#define LABEL_TEXT "HOUR  MINS"

GFont *text_font;
GFont *num_font;

float hour;
float minute;
int current_hour;
int current_minute;

static char hour_text[] = "  "; 
static char minute_text[] = "  ";

char *hour_num_format; //set based on 24h_mode in init()
char *min_num_format = "%02u";

Window *window;
Layer *root_layer;
Layer *bar_clipping_layer;
Layer *minute_bar_layer;
Layer *hour_bar_layer;
TextLayer *text_label_layer;

static PropertyAnimation *hour_up_animation;
static PropertyAnimation *hour_down_animation;
static PropertyAnimation *minute_up_animation;
static PropertyAnimation *minute_down_animation;

#define CURVE AnimationCurveEaseInOut

GRect hour_from_frame;
GRect hour_to_frame;
GRect minute_from_frame;
GRect minute_to_frame;

static int round_float_to_int(float x)
{
	if((x-(int)x) >= 0.5){return (int)x + 1;}
	else{return (int)x;}
}

static void graphics_context_set_colors(GContext *ctx)
{
	graphics_context_set_stroke_color(ctx, fg_color);
	graphics_context_set_fill_color(ctx, fg_color);
	graphics_context_set_text_color(ctx, fg_color);
}

static void animate_hour_down(int from_time, int to_time)
{
	if(!animation_is_scheduled((Animation*)hour_down_animation))
	{
		if(animation_is_scheduled((Animation*)hour_up_animation))
		{
			animation_unschedule((Animation*)hour_up_animation);
			property_animation_destroy(hour_up_animation);
			hour_up_animation = NULL;
		}
		
		hour_from_frame = GRect(HOUR_LOC, (int)(MAX_HEIGHT - (from_time * adjusted_hour_unit_height)), HOUR_WIDTH, BAR_MIN_LOC);
		hour_to_frame = GRect(HOUR_LOC, (int)(MAX_HEIGHT - (to_time * adjusted_hour_unit_height)), HOUR_WIDTH, BAR_MIN_LOC);
		
		property_animation_destroy(hour_down_animation);
		hour_down_animation = NULL;
		
		hour_down_animation = property_animation_create_layer_frame(hour_bar_layer, &hour_from_frame, &hour_to_frame);
		animation_set_curve((Animation*)hour_down_animation, CURVE); //defaults to EaseInOut
		animation_set_delay((Animation*)hour_down_animation, (animation_duration / 4));
		animation_set_duration((Animation*)hour_down_animation, ((animation_duration / 4) * 3));
		animation_schedule((Animation*)hour_down_animation);
	}
}

static void animate_hour_up(int from_time, int to_time)
{
	if(!animation_is_scheduled((Animation*)hour_up_animation))
	{
		if(animation_is_scheduled((Animation*)hour_down_animation))
		{
			animation_unschedule((Animation*)hour_down_animation);
			property_animation_destroy(hour_down_animation);
			hour_down_animation = NULL;
		}
		
		hour_from_frame = GRect(HOUR_LOC, (int)(MAX_HEIGHT - (from_time * adjusted_hour_unit_height)), HOUR_WIDTH, BAR_MIN_LOC);
		hour_to_frame = GRect(HOUR_LOC, (int)(MAX_HEIGHT - (to_time * adjusted_hour_unit_height)), HOUR_WIDTH, BAR_MIN_LOC);
		
		property_animation_destroy(hour_up_animation);
		hour_up_animation = NULL;
		
		hour_up_animation = property_animation_create_layer_frame(hour_bar_layer, &hour_from_frame, &hour_to_frame);
		animation_set_curve((Animation*)hour_up_animation, CURVE); //defaults to EaseInOut
		animation_set_delay((Animation*)hour_up_animation, (animation_duration / 4));
		animation_set_duration((Animation*)hour_up_animation, ((animation_duration / 4) * 3));
		animation_schedule((Animation*)hour_up_animation);
	}
}

static void animate_minute_down(int from_time, int to_time)
{
	if(!animation_is_scheduled((Animation*)minute_down_animation))
	{
		if(animation_is_scheduled((Animation*)minute_up_animation))
		{
			animation_unschedule((Animation*)minute_up_animation);
			property_animation_destroy(minute_up_animation);
			minute_up_animation = NULL;
		}
				
		minute_from_frame = GRect(MINUTE_LOC, (int)(MAX_HEIGHT - (from_time * MINUTE_UNIT_HEIGHT)), MINUTE_WIDTH, BAR_MIN_LOC);
		minute_to_frame = GRect(MINUTE_LOC, (int)(MAX_HEIGHT - (to_time * MINUTE_UNIT_HEIGHT)), MINUTE_WIDTH, BAR_MIN_LOC);
		
		property_animation_destroy(minute_down_animation);
		minute_down_animation = NULL;
		
		minute_down_animation = property_animation_create_layer_frame(minute_bar_layer, &minute_from_frame, &minute_to_frame);
		animation_set_curve((Animation*)minute_down_animation, CURVE); //defaults to EaseInOut
		animation_set_delay((Animation*)minute_down_animation, (animation_duration / 4));
		animation_set_duration((Animation*)minute_down_animation, ((animation_duration / 4) * 3));
		animation_schedule((Animation*)minute_down_animation);
	}
}

static void animate_minute_up(int from_time, int to_time)
{
	if(!animation_is_scheduled((Animation*)minute_up_animation))
	{
		if(animation_is_scheduled((Animation*)minute_down_animation))
		{
			animation_unschedule((Animation*)minute_down_animation);
			property_animation_destroy(minute_down_animation);
			minute_down_animation = NULL;
		}
				
		minute_from_frame = GRect(MINUTE_LOC, (int)(MAX_HEIGHT - (from_time * MINUTE_UNIT_HEIGHT)), HOUR_WIDTH, BAR_MIN_LOC);
		minute_to_frame = GRect(MINUTE_LOC, (int)(MAX_HEIGHT - (to_time * MINUTE_UNIT_HEIGHT)), HOUR_WIDTH, BAR_MIN_LOC);
		
		property_animation_destroy(minute_up_animation);
		minute_up_animation = NULL;
		
		minute_up_animation = property_animation_create_layer_frame(minute_bar_layer, &minute_from_frame, &minute_to_frame);
		animation_set_curve((Animation*)minute_up_animation, CURVE); //defaults to EaseInOut
		animation_set_delay((Animation*)minute_up_animation, (animation_duration / 4));
		animation_set_duration((Animation*)minute_up_animation, ((animation_duration / 4) * 3));
		animation_schedule((Animation*)minute_up_animation);
	}
}

static void hour_bar_layer_draw(Layer *layer, GContext *ctx) 
{
	hour = (MAX_HEIGHT - ((layer_get_frame(layer)).origin.y))/adjusted_hour_unit_height;

	graphics_context_set_colors(ctx);
	snprintf(hour_text, sizeof(hour_text), hour_num_format, round_float_to_int(hour));

	graphics_fill_rect(ctx, GRect(0, BAR_MAX_LOC, HOUR_WIDTH, MAX_HEIGHT), CORNER_SIZE, CORNER_MASK);
  	graphics_draw_text(ctx, hour_text, num_font, GRect(0, NUM_LABEL_OFFSET, HOUR_WIDTH, NUM_LABEL_HEIGHT), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

static void minute_bar_layer_draw(Layer *layer, GContext *ctx) 
{
	minute = (MAX_HEIGHT - ((layer_get_frame(layer)).origin.y))/MINUTE_UNIT_HEIGHT;

	graphics_context_set_colors(ctx);
	snprintf(minute_text, sizeof(minute_text), min_num_format, round_float_to_int(minute));
	
	graphics_fill_rect(ctx, GRect(0, BAR_MAX_LOC, MINUTE_WIDTH, MAX_HEIGHT), CORNER_SIZE, CORNER_MASK);
	graphics_draw_text(ctx, minute_text, num_font, GRect(0, NUM_LABEL_OFFSET, MINUTE_WIDTH, NUM_LABEL_HEIGHT), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed)
{
	current_hour = tick_time->tm_hour;
	current_minute = tick_time->tm_min;

	if(!clock_is_24h_style())
	{
		current_hour = current_hour%12;		
		if(current_hour == 0){current_hour = 12;}	//for correct height bar in 12 hour mode
	}
	
	if(current_minute == 0)
	{
		if(current_hour == 0 && clock_is_24h_style())
		{
			animate_hour_down(24, 0);
		}
		else if(current_hour == 1 && !clock_is_24h_style()) 
		{
			animate_hour_down(12, 1);
		}
		else
		{	
			animate_hour_up(current_hour - 1, current_hour);
		}
		animate_minute_down(60, 0);
	}
	else //current_minute != 0
	{
		animate_minute_up(current_minute - 1, current_minute);
	}
	
	if(vibrate && tick_time->tm_min == 0)
	{
		vibes_double_pulse();
	}
}

void init(void) 
{
	time_t now = time(NULL);
  	struct tm *time = localtime(&now);
	int start_hour = time->tm_hour;
	int start_min = time->tm_min;
	
	if(clock_is_24h_style())
	{
		adjusted_hour_unit_height = HOUR_UNIT_HEIGHT / 2.0f;
		hour_num_format = "%02u"; //use leading zeros in 24 hour mode
	}
	else
	{
		hour_num_format = "%u";	//no leading zeros in 12 hour mode
		start_hour = start_hour%12;		
		if(start_hour == 0){start_hour = 12;}	//for correct height bar in 12 hour mode
	}
	
	text_font = fonts_load_custom_font(resource_get_handle(TEXT_FONT_ID));
	num_font = fonts_load_custom_font(resource_get_handle(NUM_FONT_ID));
	
	load_settings();
	
	window = window_create();
	window_set_background_color(window, bg_color);
	window_stack_push(window, true);
	root_layer = window_get_root_layer(window);
	
	//prevents bars being drawn over the labels at the bottom.
	bar_clipping_layer = layer_create(GRect(0, 0, 144, BAR_MIN_LOC));
	layer_set_clips(bar_clipping_layer, true);
	layer_add_child(root_layer, bar_clipping_layer);
	
	hour_bar_layer = layer_create(GRect(HOUR_LOC, MAX_HEIGHT, HOUR_WIDTH, BAR_MIN_LOC));
	layer_set_update_proc(hour_bar_layer, hour_bar_layer_draw);
	layer_add_child(bar_clipping_layer, hour_bar_layer);

	minute_bar_layer = layer_create(GRect(MINUTE_LOC, MAX_HEIGHT, MINUTE_WIDTH, BAR_MIN_LOC));
	layer_set_update_proc(minute_bar_layer, minute_bar_layer_draw);
	layer_add_child(bar_clipping_layer, minute_bar_layer);	

	animate_hour_up(0, start_hour);
	animate_minute_up(0, start_min);	
	
	text_label_layer = text_layer_create(GRect(0, TEXT_LABEL_MAX_LOC, 144, TEXT_LABEL_HEIGHT));
	text_layer_set_text_color(text_label_layer, fg_color);
	text_layer_set_background_color(text_label_layer, GColorClear);
	text_layer_set_text_alignment(text_label_layer, GTextAlignmentCenter);
	text_layer_set_font(text_label_layer, text_font);
	text_layer_set_text(text_label_layer, LABEL_TEXT);
	layer_add_child(root_layer, text_layer_get_layer(text_label_layer));
	
	inverter_layer = inverter_layer_create(GRect(0,0,144,168));
	layer_add_child(root_layer, inverter_layer_get_layer(inverter_layer));
	layer_set_hidden(inverter_layer_get_layer(inverter_layer), !invert_colors); //hide layer to not invert colors
	
	tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);	
}

void deinit(void) 
{
	fonts_unload_custom_font(text_font);
	fonts_unload_custom_font(num_font);
	layer_destroy(hour_bar_layer);
	layer_destroy(minute_bar_layer);
	layer_destroy(bar_clipping_layer);
	inverter_layer_destroy(inverter_layer);
	text_layer_destroy(text_label_layer);
	tick_timer_service_unsubscribe();
	window_destroy(window);
	property_animation_destroy(hour_down_animation);
	property_animation_destroy(hour_up_animation);
	property_animation_destroy(minute_down_animation);
	property_animation_destroy(minute_up_animation);
	save_settings();
}

int main(void) 
{
	init();
	app_event_loop();
	deinit();
}
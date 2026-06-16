#include "dam_ui.h"
#include "dam_nvs.h"
#include <stdio.h>
#include <stdint.h>

extern const lv_font_t lv_font_montserrat_digits_64;

// ─── Colours ─────────────────────────────────────────────────────────────────
#define C_BG          lv_color_hex(0x0D1117)
#define C_HEADER      lv_color_hex(0x161B22)
#define C_SURFACE     lv_color_hex(0x21262D)
#define C_BORDER      lv_color_hex(0x30363D)
#define C_ACCENT      lv_color_hex(0x58A6FF)
#define C_MUTE_ON     lv_color_hex(0xF85149)
#define C_TEXT        lv_color_hex(0xE6EDF3)
#define C_TEXT_DIM    lv_color_hex(0x7D8590)

// ─── Layout — 640 × 480 landscape ────────────────────────────────────────────
//
//  Y=0   ┌─────────────────────────────────────────────────────────────────┐
//        │  DAM 1021                                                 v1.0  │ HEADER 50px
//  Y=50  ├──────────────────────┬──────────────────────────────┬───────────┤
//        │  ╭─────────────────╮ │                              │           │
//        │ /   50   -49 dB     \│  INPUT                       │  ▲  BRT   │
//        │ \                   /│  [ ─── AUTO ─── ]            │  │        │
//        │  ╰─────────────────╯ │                              │  │        │
//        │  [ ── MUTE ───────]  │  FILTER                      │  ▼        │
//        │                      │  [ ─── LIN ──── ]            │  80%      │
//  Y=480 └──────────────────────┴──────────────────────────────┴───────────┘
//        x=0        x=272      x=273                           x=580     x=639

#define SCR_W        640
#define SCR_H        480
#define PAD           32
#define LBL_H         20
#define HEADER_H      68    

// ── Left panel – arc + mute  ────────────────────────
#define LP_W          300
#define ARC_SIZE      250   
#define ARC_R_PAD     18   
#define ARC_TRACK_W   22
#define MUTE_H        50
#define MUTE_W        (ARC_SIZE - 64)   // 32px extra left inset vs centred
#define ARC_MUTE_GAP  8

// Vertically centre [arc + gap + mute] in left panel
#define LP_CONTENT_H  (ARC_SIZE + ARC_MUTE_GAP + MUTE_H)
#define LP_TOP_PAD    ((SCR_H - HEADER_H - LP_CONTENT_H) / 2)
#define ARC_X         (LP_W - ARC_SIZE - ARC_R_PAD)   // right-aligned
#define ARC_Y         (HEADER_H + LP_TOP_PAD)
#define MUTE_X        (ARC_X + (ARC_SIZE - MUTE_W) / 2)   // centred under arc
#define MUTE_Y        (ARC_Y + ARC_SIZE + ARC_MUTE_GAP)

// ── Right panel ───────────────────────────────────────────────────────────────
#define RP_X          (LP_W + 1)            // 301
#define RP_W          (SCR_W - RP_X)        // 367
#define RP_INNER_X    (RP_X + PAD)          // 283
#define RP_INNER_W    (RP_W - 2 * PAD)      // 347

// Brightness slider – far-right column
#define BRIGHT_SLIDER_W   16
#define BRIGHT_SLIDER_H   292   // −48px (10% SCR_H) shorter at bottom
#define BRIGHT_SLIDER_X   (RP_X + RP_W - PAD - BRIGHT_SLIDER_W - 24)  // −24px left
#define BRIGHT_SLIDER_Y   (HEADER_H + ((SCR_H - HEADER_H - BRIGHT_SLIDER_H) / 2) - 4)

// Single-cycle buttons
#define ROW_H          66    
#define BTN_W          (BRIGHT_SLIDER_X - RP_INNER_X - PAD)  // 297

// Vertically centre [inp + flt + sty] in right panel
#define RP_LBL_GAP     6
#define RP_SECT_GAP    22
#define RP_CONTENT_H   (3 * (LBL_H + RP_LBL_GAP + ROW_H) + 2 * RP_SECT_GAP)
#define RP_TOP_PAD     (((SCR_H - HEADER_H - RP_CONTENT_H) / 2) - 18)
#define INP_LBL_Y      (HEADER_H + RP_TOP_PAD)
#define INP_BTN_Y      (INP_LBL_Y + LBL_H + RP_LBL_GAP)
#define FLT_LBL_Y      (INP_BTN_Y + ROW_H + RP_SECT_GAP)
#define FLT_BTN_Y      (FLT_LBL_Y + LBL_H + RP_LBL_GAP)
#define STY_LBL_Y      (FLT_BTN_Y + ROW_H + RP_SECT_GAP)
#define STY_BTN_Y      (STY_LBL_Y + LBL_H + RP_LBL_GAP)

// ─── Names ───────────────────────────────────────────────────────────────────
static const char * const _inp_names[DAC_INPUT_COUNT]  = { "AUTO", "USB", "SPDIF", "OPT" };
static const char * const _flt_names[DAC_FILTER_COUNT] = { "LIN",  "MIX", "MIN",   "SOFT" };

// ─── Colour themes (accent only; mute is always red) ─────────────────────────
static const uint32_t _theme_hex[]        = { 0x3498DB, 0x16A085, 0x95A5A6 };
static const char * const _theme_names[] = { "RIVER",   "GREEN",  "CONCRETE" };
#define THEME_COUNT  3
static int _theme_idx = 0;

// ─── Styles ───────────────────────────────────────────────────────────────────
static lv_style_t _sty_scr;
static lv_style_t _sty_lbl;
static lv_style_t _sty_cycle;     // single tap-to-cycle buttons
static lv_style_t _sty_mute_off;
static lv_style_t _sty_mute_on;

// ─── Widgets ──────────────────────────────────────────────────────────────────
static lv_obj_t *_arc            = NULL;
static lv_obj_t *_vol_num        = NULL;
static lv_obj_t *_vol_db         = NULL;
static lv_obj_t *_inp_btn        = NULL;
static lv_obj_t *_inp_btn_lbl    = NULL;
static lv_obj_t *_flt_btn        = NULL;
static lv_obj_t *_flt_btn_lbl    = NULL;
static lv_obj_t *_sty_btn        = NULL;
static lv_obj_t *_sty_btn_lbl    = NULL;
static lv_obj_t *_mute_btn       = NULL;
static lv_obj_t *_mute_lbl       = NULL;
static lv_obj_t *_bright_slider  = NULL;
static lv_obj_t *_bright_pct_lbl = NULL;

static dam_action_cb_t _cb       = NULL;
static bool            _prog_set = false;

// ─── Theme application ───────────────────────────────────────────────────────
static void _apply_theme(int idx)
{
    lv_color_t c = lv_color_hex(_theme_hex[idx]);

    lv_obj_set_style_arc_color   (_arc,          c, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color    (_bright_slider, c, LV_PART_INDICATOR);

    lv_obj_set_style_border_color(_inp_btn, c, 0);
    lv_obj_set_style_text_color  (_inp_btn, c, 0);
    lv_obj_set_style_border_color(_flt_btn, c, 0);
    lv_obj_set_style_text_color  (_flt_btn, c, 0);
    lv_obj_set_style_border_color(_sty_btn, c, 0);
    lv_obj_set_style_text_color  (_sty_btn, c, 0);

    lv_label_set_text(_sty_btn_lbl, _theme_names[idx]);
}

// ─── Style init ───────────────────────────────────────────────────────────────
static void _init_styles(void)
{
    lv_style_init(&_sty_scr);
    lv_style_set_bg_color    (&_sty_scr, C_BG);
    lv_style_set_bg_opa      (&_sty_scr, LV_OPA_COVER);
    lv_style_set_border_width(&_sty_scr, 0);
    lv_style_set_pad_all     (&_sty_scr, 0);

    lv_style_init(&_sty_lbl);
    lv_style_set_text_color(&_sty_lbl, C_TEXT_DIM);
    lv_style_set_text_font (&_sty_lbl, &lv_font_montserrat_20);

    // Single tap-to-cycle buttons (input / filter)
    lv_style_init(&_sty_cycle);
    lv_style_set_bg_color    (&_sty_cycle, C_SURFACE);
    lv_style_set_bg_opa      (&_sty_cycle, LV_OPA_COVER);
    lv_style_set_border_color(&_sty_cycle, C_ACCENT);
    lv_style_set_border_width(&_sty_cycle, 2);
    lv_style_set_radius      (&_sty_cycle, 10);
    lv_style_set_text_color  (&_sty_cycle, C_ACCENT);
    lv_style_set_text_font   (&_sty_cycle, &lv_font_montserrat_28);
    lv_style_set_pad_all     (&_sty_cycle, 0);

    // Mute button – not muted
    lv_style_init(&_sty_mute_off);
    lv_style_set_bg_color    (&_sty_mute_off, C_SURFACE);
    lv_style_set_bg_opa      (&_sty_mute_off, LV_OPA_COVER);
    lv_style_set_border_color(&_sty_mute_off, C_BORDER);
    lv_style_set_border_width(&_sty_mute_off, 2);
    lv_style_set_radius      (&_sty_mute_off, 12);
    lv_style_set_text_color  (&_sty_mute_off, C_TEXT_DIM);
    lv_style_set_text_font   (&_sty_mute_off, &lv_font_montserrat_24);

    // Mute button – muted (red)
    lv_style_init(&_sty_mute_on);
    lv_style_set_bg_color    (&_sty_mute_on, C_MUTE_ON);
    lv_style_set_bg_opa      (&_sty_mute_on, LV_OPA_COVER);
    lv_style_set_border_width(&_sty_mute_on, 0);
    lv_style_set_radius      (&_sty_mute_on, 12);
    lv_style_set_text_color  (&_sty_mute_on, C_TEXT);
    lv_style_set_text_font   (&_sty_mute_on, &lv_font_montserrat_24);
}

// ─── Event callbacks ──────────────────────────────────────────────────────────
static void _on_arc(lv_event_t *e)
{
    if (_prog_set) return;
    int val = lv_arc_get_value(lv_event_get_target(e));
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d", val);
    lv_label_set_text(_vol_num, buf);
    char db_buf[16];
    snprintf(db_buf, sizeof(db_buf), "%d dB", val - 99);
    lv_label_set_text(_vol_db, db_buf);
    if (_cb) _cb(ACT_VOL_SET, val);
}

static void _on_mute(lv_event_t *e)
{
    (void)e;
    if (_cb) _cb(ACT_MUTE, 0);
}

static void _on_inp_cycle(lv_event_t *e)
{
    (void)e;
    if (_cb) _cb(ACT_CHANNEL_RIGHT, 0);
}

static void _on_flt_cycle(lv_event_t *e)
{
    (void)e;
    if (_cb) _cb(ACT_FILTER_CYCLE, 0);
}

static void _on_sty_cycle(lv_event_t *e)
{
    (void)e;
    _theme_idx = (_theme_idx + 1) % THEME_COUNT;
    _apply_theme(_theme_idx);
}

static void _on_brightness(lv_event_t *e)
{
    if (_prog_set) return;
    int val = lv_slider_get_value(lv_event_get_target(e));
    if (_bright_pct_lbl) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%d%%", val);
        lv_label_set_text(_bright_pct_lbl, buf);
    }
    if (_cb) _cb(ACT_BRIGHTNESS_SET, val);
}

// ─── Layout builder ───────────────────────────────────────────────────────────
static void _build(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_add_style(scr, &_sty_scr, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    // ── Header ────────────────────────────────────────────────────────────────
    lv_obj_t *hdr = lv_obj_create(scr);
    lv_obj_set_size(hdr, SCR_W, HEADER_H);
    lv_obj_set_pos (hdr, 0, 0);
    lv_obj_set_style_bg_color    (hdr, C_HEADER, 0);
    lv_obj_set_style_bg_opa      (hdr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_side (hdr, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(hdr, C_BORDER, 0);
    lv_obj_set_style_border_width(hdr, 1, 0);
    lv_obj_set_style_radius      (hdr, 0, 0);
    lv_obj_set_style_pad_all     (hdr, 0, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(hdr);
    lv_label_set_text(title, "DAM 1021");
    lv_obj_set_style_text_color(title, C_TEXT_DIM, 0);
    lv_obj_set_style_text_font (title, &lv_font_montserrat_36, 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, PAD, 8);

    lv_obj_t *ver = lv_label_create(hdr);
    lv_label_set_text(ver, "v1.1");
    lv_obj_set_style_text_color(ver, C_TEXT_DIM, 0);
    lv_obj_set_style_text_font (ver, &lv_font_montserrat_20, 0);
    lv_obj_align(ver, LV_ALIGN_RIGHT_MID, -(PAD + 18), 8);

    // ── Vertical divider ──────────────────────────────────────────────────────
    lv_obj_t *div = lv_obj_create(scr);
    lv_obj_set_size(div, 1, SCR_H - HEADER_H);
    lv_obj_set_pos (div, LP_W, HEADER_H);
    lv_obj_set_style_bg_color    (div, C_BORDER, 0);
    lv_obj_set_style_bg_opa      (div, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(div, 0, 0);
    lv_obj_set_style_radius      (div, 0, 0);

    // ── Volume arc (left panel) ───────────────────────────────────────────────
    _arc = lv_arc_create(scr);
    lv_obj_set_size(_arc, ARC_SIZE, ARC_SIZE);
    lv_obj_set_pos (_arc, ARC_X, ARC_Y);

    lv_obj_set_style_arc_color(_arc, C_SURFACE, LV_PART_MAIN);
    lv_obj_set_style_arc_width(_arc, ARC_TRACK_W, LV_PART_MAIN);
    lv_obj_set_style_arc_color(_arc, C_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(_arc, ARC_TRACK_W, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa   (_arc, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_pad_all  (_arc, 4, LV_PART_KNOB);
    lv_obj_set_style_bg_opa   (_arc, LV_OPA_TRANSP, LV_PART_MAIN);

    lv_arc_set_bg_angles(_arc, 135, 45);
    lv_arc_set_range    (_arc, 0, 99);
    lv_arc_set_value    (_arc, 50);
    lv_obj_add_event_cb (_arc, _on_arc, LV_EVENT_VALUE_CHANGED, NULL);

    _vol_num = lv_label_create(_arc);
    lv_label_set_text(_vol_num, "50");
    lv_obj_set_style_text_font (_vol_num, &lv_font_montserrat_digits_64, 0);
    lv_obj_set_style_text_color(_vol_num, C_TEXT, 0);
    lv_obj_align(_vol_num, LV_ALIGN_CENTER, 0, -14);

    _vol_db = lv_label_create(_arc);
    lv_label_set_text(_vol_db, "-49 dB");
    lv_obj_set_style_text_font (_vol_db, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(_vol_db, C_TEXT_DIM, 0);
    lv_obj_align(_vol_db, LV_ALIGN_CENTER, 0, 28);

    // ── Mute button (left panel, below arc) ───────────────────────────────────
    _mute_btn = lv_btn_create(scr);
    // Remove default btn styles BEFORE set_pos — in LVGL 8.3 position is stored
    // as LV_STYLE_X/Y local props, so remove_style_all() after set_pos wipes them.
    lv_obj_remove_style_all(_mute_btn);
    lv_obj_add_style(_mute_btn, &_sty_mute_off, LV_STATE_DEFAULT);
    lv_obj_add_style(_mute_btn, &_sty_mute_on,  LV_STATE_CHECKED);
    lv_obj_set_size(_mute_btn, MUTE_W, MUTE_H);
    lv_obj_set_pos (_mute_btn, MUTE_X, MUTE_Y);
    lv_obj_add_event_cb(_mute_btn, _on_mute, LV_EVENT_CLICKED, NULL);
    _mute_lbl = lv_label_create(_mute_btn);
    lv_label_set_text(_mute_lbl, LV_SYMBOL_MUTE "  MUTE");
    lv_obj_center(_mute_lbl);

    // ── INPUT cycle button (right panel) ─────────────────────────────────────
    lv_obj_t *inp_lbl = lv_label_create(scr);
    lv_label_set_text(inp_lbl, "INPUT");
    lv_obj_add_style(inp_lbl, &_sty_lbl, 0);
    lv_obj_set_pos(inp_lbl, RP_INNER_X, INP_LBL_Y);

    _inp_btn = lv_btn_create(scr);
    lv_obj_remove_style_all(_inp_btn);
    lv_obj_add_style(_inp_btn, &_sty_cycle, 0);
    lv_obj_set_size(_inp_btn, BTN_W, ROW_H);
    lv_obj_set_pos (_inp_btn, RP_INNER_X, INP_BTN_Y);
    lv_obj_add_event_cb(_inp_btn, _on_inp_cycle, LV_EVENT_CLICKED, NULL);
    _inp_btn_lbl = lv_label_create(_inp_btn);
    lv_label_set_text(_inp_btn_lbl, _inp_names[0]);
    lv_obj_center(_inp_btn_lbl);

    // ── FILTER cycle button (right panel) ────────────────────────────────────
    lv_obj_t *flt_lbl = lv_label_create(scr);
    lv_label_set_text(flt_lbl, "FILTER");
    lv_obj_add_style(flt_lbl, &_sty_lbl, 0);
    lv_obj_set_pos(flt_lbl, RP_INNER_X, FLT_LBL_Y);

    _flt_btn = lv_btn_create(scr);
    lv_obj_remove_style_all(_flt_btn);
    lv_obj_add_style(_flt_btn, &_sty_cycle, 0);
    lv_obj_set_size(_flt_btn, BTN_W, ROW_H);
    lv_obj_set_pos (_flt_btn, RP_INNER_X, FLT_BTN_Y);
    lv_obj_add_event_cb(_flt_btn, _on_flt_cycle, LV_EVENT_CLICKED, NULL);
    _flt_btn_lbl = lv_label_create(_flt_btn);
    lv_label_set_text(_flt_btn_lbl, _flt_names[0]);
    lv_obj_center(_flt_btn_lbl);

    // ── STYLE cycle button (right panel) ─────────────────────────────────────
    lv_obj_t *sty_lbl = lv_label_create(scr);
    lv_label_set_text(sty_lbl, "STYLE");
    lv_obj_add_style(sty_lbl, &_sty_lbl, 0);
    lv_obj_set_pos(sty_lbl, RP_INNER_X, STY_LBL_Y);

    _sty_btn = lv_btn_create(scr);
    lv_obj_remove_style_all(_sty_btn);
    lv_obj_add_style(_sty_btn, &_sty_cycle, 0);
    lv_obj_set_size(_sty_btn, BTN_W, ROW_H);
    lv_obj_set_pos (_sty_btn, RP_INNER_X, STY_BTN_Y);
    lv_obj_add_event_cb(_sty_btn, _on_sty_cycle, LV_EVENT_CLICKED, NULL);
    _sty_btn_lbl = lv_label_create(_sty_btn);
    lv_label_set_text(_sty_btn_lbl, _theme_names[0]);
    lv_obj_center(_sty_btn_lbl);

    // ── Brightness slider (far-right column) ─────────────────────────────────
    _bright_slider = lv_slider_create(scr);
    lv_slider_set_range(_bright_slider, BRIGHTNESS_MIN, BRIGHTNESS_MAX);
    lv_obj_set_size(_bright_slider, BRIGHT_SLIDER_W, BRIGHT_SLIDER_H);
    lv_obj_set_pos (_bright_slider, BRIGHT_SLIDER_X, BRIGHT_SLIDER_Y);

    lv_obj_set_style_bg_color    (_bright_slider, C_SURFACE, LV_PART_MAIN);
    lv_obj_set_style_bg_opa      (_bright_slider, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(_bright_slider, C_BORDER, LV_PART_MAIN);
    lv_obj_set_style_border_width(_bright_slider, 1, LV_PART_MAIN);
    lv_obj_set_style_radius      (_bright_slider, 4, LV_PART_MAIN);

    lv_obj_set_style_bg_color(_bright_slider, C_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa  (_bright_slider, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius  (_bright_slider, 4, LV_PART_INDICATOR);

    lv_obj_set_style_bg_opa  (_bright_slider, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_pad_all (_bright_slider, 0, LV_PART_KNOB);

    lv_slider_set_value(_bright_slider, BRIGHTNESS_DEFAULT, LV_ANIM_OFF);
    lv_obj_add_event_cb(_bright_slider, _on_brightness, LV_EVENT_VALUE_CHANGED, NULL);

    // "BRT" label above slider
    lv_obj_t *brt_icon = lv_label_create(scr);
    lv_label_set_text(brt_icon, "BRT");
    lv_obj_set_style_text_color(brt_icon, C_TEXT_DIM, 0);
    lv_obj_set_style_text_font (brt_icon, &lv_font_montserrat_16, 0);
    lv_obj_align_to(brt_icon, _bright_slider, LV_ALIGN_OUT_TOP_MID, 0, -6);

    // Percentage label below slider
    _bright_pct_lbl = lv_label_create(scr);
    lv_label_set_text(_bright_pct_lbl, "80%");
    lv_obj_set_style_text_color(_bright_pct_lbl, C_TEXT_DIM, 0);
    lv_obj_set_style_text_font (_bright_pct_lbl, &lv_font_montserrat_16, 0);
    lv_obj_align_to(_bright_pct_lbl, _bright_slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 6);

    _apply_theme(0);   // initialise with RIVER
}

// ─── Public API ───────────────────────────────────────────────────────────────
void dam_ui_init(dam_action_cb_t cb)
{
    _cb = cb;
    _init_styles();
    _build();
}

void dam_ui_set_volume(int vol, bool muted)
{
    if (!_arc) return;

    _prog_set = true;
    lv_arc_set_value(_arc, vol);
    _prog_set = false;

    char buf[16];
    snprintf(buf, sizeof(buf), "%02d", vol);
    lv_label_set_text(_vol_num, buf);

    char db_buf[16];
    snprintf(db_buf, sizeof(db_buf), "%d dB", vol - 99);
    lv_label_set_text(_vol_db, db_buf);

    if (_mute_btn) {
        if (muted) {
            lv_obj_add_state(_mute_btn, LV_STATE_CHECKED);
            lv_label_set_text(_mute_lbl, LV_SYMBOL_MUTE "  MUTED");
        } else {
            lv_obj_clear_state(_mute_btn, LV_STATE_CHECKED);
            lv_label_set_text(_mute_lbl, LV_SYMBOL_MUTE "  MUTE");
        }
    }
}

void dam_ui_set_input(dac_input_t input)
{
    if (!_inp_btn_lbl) return;
    lv_label_set_text(_inp_btn_lbl, _inp_names[(int)input]);
}

void dam_ui_set_filter(dac_filter_t filter)
{
    if (!_flt_btn_lbl) return;
    lv_label_set_text(_flt_btn_lbl, _flt_names[(int)filter]);
}

void dam_ui_set_brightness(int brightness)
{
    if (!_bright_slider) return;
    _prog_set = true;
    lv_slider_set_value(_bright_slider, brightness, LV_ANIM_OFF);
    _prog_set = false;
    if (_bright_pct_lbl) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%d%%", brightness);
        lv_label_set_text(_bright_pct_lbl, buf);
    }
}

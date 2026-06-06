#include "dam_ui.h"
#include <stdio.h>
#include <stdint.h>

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
//  Y=0   ┌────────────────────────────────────────────────────────────────┐
//        │  DAM 1021                                                v1.0  │ HEADER 50px
//  Y=50  ├──────────────────────┬─────────────────────────────────────────┤
//        │   ╭──────────────╮   │                                          │
//        │  /   50   -49 dB  \  │  INPUT                                   │
//        │  \                /  │  [◄]  [    AUTO    ]  [►]               │ row 70px
//        │   ╰──────────────╯   │                                          │
//        │   [ MUTE / MUTED ]   │  FILTER                                  │
//        │                      │  [◄]  [    LIN     ]  [►]               │ row 70px
//  Y=480 └──────────────────────┴─────────────────────────────────────────┘
//        x=0        x=260      x=261                               x=639

#define SCR_W        640
#define SCR_H        480
#define PAD           10
#define BTN_GAP        3
#define LBL_H         20

#define HEADER_H      50

// ── Left panel – arc + mute ───────────────────────────────────────────────────
#define LP_W         260
#define ARC_SIZE     220    // slightly smaller to leave room for mute below
#define ARC_TRACK_W   18
#define MUTE_H        60
#define MUTE_W        ARC_SIZE
#define ARC_MUTE_GAP  10

// Vertically center [arc + gap + mute] in left panel
#define LP_CONTENT_H  (ARC_SIZE + ARC_MUTE_GAP + MUTE_H)
#define LP_TOP_PAD    ((SCR_H - HEADER_H - LP_CONTENT_H) / 2)
#define ARC_X         ((LP_W - ARC_SIZE) / 2)
#define ARC_Y         (HEADER_H + LP_TOP_PAD)
#define MUTE_X        ARC_X
#define MUTE_Y        (ARC_Y + ARC_SIZE + ARC_MUTE_GAP)

// ── Right panel – input + filter rows ─────────────────────────────────────────
#define RP_X         (LP_W + 1)
#define RP_W         (SCR_W - RP_X)
#define RP_INNER_X   (RP_X + PAD)
#define RP_INNER_W   (RP_W - 2 * PAD)

// Nav row geometry: [◄ NAV_W] [gap] [CUR_W] [gap] [► NAV_W]
// 44 + 3 + 265 + 3 + 44 = 359 = RP_INNER_W ✓
#define NAV_W         44
#define CUR_W         (RP_INNER_W - 2 * NAV_W - 2 * BTN_GAP)  // = 265
#define ROW_H         70

// Vertically center content in right panel
// Content: LBL_H + 6 + ROW_H + 30 + LBL_H + 6 + ROW_H = 222px
#define RP_SECT_GAP   30
#define RP_LBL_GAP     6
#define RP_CONTENT_H  (2 * (LBL_H + RP_LBL_GAP + ROW_H) + RP_SECT_GAP)
#define RP_TOP_PAD    ((SCR_H - HEADER_H - RP_CONTENT_H) / 2)

#define INP_LBL_Y    (HEADER_H + RP_TOP_PAD)
#define INP_BTN_Y    (INP_LBL_Y + LBL_H + RP_LBL_GAP)
#define FLT_LBL_Y    (INP_BTN_Y + ROW_H + RP_SECT_GAP)
#define FLT_BTN_Y    (FLT_LBL_Y + LBL_H + RP_LBL_GAP)

// ─── Names ───────────────────────────────────────────────────────────────────
static const char * const _inp_names[DAC_INPUT_COUNT]  = { "AUTO", "USB", "SPDIF", "OPT" };
static const char * const _flt_names[DAC_FILTER_COUNT] = { "LIN",  "MIX", "MIN",   "SOFT" };

// ─── Styles ───────────────────────────────────────────────────────────────────
static lv_style_t _sty_scr;
static lv_style_t _sty_lbl;
static lv_style_t _sty_nav;
static lv_style_t _sty_cur;       // current-selection display box
static lv_style_t _sty_mute_off;
static lv_style_t _sty_mute_on;

// ─── Widgets ──────────────────────────────────────────────────────────────────
static lv_obj_t *_arc         = NULL;
static lv_obj_t *_vol_num     = NULL;
static lv_obj_t *_vol_db      = NULL;
static lv_obj_t *_inp_cur_lbl = NULL;  // current input name
static lv_obj_t *_flt_cur_lbl = NULL;  // current filter name
static lv_obj_t *_mute_btn    = NULL;
static lv_obj_t *_mute_lbl    = NULL;

static dam_action_cb_t _cb = NULL;
static bool _prog_set = false;

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
    lv_style_set_text_font (&_sty_lbl, &lv_font_montserrat_16);

    // Navigation ◄/► buttons
    lv_style_init(&_sty_nav);
    lv_style_set_bg_color    (&_sty_nav, C_SURFACE);
    lv_style_set_bg_opa      (&_sty_nav, LV_OPA_COVER);
    lv_style_set_border_color(&_sty_nav, C_ACCENT);
    lv_style_set_border_width(&_sty_nav, 2);
    lv_style_set_radius      (&_sty_nav, 8);
    lv_style_set_text_color  (&_sty_nav, C_ACCENT);
    lv_style_set_text_font   (&_sty_nav, &lv_font_montserrat_24);

    // Current-selection display box (non-interactive)
    lv_style_init(&_sty_cur);
    lv_style_set_bg_color    (&_sty_cur, C_SURFACE);
    lv_style_set_bg_opa      (&_sty_cur, LV_OPA_COVER);
    lv_style_set_border_color(&_sty_cur, C_ACCENT);
    lv_style_set_border_width(&_sty_cur, 2);
    lv_style_set_radius      (&_sty_cur, 12);
    lv_style_set_pad_all     (&_sty_cur, 0);

    // Mute button – not muted
    lv_style_init(&_sty_mute_off);
    lv_style_set_bg_color    (&_sty_mute_off, C_SURFACE);
    lv_style_set_bg_opa      (&_sty_mute_off, LV_OPA_COVER);
    lv_style_set_border_color(&_sty_mute_off, C_BORDER);
    lv_style_set_border_width(&_sty_mute_off, 2);
    lv_style_set_radius      (&_sty_mute_off, 12);
    lv_style_set_text_color  (&_sty_mute_off, C_TEXT_DIM);
    lv_style_set_text_font   (&_sty_mute_off, &lv_font_montserrat_20);

    // Mute button – muted (red)
    lv_style_init(&_sty_mute_on);
    lv_style_set_bg_color    (&_sty_mute_on, C_MUTE_ON);
    lv_style_set_bg_opa      (&_sty_mute_on, LV_OPA_COVER);
    lv_style_set_border_width(&_sty_mute_on, 0);
    lv_style_set_radius      (&_sty_mute_on, 12);
    lv_style_set_text_color  (&_sty_mute_on, C_TEXT);
    lv_style_set_text_font   (&_sty_mute_on, &lv_font_montserrat_20);
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

static void _on_input_nav(lv_event_t *e)
{
    int dir = (int)(intptr_t)lv_obj_get_user_data(lv_event_get_target(e));
    if (_cb) _cb(dir < 0 ? ACT_CHANNEL_LEFT : ACT_CHANNEL_RIGHT, 0);
}

static void _on_filter_nav(lv_event_t *e)
{
    int dir = (int)(intptr_t)lv_obj_get_user_data(lv_event_get_target(e));
    if (_cb) _cb(dir < 0 ? ACT_FILTER_BACK : ACT_FILTER_CYCLE, 0);
}

// ─── Helper: build one nav row (◄ / display box / ►) ─────────────────────────
static lv_obj_t *_build_nav_row(int y,
                                 lv_event_cb_t nav_cb,
                                 const char *init_text)
{
    // ◄ button
    lv_obj_t *prev = lv_btn_create(lv_scr_act());
    lv_obj_add_style(prev, &_sty_nav, 0);
    lv_obj_set_size(prev, NAV_W, ROW_H);
    lv_obj_set_pos (prev, RP_INNER_X, y);
    lv_obj_add_event_cb(prev, nav_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_user_data(prev, (void *)(intptr_t)(-1));
    lv_obj_t *l1 = lv_label_create(prev);
    lv_label_set_text(l1, LV_SYMBOL_LEFT);
    lv_obj_center(l1);

    // Current-selection display
    lv_obj_t *box = lv_obj_create(lv_scr_act());
    lv_obj_add_style(box, &_sty_cur, 0);
    lv_obj_set_size(box, CUR_W, ROW_H);
    lv_obj_set_pos (box, RP_INNER_X + NAV_W + BTN_GAP, y);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *cur = lv_label_create(box);
    lv_label_set_text(cur, init_text);
    lv_obj_set_style_text_font (cur, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(cur, C_ACCENT, 0);
    lv_obj_center(cur);

    // ► button
    lv_obj_t *next = lv_btn_create(lv_scr_act());
    lv_obj_add_style(next, &_sty_nav, 0);
    lv_obj_set_size(next, NAV_W, ROW_H);
    lv_obj_set_pos (next, RP_INNER_X + NAV_W + BTN_GAP + CUR_W + BTN_GAP, y);
    lv_obj_add_event_cb(next, nav_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_user_data(next, (void *)(intptr_t)(+1));
    lv_obj_t *l2 = lv_label_create(next);
    lv_label_set_text(l2, LV_SYMBOL_RIGHT);
    lv_obj_center(l2);

    return cur;   // caller saves the label to update it later
}

// ─── Layout builder ───────────────────────────────────────────────────────────
static void _build(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_add_style(scr, &_sty_scr, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    // ── Header (full width) ───────────────────────────────────────────────────
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
    lv_obj_align(title, LV_ALIGN_LEFT_MID, PAD, 0);

    lv_obj_t *ver = lv_label_create(hdr);
    lv_label_set_text(ver, "v1.0");
    lv_obj_set_style_text_color(ver, C_BORDER, 0);
    lv_obj_set_style_text_font (ver, &lv_font_montserrat_16, 0);
    lv_obj_align(ver, LV_ALIGN_RIGHT_MID, -PAD, 0);

    // ── Vertical divider ──────────────────────────────────────────────────────
    lv_obj_t *div = lv_obj_create(scr);
    lv_obj_set_size(div, 1, SCR_H - HEADER_H);
    lv_obj_set_pos (div, LP_W, HEADER_H);
    lv_obj_set_style_bg_color    (div, C_BORDER, 0);
    lv_obj_set_style_bg_opa      (div, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(div, 0, 0);
    lv_obj_set_style_radius      (div, 0, 0);

    // ── Volume Arc (left panel) ────────────────────────────────────────────────
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
    lv_obj_set_style_text_font (_vol_num, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(_vol_num, C_TEXT, 0);
    lv_obj_align(_vol_num, LV_ALIGN_CENTER, 0, -14);

    _vol_db = lv_label_create(_arc);
    lv_label_set_text(_vol_db, "-49 dB");
    lv_obj_set_style_text_font (_vol_db, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(_vol_db, C_TEXT_DIM, 0);
    lv_obj_align(_vol_db, LV_ALIGN_CENTER, 0, 28);

    // ── Mute button (left panel, below arc) ───────────────────────────────────
    _mute_btn = lv_btn_create(scr);
    // Remove default btn theme styles BEFORE set_pos: in LVGL 8.3 position is stored
    // as LV_STYLE_X/Y local props, so remove_style_all() after set_pos would wipe them.
    lv_obj_remove_style_all(_mute_btn);
    lv_obj_add_style(_mute_btn, &_sty_mute_off, LV_STATE_DEFAULT);
    lv_obj_add_style(_mute_btn, &_sty_mute_on,  LV_STATE_CHECKED);
    lv_obj_set_size(_mute_btn, MUTE_W, MUTE_H);
    lv_obj_set_pos (_mute_btn, MUTE_X, MUTE_Y);
    lv_obj_add_event_cb(_mute_btn, _on_mute, LV_EVENT_CLICKED, NULL);
    _mute_lbl = lv_label_create(_mute_btn);
    lv_label_set_text(_mute_lbl, LV_SYMBOL_MUTE "  MUTE");
    lv_obj_center(_mute_lbl);

    // ── INPUT row (right panel) ────────────────────────────────────────────────
    lv_obj_t *inp_lbl = lv_label_create(scr);
    lv_label_set_text(inp_lbl, "INPUT");
    lv_obj_add_style(inp_lbl, &_sty_lbl, 0);
    lv_obj_set_pos(inp_lbl, RP_INNER_X, INP_LBL_Y);

    _inp_cur_lbl = _build_nav_row(INP_BTN_Y, _on_input_nav, _inp_names[0]);

    // ── FILTER row (right panel) ───────────────────────────────────────────────
    lv_obj_t *flt_lbl = lv_label_create(scr);
    lv_label_set_text(flt_lbl, "FILTER");
    lv_obj_add_style(flt_lbl, &_sty_lbl, 0);
    lv_obj_set_pos(flt_lbl, RP_INNER_X, FLT_LBL_Y);

    _flt_cur_lbl = _build_nav_row(FLT_BTN_Y, _on_filter_nav, _flt_names[0]);
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
    if (!_inp_cur_lbl) return;
    lv_label_set_text(_inp_cur_lbl, _inp_names[(int)input]);
}

void dam_ui_set_filter(dac_filter_t filter)
{
    if (!_flt_cur_lbl) return;
    lv_label_set_text(_flt_cur_lbl, _flt_names[(int)filter]);
}

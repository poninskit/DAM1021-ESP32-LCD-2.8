#include "dam_ui.h"
#include <stdio.h>
#include <stdint.h>

// ─── Colour palette ──────────────────────────────────────────────────────────
#define C_BG           lv_color_hex(0x0D1117)
#define C_HEADER       lv_color_hex(0x161B22)
#define C_SURFACE      lv_color_hex(0x21262D)
#define C_BORDER       lv_color_hex(0x30363D)
#define C_ACCENT       lv_color_hex(0x58A6FF)
#define C_MUTE_ACTIVE  lv_color_hex(0xF85149)
#define C_TEXT         lv_color_hex(0xE6EDF3)
#define C_TEXT_DIM     lv_color_hex(0x7D8590)

// ─── Layout — 480 × 640 portrait ─────────────────────────────────────────────
#define SCR_W    480
#define SCR_H    640
#define PAD       12
#define BTN_GAP    8
// 4 equal-width buttons across the screen
#define BTN4_W   ((SCR_W - 2*PAD - 3*BTN_GAP) / 4)   // = 108 px

//  Section           Y    H
#define HEADER_Y      0
#define HEADER_H     80
#define VOL_NUM_Y    80
#define VOL_NUM_H   180
#define VOL_BTN_Y   260
#define VOL_BTN_H   100
#define VOL_BTN_W   200
#define DIV1_Y      360
#define INP_LBL_Y   364
#define INP_BTN_Y   406
#define INP_BTN_H   100
#define DIV2_Y      506
#define FLT_LBL_Y   510
#define FLT_BTN_Y   552
#define FLT_BTN_H    52
#define DIV3_Y      604
#define MUTE_Y      607
#define MUTE_H       33

#define RADIUS_SM   12
#define RADIUS_LG   16

// ─── Styles ───────────────────────────────────────────────────────────────────
static lv_style_t _style_scr;
static lv_style_t _style_section_lbl;
static lv_style_t _style_btn_inactive;
static lv_style_t _style_btn_active;
static lv_style_t _style_vol_btn;
static lv_style_t _style_mute_inactive;
static lv_style_t _style_mute_active;

// ─── Widgets ──────────────────────────────────────────────────────────────────
static lv_obj_t *_vol_label       = NULL;
static lv_obj_t *_mute_btn        = NULL;
static lv_obj_t *_mute_lbl        = NULL;
static lv_obj_t *_input_btns[4];
static lv_obj_t *_filter_btns[4];

static dam_action_cb_t _action_cb = NULL;

// ─── Event callbacks ──────────────────────────────────────────────────────────
static void _on_vol_down(lv_event_t *e)
{
    (void)e;
    if (_action_cb) _action_cb(ACT_VOL_DOWN);
}
static void _on_vol_up(lv_event_t *e)
{
    (void)e;
    if (_action_cb) _action_cb(ACT_VOL_UP);
}
static void _on_mute(lv_event_t *e)
{
    (void)e;
    if (_action_cb) _action_cb(ACT_MUTE);
}
static void _on_input(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_obj_get_user_data(lv_event_get_target(e));
    static const dam_action_t acts[4] = {
        ACT_INPUT_AUTO, ACT_INPUT_USB, ACT_INPUT_SPDIF, ACT_INPUT_OPT
    };
    if (_action_cb) _action_cb(acts[idx]);
}
static void _on_filter(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_obj_get_user_data(lv_event_get_target(e));
    static const dam_action_t acts[4] = {
        ACT_FILTER_LINEAR, ACT_FILTER_MIXED, ACT_FILTER_MINIMUM, ACT_FILTER_SOFT
    };
    if (_action_cb) _action_cb(acts[idx]);
}

// ─── Style init ───────────────────────────────────────────────────────────────
static void _init_styles(void)
{
    lv_style_init(&_style_scr);
    lv_style_set_bg_color    (&_style_scr, C_BG);
    lv_style_set_bg_opa      (&_style_scr, LV_OPA_COVER);
    lv_style_set_border_width(&_style_scr, 0);
    lv_style_set_pad_all     (&_style_scr, 0);

    lv_style_init(&_style_section_lbl);
    lv_style_set_text_color(&_style_section_lbl, C_TEXT_DIM);
    lv_style_set_text_font (&_style_section_lbl, &lv_font_montserrat_24);

    lv_style_init(&_style_btn_inactive);
    lv_style_set_bg_color    (&_style_btn_inactive, C_SURFACE);
    lv_style_set_bg_opa      (&_style_btn_inactive, LV_OPA_COVER);
    lv_style_set_border_color(&_style_btn_inactive, C_BORDER);
    lv_style_set_border_width(&_style_btn_inactive, 2);
    lv_style_set_radius      (&_style_btn_inactive, RADIUS_SM);
    lv_style_set_text_color  (&_style_btn_inactive, C_TEXT_DIM);
    lv_style_set_text_font   (&_style_btn_inactive, &lv_font_montserrat_24);
    lv_style_set_pad_all     (&_style_btn_inactive, 4);

    lv_style_init(&_style_btn_active);
    lv_style_set_bg_color    (&_style_btn_active, C_ACCENT);
    lv_style_set_bg_opa      (&_style_btn_active, LV_OPA_COVER);
    lv_style_set_border_width(&_style_btn_active, 0);
    lv_style_set_radius      (&_style_btn_active, RADIUS_SM);
    lv_style_set_text_color  (&_style_btn_active, lv_color_hex(0x0D1117));
    lv_style_set_text_font   (&_style_btn_active, &lv_font_montserrat_24);
    lv_style_set_pad_all     (&_style_btn_active, 4);

    lv_style_init(&_style_vol_btn);
    lv_style_set_bg_color    (&_style_vol_btn, C_ACCENT);
    lv_style_set_bg_opa      (&_style_vol_btn, LV_OPA_COVER);
    lv_style_set_border_width(&_style_vol_btn, 0);
    lv_style_set_radius      (&_style_vol_btn, RADIUS_LG);
    lv_style_set_text_color  (&_style_vol_btn, lv_color_hex(0x0D1117));
    lv_style_set_text_font   (&_style_vol_btn, &lv_font_montserrat_48);
    lv_style_set_pad_all     (&_style_vol_btn, 0);

    lv_style_init(&_style_mute_inactive);
    lv_style_set_bg_color    (&_style_mute_inactive, C_SURFACE);
    lv_style_set_bg_opa      (&_style_mute_inactive, LV_OPA_COVER);
    lv_style_set_border_color(&_style_mute_inactive, C_BORDER);
    lv_style_set_border_width(&_style_mute_inactive, 2);
    lv_style_set_radius      (&_style_mute_inactive, RADIUS_SM);
    lv_style_set_text_color  (&_style_mute_inactive, C_TEXT_DIM);
    lv_style_set_text_font   (&_style_mute_inactive, &lv_font_montserrat_24);
    lv_style_set_pad_all     (&_style_mute_inactive, 0);

    lv_style_init(&_style_mute_active);
    lv_style_set_bg_color    (&_style_mute_active, C_MUTE_ACTIVE);
    lv_style_set_bg_opa      (&_style_mute_active, LV_OPA_COVER);
    lv_style_set_border_width(&_style_mute_active, 0);
    lv_style_set_radius      (&_style_mute_active, RADIUS_SM);
    lv_style_set_text_color  (&_style_mute_active, C_TEXT);
    lv_style_set_text_font   (&_style_mute_active, &lv_font_montserrat_24);
    lv_style_set_pad_all     (&_style_mute_active, 0);
}

// ─── Helper: thin divider line ────────────────────────────────────────────────
static void _make_divider(lv_obj_t *scr, int y)
{
    lv_obj_t *d = lv_obj_create(scr);
    lv_obj_set_size(d, SCR_W - 2*PAD, 2);
    lv_obj_set_pos (d, PAD, y);
    lv_obj_set_style_bg_color    (d, C_BORDER, 0);
    lv_obj_set_style_bg_opa      (d, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(d, 0, 0);
    lv_obj_set_style_radius      (d, 0, 0);
    lv_obj_set_style_pad_all     (d, 0, 0);
    lv_obj_clear_flag(d, LV_OBJ_FLAG_SCROLLABLE);
}

// ─── Layout builder ───────────────────────────────────────────────────────────
static void _build_layout(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_add_style(scr, &_style_scr, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    // ── HEADER ────────────────────────────────────────────────────────────────
    lv_obj_t *hdr = lv_obj_create(scr);
    lv_obj_set_size(hdr, SCR_W, HEADER_H);
    lv_obj_set_pos (hdr, 0, HEADER_Y);
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

    // ── VOLUME NUMBER ─────────────────────────────────────────────────────────
    _vol_label = lv_label_create(scr);
    lv_label_set_text(_vol_label, "-49 dB");
    lv_obj_set_style_text_color(_vol_label, C_TEXT, 0);
    lv_obj_set_style_text_font (_vol_label, &lv_font_montserrat_48, 0);
    lv_obj_set_size(_vol_label, SCR_W, VOL_NUM_H);
    lv_obj_set_pos (_vol_label, 0, VOL_NUM_Y);
    lv_obj_set_style_text_align(_vol_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_top   (_vol_label, 60, 0);  // visually center in 180px
    lv_obj_set_style_bg_opa    (_vol_label, LV_OPA_TRANSP, 0);

    // ── VOLUME ± BUTTONS ──────────────────────────────────────────────────────
    lv_obj_t *vol_down = lv_btn_create(scr);
    lv_obj_add_style(vol_down, &_style_vol_btn, 0);
    lv_obj_set_size(vol_down, VOL_BTN_W, VOL_BTN_H - 8);
    lv_obj_set_pos (vol_down, PAD, VOL_BTN_Y + 4);
    lv_obj_add_event_cb(vol_down, _on_vol_down, LV_EVENT_CLICKED, NULL);
    lv_obj_t *vd_lbl = lv_label_create(vol_down);
    lv_label_set_text(vd_lbl, LV_SYMBOL_MINUS);
    lv_obj_set_style_text_font(vd_lbl, &lv_font_montserrat_48, 0);
    lv_obj_center(vd_lbl);

    lv_obj_t *vol_up = lv_btn_create(scr);
    lv_obj_add_style(vol_up, &_style_vol_btn, 0);
    lv_obj_set_size(vol_up, VOL_BTN_W, VOL_BTN_H - 8);
    lv_obj_set_pos (vol_up, SCR_W - PAD - VOL_BTN_W, VOL_BTN_Y + 4);
    lv_obj_add_event_cb(vol_up, _on_vol_up, LV_EVENT_CLICKED, NULL);
    lv_obj_t *vu_lbl = lv_label_create(vol_up);
    lv_label_set_text(vu_lbl, LV_SYMBOL_PLUS);
    lv_obj_set_style_text_font(vu_lbl, &lv_font_montserrat_48, 0);
    lv_obj_center(vu_lbl);

    _make_divider(scr, DIV1_Y);

    // ── INPUT SECTION ─────────────────────────────────────────────────────────
    lv_obj_t *inp_lbl = lv_label_create(scr);
    lv_label_set_text(inp_lbl, "INPUT");
    lv_obj_add_style(inp_lbl, &_style_section_lbl, 0);
    lv_obj_set_pos(inp_lbl, PAD, INP_LBL_Y + 4);

    static const char *inp_names[4] = { "AUTO", "USB", "SPDF", "OPT" };
    for (int i = 0; i < 4; i++) {
        _input_btns[i] = lv_btn_create(scr);
        lv_obj_add_style(_input_btns[i], &_style_btn_inactive, 0);
        lv_obj_set_size(_input_btns[i], BTN4_W, INP_BTN_H - 10);
        lv_obj_set_pos (_input_btns[i], PAD + i*(BTN4_W + BTN_GAP), INP_BTN_Y + 5);
        lv_obj_add_event_cb(_input_btns[i], _on_input, LV_EVENT_CLICKED, NULL);
        lv_obj_set_user_data(_input_btns[i], (void*)(intptr_t)i);
        lv_obj_t *lbl = lv_label_create(_input_btns[i]);
        lv_label_set_text(lbl, inp_names[i]);
        lv_obj_center(lbl);
    }

    _make_divider(scr, DIV2_Y);

    // ── FILTER SECTION ────────────────────────────────────────────────────────
    lv_obj_t *flt_lbl = lv_label_create(scr);
    lv_label_set_text(flt_lbl, "FILTER");
    lv_obj_add_style(flt_lbl, &_style_section_lbl, 0);
    lv_obj_set_pos(flt_lbl, PAD, FLT_LBL_Y + 4);

    static const char *flt_names[4] = { "LIN", "MIX", "MIN", "SOFT" };
    for (int i = 0; i < 4; i++) {
        _filter_btns[i] = lv_btn_create(scr);
        lv_obj_add_style(_filter_btns[i], &_style_btn_inactive, 0);
        lv_obj_set_size(_filter_btns[i], BTN4_W, FLT_BTN_H - 8);
        lv_obj_set_pos (_filter_btns[i], PAD + i*(BTN4_W + BTN_GAP), FLT_BTN_Y + 4);
        lv_obj_add_event_cb(_filter_btns[i], _on_filter, LV_EVENT_CLICKED, NULL);
        lv_obj_set_user_data(_filter_btns[i], (void*)(intptr_t)i);
        lv_obj_t *lbl = lv_label_create(_filter_btns[i]);
        lv_label_set_text(lbl, flt_names[i]);
        lv_obj_center(lbl);
    }

    _make_divider(scr, DIV3_Y);

    // ── MUTE BUTTON ───────────────────────────────────────────────────────────
    _mute_btn = lv_btn_create(scr);
    lv_obj_add_style(_mute_btn, &_style_mute_inactive, 0);
    lv_obj_set_size(_mute_btn, SCR_W - 2*PAD, MUTE_H);
    lv_obj_set_pos (_mute_btn, PAD, MUTE_Y);
    lv_obj_add_event_cb(_mute_btn, _on_mute, LV_EVENT_CLICKED, NULL);
    _mute_lbl = lv_label_create(_mute_btn);
    lv_label_set_text(_mute_lbl, LV_SYMBOL_MUTE "  MUTE");
    lv_obj_center(_mute_lbl);
}

// ─── Public API ───────────────────────────────────────────────────────────────
void dam_ui_init(dam_action_cb_t cb)
{
    _action_cb = cb;
    _init_styles();
    _build_layout();
}

void dam_ui_set_volume(int vol, bool muted)
{
    if (!_vol_label) return;
    char buf[16];
    int dB = vol - 99;
    snprintf(buf, sizeof(buf), "%d dB", dB);
    lv_label_set_text(_vol_label, buf);

    if (_mute_btn) {
        lv_obj_remove_style_all(_mute_btn);
        if (muted) {
            lv_obj_add_style(_mute_btn, &_style_mute_active, 0);
            lv_label_set_text(_mute_lbl, LV_SYMBOL_MUTE "  MUTED");
        } else {
            lv_obj_add_style(_mute_btn, &_style_mute_inactive, 0);
            lv_label_set_text(_mute_lbl, LV_SYMBOL_MUTE "  MUTE");
        }
    }
}

void dam_ui_set_input(dac_input_t input)
{
    for (int i = 0; i < 4; i++) {
        if (!_input_btns[i]) continue;
        lv_obj_remove_style_all(_input_btns[i]);
        lv_obj_add_style(_input_btns[i],
            (i == (int)input) ? &_style_btn_active : &_style_btn_inactive, 0);
        lv_obj_invalidate(_input_btns[i]);
    }
}

void dam_ui_set_filter(dac_filter_t filter)
{
    for (int i = 0; i < 4; i++) {
        if (!_filter_btns[i]) continue;
        lv_obj_remove_style_all(_filter_btns[i]);
        lv_obj_add_style(_filter_btns[i],
            (i == (int)filter) ? &_style_btn_active : &_style_btn_inactive, 0);
        lv_obj_invalidate(_filter_btns[i]);
    }
}

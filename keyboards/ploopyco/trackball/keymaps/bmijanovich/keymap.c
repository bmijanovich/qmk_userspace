#include QMK_KEYBOARD_H

// Per-button TAPPING_TERM relative to thumb/finger strength
#define BTN2_TAPPING_TERM TAPPING_TERM
#define BTN4_TAPPING_TERM TAPPING_TERM + 75
#define BTN5_TAPPING_TERM TAPPING_TERM + 75

// CKC = Custom key code
// Mac utility function keycodes
#define CKC_MAC_DL     LCTL(KC_LEFT)        // Move leftward one space
#define CKC_MAC_DR     LCTL(KC_RGHT)        // Move rightward one space
#define CKC_MAC_SW     LCTL(KC_UP)          // Activate Mission Control
#define CKC_MAC_SD     KC_F11               // Show the desktop
#define CKC_MAC_SS     LSFT(LGUI(KC_4))     // Take a screenshot (selection)
#define CKC_MAC_CUT    LGUI(KC_X)           // Cut
#define CKC_MAC_COPY   LGUI(KC_C)           // Copy
#define CKC_MAC_PASTE  LGUI(KC_V)           // Paste

// Windows utility function keycodes
#define CKC_WIN_DL     LGUI(LCTL(KC_LEFT))  // Move leftward one desktop
#define CKC_WIN_DR     LGUI(LCTL(KC_RGHT))  // Move rightward one desktop
#define CKC_WIN_SW     LGUI(KC_TAB)         // Activate Task View
#define CKC_WIN_SD     LGUI(KC_D)           // Show the desktop
#define CKC_WIN_SS     LSFT(LGUI(KC_S))     // Take a screenshot (Snipping Tool)
#define CKC_WIN_CUT    LCTL(KC_X)           // Cut
#define CKC_WIN_COPY   LCTL(KC_C)           // Copy
#define CKC_WIN_PASTE  LCTL(KC_V)           // Paste

// Layers
enum {
    _BASE,
    _DRAG_LOCK,
    _DPI_CONTROL,
    _RAISE,
    _LOWER,
    _SYSTEM,
};

// Custom keycodes
enum {
    CKC_DRAG_LOCK_ON = SAFE_RANGE,
    CKC_DRAG_LOCK_OFF,
    CKC_TOGGLE_MAC_WINDOWS,
    CKC_SHOW_WINDOWS,
    CKC_SHOW_DESKTOP,
    CKC_SCREENSHOT,
    CKC_CUT,
    CKC_COPY,
    CKC_PASTE,
};

// Tap Dance keycodes
enum {
    TD_BTN2,  // 1: KC_BTN2; 2: KC_ENT; Hold: _DPI_CONTROL
    TD_BTN4,  // 1: KC_BTN4; 2: Desktop left; Hold: _RAISE + DRAG_SCROLL + horizontal wheel scroll
    TD_BTN5,  // 1: KC_BTN5; 2: Desktop right; Hold: _LOWER
};

// Keymap
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    // Base layer
    [_BASE] = LAYOUT(
        KC_BTN1, CKC_SHOW_WINDOWS, TD(TD_BTN2),
        TD(TD_BTN4), TD(TD_BTN5)
    ),
    // Special layer for drag lock state
    [_DRAG_LOCK] = LAYOUT(
        CKC_DRAG_LOCK_OFF, _______, _______,
        _______, _______
    ),
    // Cycle trackball DPI
    [_DPI_CONTROL] = LAYOUT(
        XXXXXXX, DPI_CONFIG, _______,
        XXXXXXX, XXXXXXX
    ),
    // Drag scroll, horizontal scroll with wheel, drag lock and utility functions
    [_RAISE] = LAYOUT(
        CKC_DRAG_LOCK_ON, CKC_SHOW_DESKTOP, CKC_CUT,
        _______, MO(_SYSTEM)
    ),
    // Utility functions
    [_LOWER] = LAYOUT(
        KC_BTN3, CKC_SCREENSHOT, CKC_COPY,
        CKC_PASTE, _______
    ),
    // Toggle between Mac and Windows modes, reset and bootloader
    [_SYSTEM] = LAYOUT(
        QK_RBT, QK_BOOT, CKC_TOGGLE_MAC_WINDOWS,
        _______, _______
    ),
};

// Mac / Windows mode
static bool mac = true;

// Tap Dance actions
typedef enum {
    TD_NONE,
    TD_UNKNOWN,
    TD_SINGLE_TAP,
    TD_DOUBLE_TAP,
    TD_SINGLE_HOLD,
} td_action_t;

// Track Tap Dance state separately for each Tap Dance key
static td_action_t btn2_td_action = TD_NONE;
static td_action_t btn4_td_action = TD_NONE;
static td_action_t btn5_td_action = TD_NONE;

// Fake keyrecord_t for hooking process_record_kb() with custom keycodes
static keyrecord_t fake_record;

// QMK userspace callback functions
bool process_record_user(uint16_t keycode, keyrecord_t *record);
uint16_t get_tapping_term(uint16_t keycode, keyrecord_t *record);
bool encoder_update_user(uint8_t index, bool clockwise);

// Tap Dance callback functions
static td_action_t get_tap_dance_action(tap_dance_state_t *state);
static void btn2_td_tap(tap_dance_state_t *state, void *user_data);
static void btn2_td_finished(tap_dance_state_t *state, void *user_data) ;
static void btn2_td_reset(tap_dance_state_t *state, void *user_data);
static void btn4_td_tap(tap_dance_state_t *state, void *user_data);
static void btn4_td_finished(tap_dance_state_t *state, void *user_data);
static void btn4_td_reset(tap_dance_state_t *state, void *user_data);
static void btn5_td_tap(tap_dance_state_t *state, void *user_data);
static void btn5_td_finished(tap_dance_state_t *state, void *user_data);
static void btn5_td_reset(tap_dance_state_t *state, void *user_data);

// Associate tap dance keys with their functionality
tap_dance_action_t tap_dance_actions[] = {
    [TD_BTN2] = ACTION_TAP_DANCE_FN_ADVANCED(btn2_td_tap, btn2_td_finished, btn2_td_reset),
    [TD_BTN4] = ACTION_TAP_DANCE_FN_ADVANCED(btn4_td_tap, btn4_td_finished, btn4_td_reset),
    [TD_BTN5] = ACTION_TAP_DANCE_FN_ADVANCED(btn5_td_tap, btn5_td_finished, btn5_td_reset),
};

// Functions for sending custom keycodes, since QMK functions can't register them
static void setup_fake_record(uint8_t col, uint8_t row, bool pressed);
static void register_custom_keycode(uint16_t keycode, uint8_t col, uint8_t row);
static void unregister_custom_keycode(uint16_t keycode, uint8_t col, uint8_t row);



/* QMK userspace callback functions */

// Handle custom keycodes
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case CKC_DRAG_LOCK_ON:  // Enable drag lock
            if (record->event.pressed) {
                register_code16(KC_BTN1);
                layer_on(_DRAG_LOCK);
            }
            return false;
        case CKC_DRAG_LOCK_OFF:  // Disable drag lock
            if (!record->event.pressed) {
                layer_off(_DRAG_LOCK);
                unregister_code16(KC_BTN1);
            }
            return false;
        case CKC_TOGGLE_MAC_WINDOWS:  // Toggle between Mac and Windows modes
            if (record->event.pressed) {
                mac ^= 1;
            }
            return false;
        case CKC_SHOW_WINDOWS:  // Show all windows
            if (record->event.pressed) {
                register_code16(mac ? CKC_MAC_SW : CKC_WIN_SW);
            } else {
                unregister_code16(mac ? CKC_MAC_SW : CKC_WIN_SW);
            }
            return false;
        case CKC_SHOW_DESKTOP:  // Show the desktop
            if (record->event.pressed) {
                register_code16(mac ? CKC_MAC_SD : CKC_WIN_SD);
            } else {
                unregister_code16(mac ? CKC_MAC_SD : CKC_WIN_SD);
            }
            return false;
        case CKC_SCREENSHOT:  // Take a screenshot
            if (record->event.pressed) {
                register_code16(mac ? CKC_MAC_SS : CKC_WIN_SS);
            } else {
                unregister_code16(mac ? CKC_MAC_SS : CKC_WIN_SS);
            }
            return false;
        case CKC_CUT:
            if (record->event.pressed) {
                register_code16(mac ? CKC_MAC_CUT : CKC_WIN_CUT);
            } else {
                unregister_code16(mac ? CKC_MAC_CUT : CKC_WIN_CUT);
            }
            return false;
        case CKC_COPY:
            if (record->event.pressed) {
                register_code16(mac ? CKC_MAC_COPY : CKC_WIN_COPY);
            } else {
                unregister_code16(mac ? CKC_MAC_COPY : CKC_WIN_COPY);
            }
            return false;
        case CKC_PASTE:
            if (record->event.pressed) {
                register_code16(mac ? CKC_MAC_PASTE : CKC_WIN_PASTE);
            } else {
                unregister_code16(mac ? CKC_MAC_PASTE : CKC_WIN_PASTE);
            }
            return false;
        default:
            return true;
    }
}

// Adjust Tap Dance TAPPING_TERM
uint16_t get_tapping_term(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case TD(TD_BTN2):
            return BTN2_TAPPING_TERM;
        case TD(TD_BTN4):
            return BTN4_TAPPING_TERM;
        case TD(TD_BTN5):
            return BTN5_TAPPING_TERM;
        default:
            return TAPPING_TERM;
    }
}

// Horizonal scroll with wheel on _RAISE layer
bool encoder_update_user(uint8_t index, bool clockwise) {
    if (IS_LAYER_ON(_RAISE)) {
        tap_code(clockwise ? KC_WH_L : KC_WH_R);
        return false;
    } else {
        return true;
    }
}



/* Tap Dance callback functions */

// Suports single/double taps and single hold; favors instant hold when interrupted
static td_action_t get_tap_dance_action(tap_dance_state_t *state) {
    if (state->count == 1) return (state->pressed) ? TD_SINGLE_HOLD : TD_SINGLE_TAP;
    else if (state->count == 2) return TD_DOUBLE_TAP;
    else return TD_UNKNOWN;
}

// Mouse button 2 Tap Dance on-each-tap callback
static void btn2_td_tap(tap_dance_state_t *state, void *user_data) {
    btn2_td_action = get_tap_dance_action(state);
    if (btn2_td_action == TD_DOUBLE_TAP) {
        register_code(KC_ENT);
        state->finished = true;
    }
}

// Mouse button 2 Tap Dance on-dance-finished callback
static void btn2_td_finished(tap_dance_state_t *state, void *user_data) {
    btn2_td_action = get_tap_dance_action(state);
    switch (btn2_td_action) {
        case TD_SINGLE_TAP:
            tap_code16(KC_BTN2);
            break;
        case TD_SINGLE_HOLD:
            layer_on(_DPI_CONTROL);
            break;
        default:
            break;
    }
}

// Mouse button 2 Tap Dance on-dance-reset callback
static void btn2_td_reset(tap_dance_state_t *state, void *user_data) {
    switch (btn2_td_action) {
        case TD_DOUBLE_TAP:
            unregister_code(KC_ENT);
            break;
        case TD_SINGLE_HOLD:
            layer_off(_DPI_CONTROL);
            break;
        default:
            break;
    }
    btn2_td_action = TD_NONE;
}

// Mouse button 4 Tap Dance on-each-tap callback
static void btn4_td_tap(tap_dance_state_t *state, void *user_data) {
    btn4_td_action = get_tap_dance_action(state);
    if (btn4_td_action == TD_DOUBLE_TAP) {
        register_code16(mac ? CKC_MAC_DL : CKC_WIN_DL);
        state->finished = true;
    }
}

// Mouse button 4 Tap Dance on-dance-finished callback
static void btn4_td_finished(tap_dance_state_t *state, void *user_data) {
    btn4_td_action = get_tap_dance_action(state);
    switch (btn4_td_action) {
        case TD_SINGLE_TAP:
            tap_code16(KC_BTN4);
            break;
        case TD_SINGLE_HOLD:
            layer_on(_RAISE);
            register_custom_keycode(DRAG_SCROLL, 3, 0);
            break;
        default:
            break;
    }
}

// Mouse button 4 Tap Dance on-dance-reset callback
static void btn4_td_reset(tap_dance_state_t *state, void *user_data) {
    switch (btn4_td_action) {
        case TD_DOUBLE_TAP:
            unregister_code16(mac ? CKC_MAC_DL : CKC_WIN_DL);
            break;
        case TD_SINGLE_HOLD:
            layer_off(_RAISE);
            unregister_custom_keycode(DRAG_SCROLL, 3, 0);
            break;
        default:
            break;
    }
    btn4_td_action = TD_NONE;
}

// Mouse button 5 Tap Dance on-each-tap callback
static void btn5_td_tap(tap_dance_state_t *state, void *user_data) {
    btn5_td_action = get_tap_dance_action(state);
    if (btn5_td_action == TD_DOUBLE_TAP) {
        register_code16(mac ? CKC_MAC_DR : CKC_WIN_DR);
        state->finished = true;
    }
}

// Mouse button 5 Tap Dance on-dance-finished callback
static void btn5_td_finished(tap_dance_state_t *state, void *user_data) {
    btn5_td_action = get_tap_dance_action(state);
    switch (btn5_td_action) {
        case TD_SINGLE_TAP:
            tap_code16(KC_BTN5);
            break;
        case TD_SINGLE_HOLD:
            layer_on(_LOWER);
            break;
        default:
            break;
    }
}

// Mouse button 5 Tap Dance on-dance-reset callback
static void btn5_td_reset(tap_dance_state_t *state, void *user_data) {
    switch (btn5_td_action) {
        case TD_DOUBLE_TAP:
            unregister_code16(mac ? CKC_MAC_DR : CKC_WIN_DR);
            break;
        case TD_SINGLE_HOLD:
            layer_off(_LOWER);
            break;
        default:
            break;
    }
    btn5_td_action = TD_NONE;
}



/* Functions for sending custom keycodes */

// Setup fake_record for process_record_kb()
static void setup_fake_record(uint8_t col, uint8_t row, bool pressed) {
    fake_record.event.key.col = col;
    fake_record.event.key.row = row;
    fake_record.event.pressed = pressed;
    fake_record.event.time = timer_read();
}

// Register a custom keycode with process_record_kb()
static void register_custom_keycode(uint16_t keycode, uint8_t col, uint8_t row) {
    setup_fake_record(col, row, true);
    process_record_kb(keycode, &fake_record);
}

// Unregister a custom keycode with process_record_kb()
static void unregister_custom_keycode(uint16_t keycode, uint8_t col, uint8_t row) {
    setup_fake_record(col, row, false);
    process_record_kb(keycode, &fake_record);
}

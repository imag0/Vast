#define main vast_v327_legacy_main_not_run
#include "../test_v30.c"
#undef main

/*
 * Host-side regression coverage for Android motion-event state machines.
 * test_v30.c supplies the same lightweight NDK stubs used by the legacy
 * suite, while these checks drive the production handle_motion() path.
 */

static int input_failures;
static int input_checks;
static int64_t input_clock = 20000;

static void input_check(int condition, const char *name) {
    input_checks++;
    if (condition) {
        printf("PASS %s\n", name);
    } else {
        printf("FAIL %s\n", name);
        input_failures++;
    }
}

static int nearf(float a, float b, float tolerance) {
    return fabsf(a - b) <= tolerance;
}

static void input_defaults(void) {
    memset(&G, 0, sizeof(G));
    G.screenW = 1400;
    G.screenH = 900;
    G.scale = 1.0f;
    G.uiScale = 1.0f;
    G.tool = MODE_PEN;
    G.brush = 5.5f;
    G.pressureMin = 0.18f;
    G.pressureMax = 1.0f;
    G.radialHoldSec = 1.0f;
    G.railOpen = 0;
    G.railAnim = 0.0f;
    G.minimap = 0;
    G.snap = 0;
    G.animTimerFd = -1;
    G.buttonTimerFd = -1;
    G.penHoldTimerFd = -1;
    G.railTimerFd = -1;
    G.currentStroke = -1;
    G.fingerId0 = -1;
    G.fingerId1 = -1;
    G.selectedImage = -1;
    G.selectedMeasure = -1;
    G.selectedFrame = -1;
    G.selectedNote = -1;
    G.selectedShape = -1;
    G.selectedBookmark = -1;
    G.selectedGroup = -1;
    G.nextFrameId = 1;
    G.nextNoteId = 1;
    G.nextShapeId = 1;
    G.nextBookmarkId = 1;
    G.nextGroupId = 1;
    G.visInk = 1;
    G.visMarker = 1;
    G.visPhotos = 1;
    G.visCAD = 1;
    G.visNotes = 1;
    G.visShapes = 1;
    G.visFrames = 1;
    theme_preset(0);
    G.metaDirty = 0;
}

static void finger_event(FakeEvent *event, int action, int id,
                         float x, float y) {
    memset(event, 0, sizeof(*event));
    event->type = AINPUT_EVENT_TYPE_MOTION;
    event->action = action;
    event->pc = 1;
    event->tool[0] = TOOL_FINGER;
    event->id[0] = id;
    event->x[0] = x;
    event->y[0] = y;
    event->t = input_clock += 20;
}

static void stylus_event(FakeEvent *event, int action, int id,
                         float x, float y) {
    finger_event(event, action, id, x, y);
    event->tool[0] = TOOL_STYLUS;
    event->p[0] = 0.5f;
}

static void two_finger_event(FakeEvent *event, int action,
                             int id0, float x0, float y0,
                             int id1, float x1, float y1) {
    memset(event, 0, sizeof(*event));
    event->type = AINPUT_EVENT_TYPE_MOTION;
    event->action = action;
    event->pc = 2;
    event->tool[0] = TOOL_FINGER;
    event->tool[1] = TOOL_FINGER;
    event->id[0] = id0;
    event->id[1] = id1;
    event->x[0] = x0;
    event->y[0] = y0;
    event->x[1] = x1;
    event->y[1] = y1;
    event->t = input_clock += 20;
}

static void test_ui_commit_contract(void) {
    FakeEvent event;
    const float button_x = 28.0f;
    const float button_y = 872.0f;

    input_defaults();
    input_check(ui_hit(button_x, button_y) == UI_INK_TOGGLE,
                "UI fixture targets the ink/erase control");

    finger_event(&event, AMOTION_ACTION_DOWN, 11, button_x, button_y);
    handle_motion((AInputEvent *)&event);
    input_check(G.pressedUi == UI_INK_TOGGLE && G.tool == MODE_PEN,
                "finger UI DOWN records pressed state without committing");

    finger_event(&event, AMOTION_ACTION_UP, 11, button_x, button_y);
    handle_motion((AInputEvent *)&event);
    input_check(G.pressedUi == UI_NONE && G.tool == MODE_ERASE,
                "finger UI UP over the same control commits once");

    G.tool = MODE_PEN;
    finger_event(&event, AMOTION_ACTION_DOWN, 12, button_x, button_y);
    handle_motion((AInputEvent *)&event);
    finger_event(&event, AMOTION_ACTION_MOVE, 12, 700.0f, 450.0f);
    handle_motion((AInputEvent *)&event);
    input_check(G.tool == MODE_PEN && G.pressedUi == UI_INK_TOGGLE,
                "dragging off a pressed UI control does not commit");
    finger_event(&event, AMOTION_ACTION_UP, 12, 700.0f, 450.0f);
    handle_motion((AInputEvent *)&event);
    input_check(G.tool == MODE_PEN && G.pressedUi == UI_NONE,
                "UI UP outside the original control cancels activation");

    finger_event(&event, AMOTION_ACTION_DOWN, 13, button_x, button_y);
    handle_motion((AInputEvent *)&event);
    finger_event(&event, AMOTION_ACTION_CANCEL, 13, button_x, button_y);
    handle_motion((AInputEvent *)&event);
    input_check(G.tool == MODE_PEN && G.pressedUi == UI_NONE,
                "finger UI ACTION_CANCEL clears press without activation");

    stylus_event(&event, AMOTION_ACTION_DOWN, 31, button_x, button_y);
    handle_motion((AInputEvent *)&event);
    input_check(G.tool == MODE_PEN && G.pressedUi == UI_INK_TOGGLE,
                "stylus UI DOWN also defers activation");
    stylus_event(&event, AMOTION_ACTION_UP, 31, button_x, button_y);
    handle_motion((AInputEvent *)&event);
    input_check(G.tool == MODE_ERASE && G.pressedUi == UI_NONE,
                "stylus UI UP commits through the shared release contract");
}

static void test_camera_cancel_rollback(void) {
    FakeEvent event;
    input_defaults();
    G.offX = 125.0f;
    G.offY = -35.0f;
    const float start_x = G.offX;
    const float start_y = G.offY;

    finger_event(&event, AMOTION_ACTION_DOWN, 41, 700.0f, 450.0f);
    handle_motion((AInputEvent *)&event);
    finger_event(&event, AMOTION_ACTION_MOVE, 41, 790.0f, 520.0f);
    handle_motion((AInputEvent *)&event);
    input_check(G.fingerPanning &&
                    (!nearf(G.offX, start_x, 0.001f) ||
                     !nearf(G.offY, start_y, 0.001f)),
                "one-finger camera drag mutates the live viewport");

    finger_event(&event, AMOTION_ACTION_CANCEL, 41, 790.0f, 520.0f);
    handle_motion((AInputEvent *)&event);
    input_check(nearf(G.offX, start_x, 0.001f) &&
                    nearf(G.offY, start_y, 0.001f) &&
                    nearf(G.scale, 1.0f, 0.001f),
                "camera ACTION_CANCEL restores pre-gesture viewport");
    input_check(G.fingerCount == 0 && !G.fingerPanning && !G.fingerMoved &&
                    G.fingerId0 == -1 && G.fingerId1 == -1,
                "camera ACTION_CANCEL clears transient pointer state");
}

static float simulate_fling_distance(int hz) {
    input_defaults();
    G.flingActive = 1;
    G.flingVX = 3000.0f;
    G.flingVY = 0.0f;
    for (int i = 0; i < hz; i++) fling_advance(1.0f / (float)hz);
    return G.offX;
}

static void test_camera_fling(void) {
    FakeEvent event;
    input_defaults();
    finger_event(&event, AMOTION_ACTION_DOWN, 201, 420.0f, 420.0f);
    handle_motion((AInputEvent *)&event);
    finger_event(&event, AMOTION_ACTION_MOVE, 201, 520.0f, 450.0f);
    handle_motion((AInputEvent *)&event);
    finger_event(&event, AMOTION_ACTION_MOVE, 201, 650.0f, 490.0f);
    handle_motion((AInputEvent *)&event);
    finger_event(&event, AMOTION_ACTION_UP, 201, 650.0f, 490.0f);
    handle_motion((AInputEvent *)&event);
    input_check(G.flingActive && G.flingVX > 0.0f && G.flingVY > 0.0f,
                "fast camera release starts momentum in the release direction");
    input_check(sqrtf(G.flingVX * G.flingVX + G.flingVY * G.flingVY) <= 3200.01f,
                "release velocity is capped at the controlled maximum");
    {
        float before_x = G.offX, before_y = G.offY;
        fling_advance(1.0f / 120.0f);
        input_check(G.offX > before_x && G.offY > before_y,
                    "fling advances only the camera along its sampled vector");
    }

    finger_event(&event, AMOTION_ACTION_DOWN, 202, 700.0f, 450.0f);
    handle_motion((AInputEvent *)&event);
    input_check(!G.flingActive,
                "new finger contact cancels camera momentum immediately");

    G.flingActive = 1; G.flingVX = 1600.0f; G.flingVY = 200.0f;
    two_finger_event(&event, AMOTION_ACTION_POINTER_DOWN | (1 << 8),
                     202, 700.0f, 450.0f,
                     204, 900.0f, 450.0f);
    handle_motion((AInputEvent *)&event);
    input_check(!G.flingActive && G.fingerCount == 2,
                "pinch contact cancels momentum before zoom handling");
    two_finger_event(&event, AMOTION_ACTION_CANCEL,
                     202, 700.0f, 450.0f,
                     204, 900.0f, 450.0f);
    handle_motion((AInputEvent *)&event);

    G.flingActive = 1; G.flingVX = 1800.0f; G.flingVY = -400.0f;
    stylus_event(&event, AMOTION_ACTION_DOWN, 203, 760.0f, 520.0f);
    handle_motion((AInputEvent *)&event);
    input_check(!G.flingActive && G.stylusDown,
                "stylus contact cancels momentum before canvas work begins");
    stylus_event(&event, AMOTION_ACTION_CANCEL, 203, 760.0f, 520.0f);
    handle_motion((AInputEvent *)&event);

    input_defaults();
    fling_track_reset(input_clock);
    fling_track_add(2.0f, 1.0f, input_clock += 25);
    fling_track_add(2.0f, 1.0f, input_clock += 25);
    fling_release(input_clock += 20);
    input_check(!G.flingActive,
                "slow camera release does not start momentum");

    {
        float d60 = simulate_fling_distance(60);
        float d90 = simulate_fling_distance(90);
        float d120 = simulate_fling_distance(120);
        input_check(nearf(d60, d90, 0.05f) && nearf(d90, d120, 0.05f),
                    "elapsed-time deceleration is refresh-rate independent");
    }
}

static void test_radial_progress_delay(void) {
    input_defaults();
    G.radialHoldSec = 1.0f;
    G.penHoldStartMs = 10000;
    input_check(nearf(radial_progress_value(10300), 0.0f, 0.0001f),
                "radial progress stays hidden during the longer pan grace period");
    input_check(radial_progress_value(10420) > 0.09f &&
                    radial_progress_value(10420) < 0.11f,
                "radial progress begins smoothly after the pan grace period");
}

static void test_selection_cancel_rollback(void) {
    FakeEvent event;
    input_defaults();
    G.noteN = 1;
    G.notes[0] = (NoteObj){300.0f, 250.0f, 180.0f, 100.0f,
                           0x557799u, 1, 1, NOTE_TEXT, 0, "cancel me"};
    selection_set_one(SEL_NOTE, 0);
    const float start_x = G.notes[0].x;
    const float start_y = G.notes[0].y;

    finger_event(&event, AMOTION_ACTION_DOWN, 51, 350.0f, 300.0f);
    handle_motion((AInputEvent *)&event);
    input_check(G.selectionDragging,
                "one-finger press on a selected object starts selection drag");
    finger_event(&event, AMOTION_ACTION_MOVE, 51, 430.0f, 360.0f);
    handle_motion((AInputEvent *)&event);
    input_check(!nearf(G.notes[0].x, start_x, 0.001f) &&
                    !nearf(G.notes[0].y, start_y, 0.001f),
                "selection drag mutates the live object position");

    finger_event(&event, AMOTION_ACTION_CANCEL, 51, 430.0f, 360.0f);
    handle_motion((AInputEvent *)&event);
    input_check(nearf(G.notes[0].x, start_x, 0.001f) &&
                    nearf(G.notes[0].y, start_y, 0.001f),
                "selection ACTION_CANCEL restores pre-drag object position");
    input_check(!G.selectionDragging &&
                    nearf(G.selectionDragTotalX, 0.0f, 0.001f) &&
                    nearf(G.selectionDragTotalY, 0.0f, 0.001f),
                "selection ACTION_CANCEL clears accumulated drag state");
}

static void test_reordered_pointer_pinch(void) {
    FakeEvent event;
    input_defaults();
    G.imageN = 1;
    G.images[0].active = 1;
    G.images[0].x = 300.0f;
    G.images[0].y = 300.0f;
    G.images[0].w = 400.0f;
    G.images[0].h = 300.0f;
    G.photoMode = 1;
    G.photoAngleSnap = 0;
    G.selectedImage = 0;

    finger_event(&event, AMOTION_ACTION_DOWN, 61, 400.0f, 450.0f);
    handle_motion((AInputEvent *)&event);
    two_finger_event(&event, AMOTION_ACTION_POINTER_DOWN,
                     61, 400.0f, 450.0f,
                     77, 600.0f, 450.0f);
    handle_motion((AInputEvent *)&event);
    input_check(G.imageGesture && G.fingerId0 == 61 && G.fingerId1 == 77,
                "pinch captures stable Android pointer IDs");

    /* Android may reorder the index array while keeping pointer IDs stable. */
    two_finger_event(&event, AMOTION_ACTION_MOVE,
                     77, 610.0f, 470.0f,
                     61, 390.0f, 430.0f);
    handle_motion((AInputEvent *)&event);
    const float expected_angle_1 = atan2f(40.0f, 220.0f);
    input_check(nearf(G.images[0].rot, expected_angle_1, 0.001f),
                "reordered pointer indices preserve pinch rotation direction");
    input_check(nearf(G.images[0].x + G.images[0].w * 0.5f, 500.0f, 0.001f) &&
                    nearf(G.images[0].y + G.images[0].h * 0.5f, 450.0f, 0.001f),
                "reordered pointer indices preserve pinch midpoint");

    two_finger_event(&event, AMOTION_ACTION_MOVE,
                     61, 380.0f, 420.0f,
                     77, 620.0f, 480.0f);
    handle_motion((AInputEvent *)&event);
    const float expected_angle_2 = atan2f(60.0f, 240.0f);
    input_check(nearf(G.images[0].rot, expected_angle_2, 0.001f) &&
                    G.images[0].w > 400.0f && G.images[0].h > 300.0f,
                "pinch remains continuous when indices return to original order");

    two_finger_event(&event, AMOTION_ACTION_CANCEL,
                     61, 380.0f, 420.0f,
                     77, 620.0f, 480.0f);
    handle_motion((AInputEvent *)&event);
    input_check(nearf(G.images[0].x, 300.0f, 0.001f) &&
                    nearf(G.images[0].y, 300.0f, 0.001f) &&
                    nearf(G.images[0].w, 400.0f, 0.001f) &&
                    nearf(G.images[0].h, 300.0f, 0.001f) &&
                    nearf(G.images[0].rot, 0.0f, 0.001f),
                "pinch ACTION_CANCEL restores the complete image transform");
}

static void test_locked_object_mutations(void) {
    FakeEvent event;
    input_defaults();
    G.noteN = 1;
    G.notes[0] = (NoteObj){320.0f, 260.0f, 180.0f, 100.0f,
                           0x224466u, 1, 1, NOTE_TEXT, 0, "locked"};
    selection_set_one(SEL_NOTE, 0);
    G.lockNotes = 1;

    finger_event(&event, AMOTION_ACTION_DOWN, 81, 360.0f, 300.0f);
    handle_motion((AInputEvent *)&event);
    finger_event(&event, AMOTION_ACTION_MOVE, 81, 440.0f, 360.0f);
    handle_motion((AInputEvent *)&event);
    finger_event(&event, AMOTION_ACTION_UP, 81, 440.0f, 360.0f);
    handle_motion((AInputEvent *)&event);
    input_check(nearf(G.notes[0].x, 320.0f, 0.001f) &&
                    nearf(G.notes[0].y, 260.0f, 0.001f),
                "class lock prevents one-finger object movement");

    selection_set_one(SEL_NOTE, 0);
    selection_duplicate();
    input_check(G.noteN == 1,
                "class lock prevents selection duplication");

    selection_set_one(SEL_NOTE, 0);
    G.color = 0xff3300u;
    selection_apply_color();
    input_check(G.notes[0].color == 0x224466u,
                "class lock prevents selection recolor");

    selection_set_one(SEL_NOTE, 0);
    selection_delete();
    input_check(G.noteN == 1 && G.notes[0].active,
                "class lock prevents selection deletion");

    G.lockNotes = 0;
    G.notes[0].locked = 1;
    selection_set_one(SEL_NOTE, 0);
    input_check(!objref_can_mutate(G.selectedRefs[0]),
                "per-object lock is included in mutation eligibility");
    selection_duplicate();
    input_check(G.noteN == 1,
                "per-object lock prevents selection duplication");
}

static void test_selection_drag_to_pinch_cancel_rollback(void) {
    FakeEvent event;
    input_defaults();
    G.noteN = 1;
    G.notes[0] = (NoteObj){300.0f, 250.0f, 180.0f, 100.0f,
                           0x6688aau, 7, 1, NOTE_TEXT, 0, "whole gesture"};
    const NoteObj original = G.notes[0];
    selection_set_one(SEL_NOTE, 0);

    finger_event(&event, AMOTION_ACTION_DOWN, 91, 350.0f, 300.0f);
    handle_motion((AInputEvent *)&event);
    finger_event(&event, AMOTION_ACTION_MOVE, 91, 390.0f, 330.0f);
    handle_motion((AInputEvent *)&event);
    input_check(G.selectionDragging &&
                    !nearf(G.notes[0].x, original.x, 0.001f),
                "selection moves during the one-finger phase of a compound gesture");

    two_finger_event(&event, AMOTION_ACTION_POINTER_DOWN,
                     91, 390.0f, 330.0f,
                     92, 480.0f, 350.0f);
    handle_motion((AInputEvent *)&event);
    input_check(G.selectionGesture,
                "second pointer transitions selection drag into a two-finger gesture");
    two_finger_event(&event, AMOTION_ACTION_MOVE,
                     91, 380.0f, 320.0f,
                     92, 500.0f, 380.0f);
    handle_motion((AInputEvent *)&event);
    two_finger_event(&event, AMOTION_ACTION_CANCEL,
                     91, 380.0f, 320.0f,
                     92, 500.0f, 380.0f);
    handle_motion((AInputEvent *)&event);

    input_check(nearf(G.notes[0].x, original.x, 0.001f) &&
                    nearf(G.notes[0].y, original.y, 0.001f) &&
                    nearf(G.notes[0].w, original.w, 0.001f) &&
                    nearf(G.notes[0].h, original.h, 0.001f),
                "compound selection ACTION_CANCEL restores the pre-DOWN object state");
    input_check(!G.selectionDragging && !G.selectionGesture &&
                    G.transformSnapshotN == 0 &&
                    nearf(G.selectionDragTotalX, 0.0f, 0.001f) &&
                    nearf(G.selectionDragTotalY, 0.0f, 0.001f),
                "compound selection ACTION_CANCEL clears both gesture phases");
}

static void test_frame_resize_to_pinch_cancel_rollback(void) {
    FakeEvent event;
    input_defaults();
    G.frameN = 1;
    G.frames[0] = (FrameObj){200.0f, 200.0f, 300.0f, 200.0f,
                             0x7799bbu, 11, 1};
    const FrameObj original = G.frames[0];
    selection_set_one(SEL_FRAME, 0);

    finger_event(&event, AMOTION_ACTION_DOWN, 101, 500.0f, 400.0f);
    handle_motion((AInputEvent *)&event);
    input_check(G.frameResizeDragging,
                "one-finger DOWN on a frame corner starts resize");
    finger_event(&event, AMOTION_ACTION_MOVE, 101, 560.0f, 460.0f);
    handle_motion((AInputEvent *)&event);
    input_check(G.frames[0].w > original.w && G.frames[0].h > original.h,
                "frame resize mutates live geometry before completion");

    two_finger_event(&event, AMOTION_ACTION_POINTER_DOWN,
                     101, 560.0f, 460.0f,
                     102, 620.0f, 500.0f);
    handle_motion((AInputEvent *)&event);
    two_finger_event(&event, AMOTION_ACTION_CANCEL,
                     101, 560.0f, 460.0f,
                     102, 620.0f, 500.0f);
    handle_motion((AInputEvent *)&event);

    input_check(nearf(G.frames[0].x, original.x, 0.001f) &&
                    nearf(G.frames[0].y, original.y, 0.001f) &&
                    nearf(G.frames[0].w, original.w, 0.001f) &&
                    nearf(G.frames[0].h, original.h, 0.001f),
                "frame resize followed by POINTER_DOWN and CANCEL restores original frame");
    input_check(!G.frameResizeDragging && G.fingerCount == 0 &&
                    G.fingerId0 == -1 && G.fingerId1 == -1,
                "cancelled compound frame resize clears pointer state");
}

static void test_pointer_up_suppresses_final_single_tap(void) {
    FakeEvent event;
    input_defaults();
    G.noteN = 1;
    G.notes[0] = (NoteObj){300.0f, 250.0f, 180.0f, 100.0f,
                           0x335577u, 13, 1, NOTE_TEXT, 0, "stay selected"};
    selection_set_one(SEL_NOTE, 0);

    finger_event(&event, AMOTION_ACTION_DOWN, 111, 800.0f, 500.0f);
    handle_motion((AInputEvent *)&event);
    two_finger_event(&event, AMOTION_ACTION_POINTER_DOWN,
                     111, 800.0f, 500.0f,
                     112, 900.0f, 500.0f);
    handle_motion((AInputEvent *)&event);
    two_finger_event(&event, AMOTION_ACTION_POINTER_UP,
                     111, 800.0f, 500.0f,
                     112, 900.0f, 500.0f);
    handle_motion((AInputEvent *)&event);
    input_check(G.selectedRefN == 1 &&
                    G.selectedRefs[0].type == SEL_NOTE &&
                    G.selectedRefs[0].index == 0,
                "two-finger POINTER_UP leaves prior selection intact");

    finger_event(&event, AMOTION_ACTION_UP, 111, 800.0f, 500.0f);
    handle_motion((AInputEvent *)&event);
    input_check(G.selectedRefN == 1 &&
                    G.selectedRefs[0].type == SEL_NOTE &&
                    G.selectedRefs[0].index == 0,
                "final single ACTION_UP after multi-touch cannot clear selection");
}

static void test_minimap_cancel_rollback(void) {
    FakeEvent event;
    static uint32_t minimap_pixel;
    input_defaults();
    G.minimap = 1;
    G.minimapCache = &minimap_pixel;
    G.minimapCacheW = 1;
    G.minimapCacheH = 1;
    G.minimapMinX = -1000.0f;
    G.minimapMinY = -800.0f;
    G.minimapMaxX = 1400.0f;
    G.minimapMaxY = 1200.0f;
    G.offX = 85.0f;
    G.offY = -45.0f;
    G.scale = 1.25f;
    const float original_x = G.offX;
    const float original_y = G.offY;
    const float original_scale = G.scale;

    const int margin = margin_ui();
    const int map_w = us(320);
    const int map_h = us(205);
    const int pad = us(14);
    const int top = us(40);
    const float map_x = (float)(G.screenW - margin - map_w + pad +
                                (map_w - 2 * pad) / 2);
    const float map_y = (float)(G.screenH - margin - map_h - us(100) + top +
                                (map_h - top - pad) / 2);

    finger_event(&event, AMOTION_ACTION_DOWN, 121, map_x, map_y);
    handle_motion((AInputEvent *)&event);
    input_check(!nearf(G.offX, original_x, 0.001f) ||
                    !nearf(G.offY, original_y, 0.001f),
                "minimap DOWN navigates the live camera");
    finger_event(&event, AMOTION_ACTION_CANCEL, 121, map_x, map_y);
    handle_motion((AInputEvent *)&event);
    input_check(nearf(G.offX, original_x, 0.001f) &&
                    nearf(G.offY, original_y, 0.001f) &&
                    nearf(G.scale, original_scale, 0.001f),
                "minimap ACTION_CANCEL restores pre-DOWN camera");
    input_check(G.fingerCount == 0 && G.fingerId0 == -1 &&
                    G.fingerId1 == -1,
                "minimap ACTION_CANCEL leaves no active pointer state");
    G.minimapCache = 0;
}

static int image_transform_equals(const ImageObj *image,
                                  float x, float y, float w, float h,
                                  float rotation) {
    return nearf(image->x, x, 0.001f) && nearf(image->y, y, 0.001f) &&
           nearf(image->w, w, 0.001f) && nearf(image->h, h, 0.001f) &&
           nearf(image->rot, rotation, 0.001f);
}

static void setup_selected_photo(void) {
    input_defaults();
    G.imageN = 1;
    G.images[0].active = 1;
    G.images[0].x = 300.0f;
    G.images[0].y = 300.0f;
    G.images[0].w = 400.0f;
    G.images[0].h = 300.0f;
    G.images[0].rot = 0.15f;
    G.photoMode = 1;
    G.photoAngleSnap = 0;
    G.selectedImage = 0;
}

static void test_photo_drag_cancel_whole_gesture(void) {
    FakeEvent event;

    setup_selected_photo();
    finger_event(&event, AMOTION_ACTION_DOWN, 131, 400.0f, 400.0f);
    handle_motion((AInputEvent *)&event);
    finger_event(&event, AMOTION_ACTION_MOVE, 131, 450.0f, 440.0f);
    handle_motion((AInputEvent *)&event);
    input_check(G.imageDragging &&
                    !image_transform_equals(&G.images[0],
                                            300.0f, 300.0f,
                                            400.0f, 300.0f, 0.15f),
                "direct selected-photo drag mutates the live image transform");
    finger_event(&event, AMOTION_ACTION_CANCEL, 131, 450.0f, 440.0f);
    handle_motion((AInputEvent *)&event);
    input_check(image_transform_equals(&G.images[0],
                                       300.0f, 300.0f,
                                       400.0f, 300.0f, 0.15f),
                "direct selected-photo ACTION_CANCEL restores pre-DOWN transform");
    input_check(!G.imageDragging && !G.imageGesture && !G.imageCancelValid,
                "direct photo cancellation clears image gesture state");

    setup_selected_photo();
    finger_event(&event, AMOTION_ACTION_DOWN, 141, 400.0f, 400.0f);
    handle_motion((AInputEvent *)&event);
    finger_event(&event, AMOTION_ACTION_MOVE, 141, 450.0f, 430.0f);
    handle_motion((AInputEvent *)&event);
    input_check(nearf(G.images[0].x, 350.0f, 0.001f) &&
                    nearf(G.images[0].y, 330.0f, 0.001f),
                "photo moves before transitioning from drag to pinch");
    two_finger_event(&event, AMOTION_ACTION_POINTER_DOWN,
                     141, 450.0f, 430.0f,
                     142, 620.0f, 470.0f);
    handle_motion((AInputEvent *)&event);
    input_check(G.imageGesture && !G.imageDragging && !G.twoTapCandidate,
                "second pointer transitions photo drag into pinch without undo arming");
    two_finger_event(&event, AMOTION_ACTION_MOVE,
                     141, 430.0f, 410.0f,
                     142, 660.0f, 500.0f);
    handle_motion((AInputEvent *)&event);
    input_check(!nearf(G.images[0].w, 400.0f, 0.001f) &&
                    !nearf(G.images[0].rot, 0.15f, 0.001f),
                "two-finger phase mutates photo scale and rotation");
    two_finger_event(&event, AMOTION_ACTION_CANCEL,
                     141, 430.0f, 410.0f,
                     142, 660.0f, 500.0f);
    handle_motion((AInputEvent *)&event);
    input_check(image_transform_equals(&G.images[0],
                                       300.0f, 300.0f,
                                       400.0f, 300.0f, 0.15f),
                "drag-to-pinch ACTION_CANCEL restores the original pre-DOWN photo");
    input_check(!G.imageDragging && !G.imageGesture && !G.imageCancelValid &&
                    G.fingerCount == 0,
                "compound photo cancellation clears all gesture state");
}

static void test_mutation_before_pointer_down_cannot_arm_undo(void) {
    FakeEvent event;

    /* Selected-object movement followed by a second pointer outside the
       selection must become navigation, not a two-finger tap candidate. */
    input_defaults();
    G.noteN = 1;
    G.notes[0] = (NoteObj){300.0f, 250.0f, 180.0f, 100.0f,
                           0x446688u, 17, 1, NOTE_TEXT, 0, "moving"};
    selection_set_one(SEL_NOTE, 0);
    finger_event(&event, AMOTION_ACTION_DOWN, 151, 350.0f, 300.0f);
    handle_motion((AInputEvent *)&event);
    finger_event(&event, AMOTION_ACTION_MOVE, 151, 400.0f, 340.0f);
    handle_motion((AInputEvent *)&event);
    two_finger_event(&event, AMOTION_ACTION_POINTER_DOWN,
                     151, 400.0f, 340.0f,
                     152, 900.0f, 650.0f);
    handle_motion((AInputEvent *)&event);
    input_check(!G.twoTapCandidate,
                "second pointer after selection movement cannot arm undo");
    two_finger_event(&event, AMOTION_ACTION_CANCEL,
                     151, 400.0f, 340.0f,
                     152, 900.0f, 650.0f);
    handle_motion((AInputEvent *)&event);

    input_defaults();
    G.frameN = 1;
    G.frames[0] = (FrameObj){200.0f, 200.0f, 300.0f, 200.0f,
                             0x557799u, 19, 1};
    selection_set_one(SEL_FRAME, 0);
    finger_event(&event, AMOTION_ACTION_DOWN, 161, 500.0f, 400.0f);
    handle_motion((AInputEvent *)&event);
    finger_event(&event, AMOTION_ACTION_MOVE, 161, 560.0f, 460.0f);
    handle_motion((AInputEvent *)&event);
    two_finger_event(&event, AMOTION_ACTION_POINTER_DOWN,
                     161, 560.0f, 460.0f,
                     162, 630.0f, 510.0f);
    handle_motion((AInputEvent *)&event);
    input_check(!G.twoTapCandidate,
                "second pointer during frame resize cannot arm undo");
    two_finger_event(&event, AMOTION_ACTION_CANCEL,
                     161, 560.0f, 460.0f,
                     162, 630.0f, 510.0f);
    handle_motion((AInputEvent *)&event);

    {
        static uint32_t minimap_pixel;
        input_defaults();
        G.minimap = 1;
        G.minimapCache = &minimap_pixel;
        G.minimapCacheW = G.minimapCacheH = 1;
        G.minimapMinX = -1000.0f;
        G.minimapMinY = -800.0f;
        G.minimapMaxX = 1400.0f;
        G.minimapMaxY = 1200.0f;
        const int margin = margin_ui();
        const int map_w = us(320), map_h = us(205);
        const int pad = us(14), top = us(40);
        const float map_x = (float)(G.screenW - margin - map_w + pad +
                                    (map_w - 2 * pad) / 2);
        const float map_y = (float)(G.screenH - margin - map_h - us(100) + top +
                                    (map_h - top - pad) / 2);
        finger_event(&event, AMOTION_ACTION_DOWN, 171, map_x, map_y);
        handle_motion((AInputEvent *)&event);
        two_finger_event(&event, AMOTION_ACTION_POINTER_DOWN,
                         171, map_x, map_y,
                         172, map_x - 120.0f, map_y - 100.0f);
        handle_motion((AInputEvent *)&event);
        input_check(!G.twoTapCandidate,
                    "second pointer after minimap navigation cannot arm undo");
        two_finger_event(&event, AMOTION_ACTION_CANCEL,
                         171, map_x, map_y,
                         172, map_x - 120.0f, map_y - 100.0f);
        handle_motion((AInputEvent *)&event);
        G.minimapCache = 0;
    }

    input_defaults();
    finger_event(&event, AMOTION_ACTION_DOWN, 181, 700.0f, 450.0f);
    handle_motion((AInputEvent *)&event);
    finger_event(&event, AMOTION_ACTION_MOVE, 181, 790.0f, 520.0f);
    handle_motion((AInputEvent *)&event);
    two_finger_event(&event, AMOTION_ACTION_POINTER_DOWN,
                     181, 790.0f, 520.0f,
                     182, 900.0f, 560.0f);
    handle_motion((AInputEvent *)&event);
    input_check(!G.twoTapCandidate,
                "second pointer after canvas pan cannot arm undo");
    two_finger_event(&event, AMOTION_ACTION_CANCEL,
                     181, 790.0f, 520.0f,
                     182, 900.0f, 560.0f);
    handle_motion((AInputEvent *)&event);
}

static void test_eraser_contact_ownership(void){
    FakeEvent e;input_defaults();erase_index_reset();G.tool=MODE_ERASE;
    finger_event(&e,AMOTION_ACTION_DOWN,41,600,400);handle_motion((AInputEvent*)&e);
    input_check(G.fingerCount==1&&!G.erasing&&!eraser_contact_visible(),"eraser finger DOWN owns navigation, no reticle");
    finger_event(&e,AMOTION_ACTION_MOVE,41,650,430);handle_motion((AInputEvent*)&e);
    input_check(G.offX==50&&G.offY==30&&!eraser_contact_visible(),"eraser finger MOVE pans without reticle");
    two_finger_event(&e,AMOTION_ACTION_POINTER_DOWN|(1<<8),41,650,430,84,800,430);handle_motion((AInputEvent*)&e);
    input_check(G.fingerCount==2&&!G.erasing&&!eraser_contact_visible(),"second navigation finger never starts eraser");
    two_finger_event(&e,AMOTION_ACTION_CANCEL,41,650,430,84,800,430);handle_motion((AInputEvent*)&e);
    stylus_event(&e,AMOTION_ACTION_HOVER_MOVE,99,700,450);handle_motion((AInputEvent*)&e);
    input_check(!eraser_contact_visible(),"stylus hover never shows eraser reticle");
    stylus_event(&e,AMOTION_ACTION_DOWN,99,700,450);handle_motion((AInputEvent*)&e);
    input_check(G.stylusPointerId==99&&eraser_contact_visible(),"actual stylus DOWN owns eraser and reticle");
    two_finger_event(&e,AMOTION_ACTION_POINTER_DOWN|(1<<8),99,700,450,17,900,600);e.tool[0]=TOOL_STYLUS;handle_motion((AInputEvent*)&e);
    input_check(G.stylusPointerId==99&&G.erasing,"other pointer DOWN cannot steal stylus owner");
    two_finger_event(&e,AMOTION_ACTION_MOVE,17,900,600,99,730,470);e.tool[1]=TOOL_STYLUS;handle_motion((AInputEvent*)&e);
    input_check(G.hoverX==730&&G.hoverY==470&&eraser_contact_visible(),"reordered indices track the stylus pointer ID");
    two_finger_event(&e,AMOTION_ACTION_POINTER_UP|(1<<8),17,900,600,99,730,470);e.tool[1]=TOOL_STYLUS;handle_motion((AInputEvent*)&e);
    input_check(!G.erasing&&!eraser_contact_visible(),"stylus POINTER_UP hides reticle with finger remaining");
    stylus_event(&e,AMOTION_ACTION_DOWN,100,700,450);handle_motion((AInputEvent*)&e);
    finger_event(&e,AMOTION_ACTION_CANCEL,17,900,600);handle_motion((AInputEvent*)&e);
    input_check(!G.stylusDown&&!G.erasing&&!eraser_contact_visible(),"CANCEL without stylus pointer clears contact");
    stylus_event(&e,AMOTION_ACTION_DOWN,100,700,450);handle_motion((AInputEvent*)&e);e.action=AMOTION_ACTION_CANCEL;e.pc=0;handle_motion((AInputEvent*)&e);
    input_check(!G.stylusDown&&!G.erasing,"zero-pointer CANCEL clears contact");
    stylus_event(&e,AMOTION_ACTION_DOWN,100,700,450);handle_motion((AInputEvent*)&e);
    stylus_event(&e,AMOTION_ACTION_MOVE,100,28,872);handle_motion((AInputEvent*)&e);
    input_check(G.pressedUi==0&&G.erasing,"owned stylus MOVE across a control stays on canvas");
    stylus_event(&e,AMOTION_ACTION_UP,100,28,872);handle_motion((AInputEvent*)&e);
    input_check(G.tool==MODE_ERASE&&!eraser_contact_visible(),"canvas release over UI cannot activate a second system");
    ensure_strokes(1);G.strokeN=1;memset(G.strokes,0,sizeof(Stroke));G.strokes[0].active=1;append_point(&G.strokes[0],700,450,.7f);append_point(&G.strokes[0],750,450,.7f);erase_index_reset();
    stylus_event(&e,AMOTION_ACTION_DOWN,100,700,450);handle_motion((AInputEvent*)&e);
    input_check(!G.strokes[0].active&&G.eraseAction.n==1,"eraser contact removes nearby ink once");
    finger_event(&e,AMOTION_ACTION_CANCEL,17,900,600);handle_motion((AInputEvent*)&e);
    input_check(G.strokes[0].active&&G.eraseAction.n==0&&G.actionN==0,"cross-tool CANCEL restores ink without an undo entry");
    stylus_event(&e,AMOTION_ACTION_DOWN,100,700,450);handle_motion((AInputEvent*)&e);execute_button_action(BA_PEN,0);
    input_check(G.tool==MODE_ERASE&&G.erasing,"side button cannot replace the active canvas tool");
    stylus_event(&e,AMOTION_ACTION_UP,100,750,450);handle_motion((AInputEvent*)&e);
    input_check(!G.strokes[0].active&&G.actionN==1&&G.actions[0].n==1,"eraser gesture commits a single undo action");undo_action();input_check(G.strokes[0].active,"normal undo restores erased ink");
}
int main(void) {
    test_eraser_contact_ownership();
    test_ui_commit_contract();
    test_camera_cancel_rollback();
    test_camera_fling();
    test_radial_progress_delay();
    test_selection_cancel_rollback();
    test_reordered_pointer_pinch();
    test_locked_object_mutations();
    test_selection_drag_to_pinch_cancel_rollback();
    test_frame_resize_to_pinch_cancel_rollback();
    test_pointer_up_suppresses_final_single_tap();
    test_minimap_cancel_rollback();
    test_photo_drag_cancel_whole_gesture();
    test_mutation_before_pointer_down_cannot_arm_undo();
    printf("input regression checks: %d, failures: %d\n",
           input_checks, input_failures);
    return input_failures ? 1 : 0;
}

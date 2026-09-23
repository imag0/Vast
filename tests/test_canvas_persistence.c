#define main vast_v327_regression_not_run
#include "../test_v30.c"
#undef main

static const char *META_PATH = "./build/test_canvas_persistence.meta";
static const char *WORKSPACE_PATH = "./build/test_canvas_persistence.v3";
static const char *FRAME_PATH = "./build/test_canvas_persistence.vfr";

static void persistence_defaults(void) {
    memset(&G, 0, sizeof(G));
    G.pressureMin = 0.18f;
    G.pressureMax = 1.0f;
    G.pressureCurve = 1;
    G.snap = 1;
    G.uiScale = 1.0f;
    G.pressureSmoothing = 0.42f;
    G.strokeSmoothing = 0.54f;
    G.highlighterOpacity = 0.26f;
    G.brush = 5.5f;
    G.cadScale = 1.0f;
    G.cadUnit = 2;
    G.minimap = 1;
    G.radialHoldSec = 1.0f;
    G.performanceMode = 2;
    G.photoAngleSnap = 1;
    G.scale = 1.0f;
    G.animTimerFd = -1;
    G.buttonTimerFd = -1;
    G.penHoldTimerFd = -1;
    G.railTimerFd = -1;
    snprintf(G.metaPath, sizeof(G.metaPath), "%s", META_PATH);
    snprintf(G.workspacePath, sizeof(G.workspacePath), "%s", WORKSPACE_PATH);
    snprintf(G.framePath, sizeof(G.framePath), "%s", FRAME_PATH);
}

static FrameHead valid_frame_head(char version, int32_t count) {
    FrameHead h;
    memset(&h, 0, sizeof(h));
    h.magic[0] = 'V'; h.magic[1] = 'F'; h.magic[2] = 'R';
    h.magic[3] = version;
    h.count = count;
    h.nextId = 41;
    h.visMask = 1 | 4 | 16 | 32;
    return h;
}

static MetaHead7 valid_meta7(uint32_t count) {
    MetaHead7 h;
    memset(&h, 0, sizeof(h));
    h.magic[0] = 'I'; h.magic[1] = 'C'; h.magic[2] = 'M'; h.magic[3] = '7';
    h.pmin = 0.18f;
    h.pmax = 1.0f;
    h.curve = 1;
    h.snap = 1;
    h.count = count;
    h.uiScale = 1.0f;
    h.pressAction = BA_UNDO;
    h.holdAction = BA_QUICK_ERASE;
    h.doubleAction = BA_REDO;
    h.pressureSmoothing = 0.42f;
    h.strokeSmoothing = 0.54f;
    h.highlighterOpacity = 0.26f;
    h.brush = 5.5f;
    h.inkColor = 0x123456u;
    h.cadScale = 2.5f;
    h.cadCalibrated = 1;
    h.cadUnit = 3;
    h.minimap = 1;
    h.radialHoldSec = 1.0f;
    h.buttonBindingsEnabled = 1;
    h.gridStyle = 2;
    h.farZoomMode = 1;
    h.performanceMode = 1;
    return h;
}

static WorkspaceHead valid_workspace_head(int notes, int shapes,
                                          int bookmarks, int groups) {
    WorkspaceHead h;
    memset(&h, 0, sizeof(h));
    h.magic[0] = 'V'; h.magic[1] = 'W'; h.magic[2] = 'S'; h.magic[3] = '1';
    h.noteN = notes;
    h.shapeN = shapes;
    h.bookmarkN = bookmarks;
    h.groupN = groups;
    h.nextNoteId = 2;
    h.nextShapeId = 2;
    h.nextBookmarkId = 2;
    h.nextGroupId = 2;
    h.scale = 1.25f;
    h.offX = 10.0f;
    h.offY = -20.0f;
    return h;
}

static void test_latest_meta_settings_round_trip(void) {
    persistence_defaults();
    G.performanceMode = 0;
    G.photoAngleSnap = 0;
    G.cadUnit = 4;
    G.measureN = 2;
    G.measures[0] = (Measure){1, 2, 3, 4, 1};
    G.measures[1] = (Measure){5, 6, 7, 8, 1};
    G.metaDirty = 1;
    save_meta();

    persistence_defaults();
    load_meta();
    check(G.performanceMode == 0, "latest meta retains performance mode");
    check(G.photoAngleSnap == 0, "latest meta retains photo angle snap");
    check(G.cadUnit == 4, "latest meta retains CAD unit");
    check(G.measureN == 2 && G.measures[1].bx == 7.0f,
          "latest meta retains complete measure payload");

    /* Exercise the real calibration control path: handle_ui saves only dirty
       metadata, so the unit toggle itself must mark the document dirty. */
    handle_ui(UI_CAL_UNIT, 1000);
    check(G.cadUnit == 0, "CAD unit control cycles the unit");
    persistence_defaults();
    load_meta();
    check(G.cadUnit == 0, "CAD unit control persists immediately");
}

static void test_legacy_meta7_settings(void) {
    MetaHead7 h = valid_meta7(0);
    void *f = fopen(META_PATH, "wb");
    check(f != 0, "create legacy ICM7 fixture");
    if (!f) return;
    fwrite(&h, sizeof(h), 1, f);
    fclose(f);

    persistence_defaults();
    load_meta();
    check(G.performanceMode == 1, "ICM7 retains persisted performance mode");
    check(G.cadUnit == 3, "ICM7 retains persisted CAD unit");
    check(G.photoAngleSnap == 1,
          "ICM7 without photo setting keeps the compatible default");
}

static void test_meta_count_and_truncation(void) {
    MetaHead7 h = valid_meta7(~(uint32_t)0);
    void *f = fopen(META_PATH, "wb");
    check(f != 0, "create malicious meta count fixture");
    if (!f) return;
    fwrite(&h, sizeof(h), 1, f);
    fclose(f);

    persistence_defaults();
    G.measureN = 17;
    load_meta();
    check(G.measureN == 0,
          "malicious unsigned measure count cannot become negative or phantom data");

    h = valid_meta7(3);
    Measure one = {11, 12, 13, 14, 1};
    f = fopen(META_PATH, "wb");
    check(f != 0, "create truncated meta payload fixture");
    if (!f) return;
    fwrite(&h, sizeof(h), 1, f);
    fwrite(&one, sizeof(one), 1, f);
    fclose(f);

    persistence_defaults();
    load_meta();
    check(G.measureN == 1,
          "truncated meta exposes only complete measure records");
    check(G.measures[0].ax == 11.0f && G.measures[0].by == 14.0f,
          "complete measure before truncation remains usable");

    /* Old files remain accepted, so their count field needs the same unsigned
       bounds handling as the newest format. */
    MetaHead2 h2;
    memset(&h2, 0, sizeof(h2));
    h2.magic[0] = 'I'; h2.magic[1] = 'C'; h2.magic[2] = 'M'; h2.magic[3] = '2';
    h2.pmin = 0.18f;
    h2.pmax = 1.0f;
    h2.curve = 1;
    h2.snap = 1;
    h2.count = ~(uint32_t)0;
    f = fopen(META_PATH, "wb");
    check(f != 0, "create malicious legacy meta count fixture");
    if (!f) return;
    fwrite(&h2, sizeof(h2), 1, f);
    fclose(f);

    persistence_defaults();
    G.measureN = 9;
    load_meta();
    check(G.measureN == 0,
          "legacy meta count cannot wrap into a negative measure count");
}

static void test_workspace_sanitization(void) {
    WorkspaceHead h = valid_workspace_head(1, 48, 1, 2);
    LegacyNoteObj note;
    ShapeObj shapes[48];
    BookmarkObj bookmark;
    GroupObj groups[2];
    memset(&note, 0, sizeof(note));
    memset(shapes, 0, sizeof(shapes));
    memset(&bookmark, 0, sizeof(bookmark));
    memset(groups, 0, sizeof(groups));
    note.active = 1;
    note.id = 1;
    memset(note.text, 'N', sizeof(note.text));
    bookmark.active = 1;
    bookmark.id = 1;
    memset(bookmark.name, 'B', sizeof(bookmark.name));
    groups[0].active = 1;
    groups[0].id = 1;
    groups[0].n = 0x7fffffff;
    for (int i = 0; i < 48; i++) {
        shapes[i].active = 1;
        shapes[i].id = i + 1;
        groups[0].refs[i] = (ObjRef){SEL_SHAPE, i};
    }
    groups[1].active = 1;
    groups[1].id = 2;
    groups[1].n = (-0x7fffffff - 1);

    void *f = fopen(WORKSPACE_PATH, "wb");
    check(f != 0, "create workspace sanitization fixture");
    if (!f) return;
    fwrite(&h, sizeof(h), 1, f);
    fwrite(&note, sizeof(note), 1, f);
    fwrite(shapes, sizeof(ShapeObj), 48, f);
    fwrite(&bookmark, sizeof(bookmark), 1, f);
    fwrite(groups, sizeof(GroupObj), 2, f);
    fclose(f);

    persistence_defaults();
    load_workspace();
    check(G.noteN == 1 && G.bookmarkN == 1 && G.groupN == 2,
          "workspace loads complete section records");
    check(G.notes[0].text[95] == 0,
          "workspace note text is always NUL terminated");
    check(G.bookmarks[0].name[31] == 0,
          "workspace bookmark name is always NUL terminated");
    check(G.groups[0].n == 48 && G.groups[1].n == 0,
          "workspace group member counts are clamped to storage bounds");
}

static void test_workspace_count_and_truncation(void) {
    WorkspaceHead h = valid_workspace_head(0, 0, 0, 0x7fffffff);
    void *f = fopen(WORKSPACE_PATH, "wb");
    check(f != 0, "create malicious workspace count fixture");
    if (!f) return;
    fwrite(&h, sizeof(h), 1, f);
    fclose(f);

    persistence_defaults();
    G.groupN = 7;
    load_workspace();
    check(G.groupN == 0,
          "malicious workspace group count cannot create phantom groups");

    h = valid_workspace_head(2, 1, 1, 1);
    LegacyNoteObj one;
    memset(&one, 0, sizeof(one));
    one.id = 7;
    one.active = 1;
    memset(one.text, 'T', sizeof(one.text));
    f = fopen(WORKSPACE_PATH, "wb");
    check(f != 0, "create truncated workspace payload fixture");
    if (!f) return;
    fwrite(&h, sizeof(h), 1, f);
    fwrite(&one, sizeof(one), 1, f);
    fclose(f);

    persistence_defaults();
    load_workspace();
    check(G.noteN == 1 && G.notes[0].id == 7,
          "truncated workspace exposes its one complete note");
    check(G.shapeN == 0 && G.bookmarkN == 0 && G.groupN == 0,
          "truncated workspace exposes no missing later records");
    check(G.notes[0].text[95] == 0,
          "truncated workspace still terminates complete note text");

    h = valid_workspace_head(1, 0, 0, 0);
    f = fopen(WORKSPACE_PATH, "wb");
    check(f != 0, "create partial workspace record fixture");
    if (!f) return;
    fwrite(&h, sizeof(h), 1, f);
    fwrite(&one, sizeof(one) - 1, 1, f);
    fclose(f);

    persistence_defaults();
    load_workspace();
    check(G.noteN == 0,
          "partial workspace record is not exposed as a complete object");
}

static void test_legacy_vfr1_count_and_truncation(void) {
    FrameHead h = valid_frame_head('1', 0x7fffffff);
    FrameObj first = {10.0f, 20.0f, 300.0f, 180.0f, 0x123456u, 31, 1};
    void *f = fopen(FRAME_PATH, "wb");
    check(f != 0, "create excessive VFR1 count fixture");
    if (!f) return;
    fwrite(&h, sizeof(h), 1, f);
    fwrite(&first, sizeof(first), 1, f);
    fclose(f);

    persistence_defaults();
    G.frameN = 1;
    G.frames[0] = (FrameObj){7.0f, 8.0f, 9.0f, 10.0f, 0x55u, 911, 1};
    copy_text_local(G.frameNames[0], 32, "Existing frame");
    G.frameLocked[0] = 1;
    load_frames();
    check(G.frameN == 1 && G.frames[0].id == 911,
          "excessive VFR1 count is rejected without exposing its payload");
    check(text_equal_local(G.frameNames[0], "Existing frame") &&
              G.frameLocked[0],
          "rejected excessive VFR1 count leaves live frame state unchanged");

    h = valid_frame_head('1', 3);
    FrameObj partial = {90.0f, 91.0f, 92.0f, 93.0f, 0xabcdefu, 32, 1};
    f = fopen(FRAME_PATH, "wb");
    check(f != 0, "create truncated VFR1 payload fixture");
    if (!f) return;
    fwrite(&h, sizeof(h), 1, f);
    fwrite(&first, sizeof(first), 1, f);
    fwrite(&partial, sizeof(partial) - 1, 1, f);
    fclose(f);

    persistence_defaults();
    memset(G.frames, 0x6b, sizeof(G.frames));
    load_frames();
    check(G.frameN == 1 && G.frames[0].id == 31,
          "truncated VFR1 exposes no partial or missing frame records");

    h = valid_frame_head('1', 1);
    f = fopen(FRAME_PATH, "wb");
    check(f != 0, "create partial-only VFR1 frame fixture");
    if (!f) return;
    fwrite(&h, sizeof(h), 1, f);
    fwrite(&partial, sizeof(partial) - 1, 1, f);
    fclose(f);

    persistence_defaults();
    G.frameN = 7;
    memset(G.frames, 0x7c, sizeof(G.frames));
    load_frames();
    check(G.frameN == 0,
          "partial VFR1 frame is not exposed as a complete object");

    h = valid_frame_head('1', (-0x7fffffff - 1));
    f = fopen(FRAME_PATH, "wb");
    check(f != 0, "create negative VFR1 count fixture");
    if (!f) return;
    fwrite(&h, sizeof(h), 1, f);
    fclose(f);

    persistence_defaults();
    G.frameN = 1;
    G.frames[0] = (FrameObj){11.0f, 12.0f, 13.0f, 14.0f, 0x66u, 912, 1};
    load_frames();
    check(G.frameN == 1 && G.frames[0].id == 912,
          "negative VFR1 count is rejected without mutating live frames");
}

static void test_frame_save_round_trip_and_open_failure(void) {
    persistence_defaults();
    G.frameN = 2;
    G.nextFrameId = 19;
    G.frames[0] = (FrameObj){1.0f, 2.0f, 100.0f, 80.0f, 0x112233u, 17, 1};
    G.frames[1] = (FrameObj){5.0f, 6.0f, 200.0f, 90.0f, 0x445566u, 18, 1};
    copy_text_local(G.frameNames[0], 32, "Alpha region");
    copy_text_local(G.frameNames[1], 32, "Beta region");
    G.frameLocked[0] = 1;
    G.frameSizeLocked[1] = 1;
    G.visInk = G.visPhotos = G.visFrames = G.visNotes = 1;
    save_frames();

    persistence_defaults();
    load_frames();
    check(G.frameN == 2 && G.nextFrameId == 19 &&
              G.frames[0].id == 17 && G.frames[1].id == 18,
          "current frame format round-trips complete frame records");
    check(text_equal_local(G.frameNames[0], "Alpha region") &&
              text_equal_local(G.frameNames[1], "Beta region") &&
              G.frameLocked[0] && G.frameSizeLocked[1],
          "current frame format round-trips names and lock metadata");

    FrameObj before = G.frames[0];
    int beforeN = G.frameN;
    snprintf(G.framePath, sizeof(G.framePath), "%s", "./build");
    save_frames();
    check(G.frameN == beforeN && G.frames[0].id == before.id &&
              G.frames[0].x == before.x,
          "failed frame-file open leaves live frame state unchanged");
}

static void test_new_note_cancel_rollback(void) {
    persistence_defaults();
    G.screenW = 1200;
    G.screenH = 800;
    G.noteN = 1;
    G.nextNoteId = 18;
    G.notes[0].id = 17;
    G.notes[0].active = 1;
    G.notes[0].type = NOTE_TEXT;
    G.notes[0].x = 40.0f;
    G.notes[0].y = 60.0f;
    G.notes[0].w = 220.0f;
    G.notes[0].h = 80.0f;
    copy_text_local(G.notes[0].text, 96, "Original note");
    save_workspace();

    add_note_at_center(NOTE_TEXT);
    check(G.noteN == 2 && G.editorOpen && G.editorCreated,
          "new text note is tracked as an editor-created object");
    check(G.editorTarget == 1 && G.notes[1].id == 18,
          "new-note editor targets only the appended note");

    copy_text_local(G.notes[1].text, 96, "Transient draft");
    editor_cancel();
    check(G.noteN == 1 && G.notes[0].id == 17 && G.notes[0].active,
          "cancel removes only the newly created note");
    check(text_equal_local(G.notes[0].text, "Original note"),
          "cancel leaves the original note unchanged");
    check(!G.editorOpen && !G.editorCreated && G.editorTarget == -1,
          "cancel clears new-note editor state");

    memset(G.notes, 0, sizeof(G.notes));
    G.noteN = 0;
    load_workspace();
    check(G.noteN == 1 && G.notes[0].id == 17 &&
              text_equal_local(G.notes[0].text, "Original note"),
          "cancelled new note stays absent after workspace reload");
}

static void test_utf8_copy_boundaries(void) {
    char exact[4];
    char short_buf[4];
    char invalid[8];
    char sanitized[16];
    char supplementary[16];
    char round_trip[16];
    uint32_t scalar = 0;
    const unsigned char overlong[] = {0xC0, 0xAF, 0};
    const unsigned char surrogate[] = {0xED, 0xA0, 0x80, 0};
    const unsigned char above_unicode[] = {0xF4, 0x90, 0x80, 0x80, 0};
    const char emoji_utf8[] = {'A', (char)0xF0, (char)0x9F, (char)0x98,
                               (char)0x80, 'Z', 0};
    const jchar emoji_utf16[] = {'A', 0xD83D, 0xDE00, 'Z'};
    jchar decoded[8];
    copy_text_local(exact, sizeof(exact), "\xE2\x82\xACX");
    check((unsigned char)exact[0] == 0xE2 &&
              (unsigned char)exact[1] == 0x82 &&
              (unsigned char)exact[2] == 0xAC && exact[3] == 0,
          "UTF-8 copy keeps the last complete multibyte character");
    copy_text_local(short_buf, sizeof(short_buf), "\xF0\x9F\x98\x80");
    check(short_buf[0] == 0,
          "UTF-8 copy never persists a split four-byte character");
    copy_text_local(invalid, sizeof(invalid), "A\xE2Z");
    check(text_equal_local(invalid, "A?Z"),
          "UTF-8 copy sanitizes malformed input bytes");

    check(utf8_decode_one(overlong, &scalar) < 0,
          "UTF-8 decoder rejects overlong scalar encodings");
    copy_text_local(sanitized, sizeof(sanitized), (const char *)overlong);
    check(text_equal_local(sanitized, "??"),
          "UTF-8 copy sanitizes every byte of an overlong encoding");

    check(utf8_decode_one(surrogate, &scalar) < 0,
          "UTF-8 decoder rejects encoded UTF-16 surrogates");
    copy_text_local(sanitized, sizeof(sanitized), (const char *)surrogate);
    check(text_equal_local(sanitized, "???"),
          "UTF-8 copy sanitizes an encoded surrogate");

    check(utf8_decode_one(above_unicode, &scalar) < 0,
          "UTF-8 decoder rejects scalars above U+10FFFF");
    copy_text_local(sanitized, sizeof(sanitized),
                    (const char *)above_unicode);
    check(text_equal_local(sanitized, "????"),
          "UTF-8 copy sanitizes an out-of-range scalar");

    copy_utf16_to_utf8(supplementary, sizeof(supplementary),
                       emoji_utf16, 4);
    check(text_equal_local(supplementary, emoji_utf8),
          "UTF-16 surrogate pair becomes standard four-byte UTF-8");

    int decoded_n = copy_utf8_to_utf16(decoded, 8, emoji_utf8);
    check(decoded_n == 4 && decoded[0] == 'A' && decoded[1] == 0xD83D &&
              decoded[2] == 0xDE00 && decoded[3] == 'Z',
          "four-byte UTF-8 becomes the matching UTF-16 surrogate pair");
    copy_utf16_to_utf8(round_trip, sizeof(round_trip), decoded, decoded_n);
    check(text_equal_local(round_trip, emoji_utf8),
          "supplementary Unicode survives UTF-8 and UTF-16 round trip");
}

static void test_lock_measure_and_outline_regressions(void) {
    persistence_defaults();
    G.noteN = 1;
    G.notes[0] = (NoteObj){10, 20, 200, 80, 0x123456u, 1, 1, NOTE_TEXT, 0, "Locked layer note"};
    G.lockNotes = 1;
    selection_set_one(SEL_NOTE, 0);
    selection_delete();
    check(G.notes[0].active,
          "class lock prevents selection deletion");

    persistence_defaults();
    G.measureN = 2;
    G.measures[0] = (Measure){10, 10, 20, 10, 1};
    G.measures[1] = (Measure){30, 30, 50, 30, 1};
    selection_set_one(SEL_MEASURE, 0);
    selection_delete();
    check(G.measureN == 1 && G.measures[0].ax == 30.0f &&
              G.measures[0].active,
          "selection deletion compacts CAD data without ghost geometry");

    persistence_defaults();
    G.shapeN = 1;
    G.shapes[0] = (ShapeObj){100, 100, 300, 240, 0xffffffu, 3.0f, 7, 1,
                             SHAPE_RECT, 0};
    check(shape_hit_screen(&G.shapes[0], 100, 170),
          "outlined rectangle is selectable near its edge");
    check(!shape_hit_screen(&G.shapes[0], 200, 170),
          "outlined rectangle no longer consumes its empty interior");
}

static void release_test_history(void) {
    for (int i = 0; i < G.strokeN; i++) {
        if (G.strokes[i].pts) free(G.strokes[i].pts);
    }
    for (int i = 0; i < G.actionN; i++) free_action(&G.actions[i]);
    if (G.strokes) free(G.strokes);
    if (G.actions) free(G.actions);
    G.strokes = 0;
    G.actions = 0;
    G.strokeN = G.strokeCap = 0;
    G.actionN = G.actionCap = G.actionCursor = 0;
}

static void seed_test_stroke(int index, uint32_t color, float y) {
    check(ensure_strokes(index + 1), "stroke regression fixture allocates storage");
    Stroke *stroke = &G.strokes[index];
    memset(stroke, 0, sizeof(*stroke));
    check(ensure_points(stroke, 4), "stroke regression fixture allocates points");
    stroke->n = 4;
    stroke->active = 1;
    stroke->color = color;
    stroke->baseWidth = 5.0f;
    for (int i = 0; i < 4; i++) {
        stroke->pts[i] = (Point){20.0f + i * 30.0f, y, 0.6f};
    }
    stroke->minx = stroke->pts[0].x;
    stroke->maxx = stroke->pts[3].x;
    stroke->miny = stroke->maxy = y;
    if (G.strokeN <= index) G.strokeN = index + 1;
}

static void test_shape_and_theme_cache_signals(void) {
    persistence_defaults();
    G.sceneRevision = 17;
    G.shapeDrawing = 1;
    G.color = 0x336699u;
    G.liveShape = (ShapeObj){10, 20, 180, 120, 0, 0, 0, 0,
                             SHAPE_RECT, 0};
    int before = G.sceneRevision;
    commit_live_shape();
    check(G.shapeN == 1 && G.shapes[0].active,
          "shape commit persists the live shape");
    check(G.sceneRevision != before,
          "shape commit invalidates the GPU canvas scene signal");

    persistence_defaults();
    G.sceneRevision = 29;
    before = G.sceneRevision;
    theme_role_set(3, 0xabcdefu);
    check(G.theme.text == 0xabcdefu && G.sceneRevision != before,
          "non-accent theme changes invalidate cached canvas content");
}

static void test_shape_and_clear_locks(void) {
    persistence_defaults();
    seed_test_stroke(0, 0x224466u, 40.0f);
    G.currentStroke = 0;
    G.tool = MODE_PEN;
    G.lockShapes = 1;
    G.strokeLastMoveMs = 100;
    check(!clean_current_stroke_to_shape(500) && G.shapeN == 0 &&
              G.currentStroke == 0 && G.strokes[0].active,
          "automatic shape conversion respects the Shapes layer lock");
    release_test_history();

    persistence_defaults();
    seed_test_stroke(0, 0x112233u, 40.0f);
    seed_test_stroke(1, 0x8000aaffu, 80.0f);
    G.lockInk = 1;
    clear_canvas();
    check(G.strokes[0].active && !G.strokes[1].active,
          "clear preserves locked ink while clearing unlocked marker strokes");
    release_test_history();

    persistence_defaults();
    seed_test_stroke(0, 0x112233u, 40.0f);
    seed_test_stroke(1, 0x8000aaffu, 80.0f);
    G.lockMarker = 1;
    clear_canvas();
    check(!G.strokes[0].active && G.strokes[1].active,
          "clear preserves locked marker while clearing unlocked ink strokes");
    release_test_history();
}

static int group_has_ref(const GroupObj *group, int type, int index) {
    for (int i = 0; group && i < group->n; i++) {
        if (group->refs[i].type == type && group->refs[i].index == index)
            return 1;
    }
    return 0;
}

static void test_measure_group_reference_remap(void) {
    persistence_defaults();
    G.measureN = 3;
    for (int i = 0; i < G.measureN; i++)
        G.measures[i] = (Measure){(float)(10 + i), 0, (float)(20 + i), 0, 1};
    G.noteN = 1;
    G.notes[0].active = 1;
    G.groupN = 1;
    G.groups[0].active = 1;
    G.groups[0].n = 4;
    G.groups[0].refs[0] = (ObjRef){SEL_MEASURE, 0};
    G.groups[0].refs[1] = (ObjRef){SEL_MEASURE, 1};
    G.groups[0].refs[2] = (ObjRef){SEL_MEASURE, 2};
    G.groups[0].refs[3] = (ObjRef){SEL_NOTE, 0};
    G.selectedMeasure = 1;
    delete_selected_measure();
    check(G.measureN == 2 && G.groups[0].n == 3 &&
              group_has_ref(&G.groups[0], SEL_MEASURE, 0) &&
              group_has_ref(&G.groups[0], SEL_MEASURE, 1) &&
              group_has_ref(&G.groups[0], SEL_NOTE, 0),
          "measure deletion removes the deleted group ref and shifts later refs");

    persistence_defaults();
    G.measureN = 3;
    for (int i = 0; i < G.measureN; i++)
        G.measures[i] = (Measure){(float)(30 + i), 0, (float)(40 + i), 0, 1};
    G.noteN = 1;
    G.notes[0].active = 1;
    G.groupN = 1;
    G.groups[0].active = 1;
    G.groups[0].n = 4;
    G.groups[0].refs[0] = (ObjRef){SEL_MEASURE, 0};
    G.groups[0].refs[1] = (ObjRef){SEL_MEASURE, 1};
    G.groups[0].refs[2] = (ObjRef){SEL_MEASURE, 2};
    G.groups[0].refs[3] = (ObjRef){SEL_NOTE, 0};
    selection_set_one(SEL_MEASURE, 1);
    selection_delete();
    check(G.measureN == 2 && G.groups[0].n == 3 &&
              group_has_ref(&G.groups[0], SEL_MEASURE, 0) &&
              group_has_ref(&G.groups[0], SEL_MEASURE, 1) &&
              group_has_ref(&G.groups[0], SEL_NOTE, 0),
          "selection measure deletion remaps persisted group references");

    persistence_defaults();
    G.measureN = 64;
    for (int i = 0; i < G.measureN; i++)
        G.measures[i] = (Measure){(float)i, 0, (float)i + 2.0f, 0, 1};
    G.noteN = 1;
    G.notes[0].active = 1;
    G.groupN = 1;
    G.groups[0].active = 1;
    G.groups[0].n = 4;
    G.groups[0].refs[0] = (ObjRef){SEL_MEASURE, 0};
    G.groups[0].refs[1] = (ObjRef){SEL_MEASURE, 1};
    G.groups[0].refs[2] = (ObjRef){SEL_MEASURE, 63};
    G.groups[0].refs[3] = (ObjRef){SEL_NOTE, 0};
    G.measuring = 1;
    G.liveMeasure = (Measure){1000, 0, 1010, 0, 1};
    end_measure();
    check(G.measureN == 64 && G.measures[63].ax == 1000.0f &&
              G.groups[0].n == 3 &&
              group_has_ref(&G.groups[0], SEL_MEASURE, 0) &&
              group_has_ref(&G.groups[0], SEL_MEASURE, 62) &&
              group_has_ref(&G.groups[0], SEL_NOTE, 0),
          "measure overflow drops the oldest group ref and shifts surviving refs");
}

int main(void) {
    test_latest_meta_settings_round_trip();
    test_legacy_meta7_settings();
    test_meta_count_and_truncation();
    test_workspace_sanitization();
    test_workspace_count_and_truncation();
    test_legacy_vfr1_count_and_truncation();
    test_frame_save_round_trip_and_open_failure();
    test_new_note_cancel_rollback();
    test_utf8_copy_boundaries();
    test_lock_measure_and_outline_regressions();
    test_shape_and_theme_cache_signals();
    test_shape_and_clear_locks();
    test_measure_group_reference_remap();
    if (fail) {
        printf("CANVAS PERSISTENCE REGRESSION: FAIL\n");
        return 1;
    }
    printf("CANVAS PERSISTENCE REGRESSION: PASS\n");
    return 0;
}
